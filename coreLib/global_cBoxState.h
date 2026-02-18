#ifndef GLOBAL_CBOXSTATE_H
#define GLOBAL_CBOXSTATE_H

// ============================================================================
// global_CBoxState.h
// ============================================================================
// Per-CPU CBox storage singleton.
//
// CBox is the ONLY IPR tier with cross-thread atomic writers:
//   - I/O threads assert IRQ lines
//   - Other CPUs send IPIs
//   - IRQController reads/writes interrupt state
//
// This is why CBox is NOT inside GlobalCPUState — different writer
// contract. GlobalCPUState guarantees single-writer (CPU run loop).
// CBox requires atomics because any thread may write at any time.
//
// Usage:
//   // At init (IRQController, PalService, etc.)
//   IPRStorage_CBox& cbox = globalCBoxState().cbox(cpuId);
//
//   // From I/O thread
//   globalCBoxState().cbox(targetCpu).postIRQ(ipl, vector);
//
//   // From another CPU (IPI)
//   globalCBoxState().cbox(targetCpu).postIPIR(request, data);
//
//   // From CPU run loop (poll)
//   if (globalCBoxState().cbox(myCpu).shouldPoll()) { ... }
//
// Removed from CBox (write-only triggers — no storage):
//   tbia, tbiap, tbis, tbisd, tbisi
//   These are dispatched directly to SPAM in writeIPR.
// ============================================================================

#include <QtGlobal>
#include <algorithm>
#include <cstring>
#include <atomic>
#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"


// ============================================================================
// IPRStorage_CBox — Per-CPU Cross-Thread Interrupt / IPI State
// ============================================================================
// All mutable fields are std::atomic — safe for concurrent access.
// Hot-path fields packed into first cache line for polling.
// ============================================================================
struct alignas(64) IPRStorage_CBox
{
    // ========================================================================
    // CACHE LINE 0: Hot Path — polled every instruction cycle
    // ========================================================================

    std::atomic<quint64> irq_pending{ 0 };    //  0: Pending IPL bitmask
    std::atomic<quint32> ipir_request{ 0 };   //  8: IPI request bits
    std::atomic<quint32> ipir_data{ 0 };      // 12: IPI data payload

    std::atomic<quint32> irq_control{ 0 };    // 16: PACKED control bits
    //   [7:0]   Current IPL
    //   [15:8]  Pending vector
    //   [16]    irq_mchk_pending
    //   [17]    irq_perf_pending
    //   [18]    has_pending_interrupt
    //   [19]    has_pending_ast
    //   [20]    has_pending_event (MASTER POLL)
    //   [31:21] reserved

    quint64 pctx{ 0 };                        // 20: Process Context (DTB ASIDs)

    std::atomic<quint16> sirr{ 0 };           // 28: Software IRQ Request
    std::atomic<quint16> sisr{ 0 };           // 30: Software IRQ Summary

    std::atomic<quint32> ast_state{ 0 };      // 32: PACKED AST state
    //   [3:0]   astrr (AST Request Register)
    //   [7:4]   ast_level
    //   [15:8]  ast_pending
    //   [31:16] reserved

    quint32 pad_hot{ 0 };                     // 36: pad

    // ?? Remaining space in cache line 0 ??
    quint64 tbchk{ 0 };                       // 40: TLB check (read-only query)
    quint64 pad_cold[2]{};                  // 48-63: reserved

    // TOTAL: 64 bytes (1 cache line)
    //
    // NOTE: virbnd and sysptbr are PAL IPRs (MFPR/MTPR, single writer).
    // They live in IPRStorage_PalIPR (m_iprGlobalMaster->x->virbnd/sysptbr),
    // NOT here. CBox is exclusively for cross-thread atomic state.

    // ========================================================================
    // IRQ CONTROL ACCESSORS
    // ========================================================================

    // Current IPL [7:0]
    AXP_HOT AXP_ALWAYS_INLINE quint8 getCurrentIPL() const noexcept {
        return static_cast<quint8>(irq_control.load(std::memory_order_acquire) & 0xFF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setCurrentIPL(quint8 ipl) noexcept {
        quint32 old = irq_control.load(std::memory_order_relaxed);
        irq_control.store((old & ~0xFFU) | ipl, std::memory_order_release);
    }

    // Pending Vector [15:8]
    AXP_HOT AXP_ALWAYS_INLINE quint8 getPendingVector() const noexcept {
        return static_cast<quint8>((irq_control.load(std::memory_order_acquire) >> 8) & 0xFF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setPendingVector(quint8 vector) noexcept {
        quint32 old = irq_control.load(std::memory_order_relaxed);
        irq_control.store((old & ~0xFF00U) | (static_cast<quint32>(vector) << 8), std::memory_order_release);
    }

    // Machine Check Pending [16]
    AXP_HOT AXP_ALWAYS_INLINE bool getMchkPending() const noexcept {
        return (irq_control.load(std::memory_order_acquire) & (1U << 16)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setMchkPending(bool v) noexcept {
        if (v) irq_control.fetch_or(1U << 16, std::memory_order_release);
        else   irq_control.fetch_and(~(1U << 16), std::memory_order_release);
    }

    // Performance Counter Pending [17]
    AXP_HOT AXP_ALWAYS_INLINE bool getPerfPending() const noexcept {
        return (irq_control.load(std::memory_order_acquire) & (1U << 17)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setPerfPending(bool v) noexcept {
        if (v) irq_control.fetch_or(1U << 17, std::memory_order_release);
        else   irq_control.fetch_and(~(1U << 17), std::memory_order_release);
    }

    // Has Pending Interrupt [18]
    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingInterrupt() const noexcept {
        return (irq_control.load(std::memory_order_acquire) & (1U << 18)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setHasPendingInterrupt(bool v) noexcept {
        if (v) irq_control.fetch_or(1U << 18, std::memory_order_release);
        else   irq_control.fetch_and(~(1U << 18), std::memory_order_release);
    }

    // Has Pending AST [19]
    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingAST() const noexcept {
        return (irq_control.load(std::memory_order_acquire) & (1U << 19)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setHasPendingAST(bool v) noexcept {
        if (v) irq_control.fetch_or(1U << 19, std::memory_order_release);
        else   irq_control.fetch_and(~(1U << 19), std::memory_order_release);
    }

    // Has Pending Event [20] — MASTER POLL FLAG
    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingEvent() const noexcept {
        return (irq_control.load(std::memory_order_acquire) & (1U << 20)) != 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setHasPendingEvent(bool v) noexcept {
        if (v) irq_control.fetch_or(1U << 20, std::memory_order_release);
        else   irq_control.fetch_and(~(1U << 20), std::memory_order_release);
    }

    // ========================================================================
    // AST STATE ACCESSORS
    // ========================================================================

    // ASTRR [3:0]
    AXP_HOT AXP_ALWAYS_INLINE quint8 getASTRR() const noexcept {
        return static_cast<quint8>(ast_state.load(std::memory_order_acquire) & 0xF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setASTRR(quint8 v) noexcept {
        quint32 old = ast_state.load(std::memory_order_relaxed);
        ast_state.store((old & ~0xFU) | (v & 0xF), std::memory_order_release);
    }

    // AST Level [7:4]
    AXP_HOT AXP_ALWAYS_INLINE quint8 getASTLevel() const noexcept {
        return static_cast<quint8>((ast_state.load(std::memory_order_acquire) >> 4) & 0xF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setASTLevel(quint8 level) noexcept {
        quint32 old = ast_state.load(std::memory_order_relaxed);
        ast_state.store((old & ~0xF0U) | ((level & 0xF) << 4), std::memory_order_release);
    }

    // AST Pending [15:8]
    AXP_HOT AXP_ALWAYS_INLINE quint8 getASTPending() const noexcept {
        return static_cast<quint8>((ast_state.load(std::memory_order_acquire) >> 8) & 0xFF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setASTPending(quint8 pending) noexcept {
        quint32 old = ast_state.load(std::memory_order_relaxed);
        ast_state.store((old & ~0xFF00U) | (static_cast<quint32>(pending) << 8), std::memory_order_release);
    }

    // ========================================================================
    // PCTX / DTB ASID ACCESSORS
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE quint64 getPCTX() const noexcept { return pctx; }
    AXP_HOT AXP_ALWAYS_INLINE void setPCTX(quint64 v) noexcept { pctx = v; }

    AXP_HOT AXP_ALWAYS_INLINE quint8 getDTB0ASID() const noexcept {
        return static_cast<quint8>((pctx >> 32) & 0xFF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setDTB0ASID(quint8 v) noexcept {
        pctx = (pctx & ~(0xFFULL << 32)) | (static_cast<quint64>(v) << 32);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint8 getDTB1ASID() const noexcept {
        return static_cast<quint8>((pctx >> 40) & 0xFF);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setDTB1ASID(quint8 v) noexcept {
        pctx = (pctx & ~(0xFFULL << 40)) | (static_cast<quint64>(v) << 40);
    }

    // ========================================================================
    // OPERATIONAL METHODS
    // ========================================================================

    inline void postIRQ(quint8 ipl, quint8 vector) noexcept {
        setCurrentIPL(ipl);
        setPendingVector(vector);
        irq_pending.fetch_or(1ULL << ipl, std::memory_order_release);
        setHasPendingInterrupt(true);
        setHasPendingEvent(true);
    }

    inline void clearIRQ(quint8 ipl) noexcept {
        irq_pending.fetch_and(~(1ULL << ipl), std::memory_order_acq_rel);
        if (irq_pending.load(std::memory_order_acquire) == 0) {
            setHasPendingInterrupt(false);
            // Only clear master poll if no other events pending
            if (!hasPendingAST())
                setHasPendingEvent(false);
        }
    }

    inline void postIPIR(quint32 request, quint32 data) noexcept {
        ipir_data.store(data, std::memory_order_release);
        ipir_request.fetch_or(request, std::memory_order_release);
        setHasPendingEvent(true);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasIRQPending() const noexcept {
        return irq_pending.load(std::memory_order_acquire) != 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasIPIRPending() const noexcept {
        return ipir_request.load(std::memory_order_acquire) != 0;
    }

    inline quint32 drainIPIR() noexcept {
        return ipir_request.exchange(0, std::memory_order_acq_rel);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool shouldPoll() const noexcept {
        return hasPendingEvent();
    }

    // ========================================================================
    // RESET
    // ========================================================================

    inline void reset() noexcept {
        irq_pending.store(0, std::memory_order_relaxed);
        ipir_request.store(0, std::memory_order_relaxed);
        ipir_data.store(0, std::memory_order_relaxed);
        irq_control.store(0, std::memory_order_relaxed);
        pctx = 0;
        sirr.store(0, std::memory_order_relaxed);
        sisr.store(0, std::memory_order_relaxed);
        ast_state.store(0, std::memory_order_relaxed);
        tbchk = 0;
    }
};

// ============================================================================
// Compile-Time Verification
// ============================================================================
static_assert(alignof(IPRStorage_CBox) == 64, "CBox must be cache-line aligned");
static_assert(sizeof(IPRStorage_CBox) == 128, "CBox must be exactly 1 cache line");


// ============================================================================
// GlobalCBoxState — Per-CPU CBox Singleton
// ============================================================================
// Separate from GlobalCPUState because CBox has cross-thread writers.
// GlobalCPUState guarantees single-writer per CPU. CBox does not.
// ============================================================================

class GlobalCBoxState final
{
public:
    GlobalCBoxState() noexcept
    {
        m_cpuCount = 1;
        resetAll();
    }

    ~GlobalCBoxState() noexcept = default;
    GlobalCBoxState(const GlobalCBoxState&) = delete;
    GlobalCBoxState& operator=(const GlobalCBoxState&) const
    { // NOOP
        //NOOP
        GlobalCBoxState cboxState; // NOOP
        return cboxState;
    }

    // ================================================================
    // CPU Count
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE void setCpuCount(quint32 count) noexcept
    {
        if (count == 0) count = 1;
        count      = std::min(count, static_cast<quint32>(MAX_CPUS));
        m_cpuCount = count;
        resetAll();
    }

    AXP_HOT AXP_ALWAYS_INLINE quint32 cpuCount() const noexcept { return m_cpuCount; }

    // ================================================================
    // Per-CPU Accessors
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_CBox& cbox(CPUIdType id) noexcept
    {
        return m_cbox[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_CBox& cbox(CPUIdType id) const noexcept
    {
        return m_cbox[idx(id)];
    }

    // ================================================================
    // Reset
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE void resetCPU(CPUIdType id) noexcept
    {
        m_cbox[idx(id)].reset();
    }

    AXP_HOT AXP_ALWAYS_INLINE void resetAll() noexcept
    {
        for (quint32 i = 0; i < m_cpuCount; ++i)
            m_cbox[i].reset();
    }

private:
    AXP_HOT AXP_ALWAYS_INLINE quint32 idx(CPUIdType id) const noexcept
    {
        return static_cast<quint32>(id) % static_cast<quint32>(MAX_CPUS);
    }

private:
    quint32 m_cpuCount{ 1 };
    alignas(64) IPRStorage_CBox m_cbox[MAX_CPUS];
};


// ============================================================================
// Singleton Accessor
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE
GlobalCBoxState& globalCBoxState() noexcept
{
    static GlobalCBoxState s_instance;
    return s_instance;
}


// ============================================================================
// Legacy / Migration Bridge
// ============================================================================
// Drop-in replacement for old globalIPRCBox(cpuId) call sites.
// New code should use globalCBoxState().cbox(cpuId) directly.
// ============================================================================

AXP_HOT AXP_ALWAYS_INLINE
IPRStorage_CBox& globalIPRCBox(CPUIdType id) noexcept
{
    return globalCBoxState().cbox(id);
}

AXP_HOT AXP_ALWAYS_INLINE
IPRStorage_CBox& globalCBox(CPUIdType id) noexcept
{
    return globalCBoxState().cbox(id);
}


#endif // GLOBAL_CBOXSTATE_H