// ============================================================================
// global_RegisterMaster_hot.h - Check if integer overflow trap is enabled
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef GLOBAL_REGISTERMASTER_HOT_H
#define GLOBAL_REGISTERMASTER_HOT_H

// ============================================================================
// global_RegisterMaster_hot.h
// ============================================================================
// SINGLE SOURCE OF TRUTH for all per-CPU hot-path register state.
//
// Consolidates (and replaces):
//   - global_ARCHRegs.h        (IntRegs, FloatRegs, PalShadow)
//   - HWPCB_core.h             (Process Control Block)
//   - IPRStorage_Hot64.h       (Run-loop IPRs, deduplicated)
//   - IPRStorage_HotExt.h      (PAL/exception IPRs, deduplicated)
//
// NOT included (cross-thread atomics, stays external):
//   - IPRStorage_CBox.h        (IRQ latches, IPI, SIRR — different writer)
//
// Architecture:
//   GlobalCPUState (singleton)
//   ├── IntRegs[MAX_CPUS]          + snapshot
//   ├── FloatRegs[MAX_CPUS]        + snapshot  (includes FPCR)
//   ├── PalShadow[MAX_CPUS]        (CPU hardware, no snapshot)
//   ├── HWPCB[MAX_CPUS]            + snapshot
//   ├── RunLoopIPR[MAX_CPUS]       (cc, cc_ctl, PCC, personality)
//   ├── PalIPR[MAX_CPUS]           (PAL regs, TLB staging, pal_temp)
//   └── OSF[MAX_CPUS]             (OSF personality: entry vectors, wrkgp)
//
// Access patterns:
//   CPUStateView view;
//   view.bind(globalCPUState(), cpuId);
//   quint64 val = view.readInt(16);
//   view.hwpcb()->setPC(0x20000000);
//   view.saveContext();
//
// Concurrency: one writer per cpuId (CPU run-loop thread).
// ============================================================================

/*
 m_iprGlobalMaster->i->   IntRegs      r[32]
m_iprGlobalMaster->f->   FloatRegs    f[31], fpcr
m_iprGlobalMaster->p->   PalShadow    bank0[27], bank1[23], enabled
m_iprGlobalMaster->h->   HWPCB        pc, ps, cm, ipl, asn, sp[], exc_addr, ptbr, va_fault, aster, astsr...
m_iprGlobalMaster->r->   RunLoop      cc, cc_ctl, pccState, intrFlag, pal_personality
m_iprGlobalMaster->x->   PalIPR       pal_base, scbb, pcbb, vptb, prbr, virbnd, sysptbr, mces, whami,
                                       i_ctl, m_ctl, dc_ctl, va_ctl, exc_sum, exc_mask, mm_stat,
                                       TLB staging, pal_temp[32], write buffer, perfmon
m_iprGlobalMaster->o->   OSF          vptptr, ent_int/arith/mm/fault/una/sys, wrkgp

 *
 */

#include <QtGlobal>
#include <QString>
#include <cstring>
#include <atomic>

#include "HWPCB_SwapContext.h"
#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "palLib_EV6/PalVectorId_refined.h"


struct CPUStateView;
// ============================================================================
// Forward Declarations
// ============================================================================
class GuestMemory;


#pragma region HWPCB Context

// ============================================================================
// SwapContext Result
// ============================================================================
struct SwapContextResult
{
    quint64 oldPCBB;        // Return in R0 to caller
    bool    ptbrChanged;    // True if PTBR differs (non-ASM TLB flush needed)
    bool    asnChanged;     // True if ASN differs
    bool    success;        // False if alignment check failed
};



// ============================================================================
// hwpcbSwapContext - Full context switch
// ============================================================================
//
// Implements the EV6 SWPCTX algorithm from the Alpha Architecture
// Reference Manual:
//
//   1. Validate R16 alignment (128-byte boundary)
//   2. Save current IPR state -> old HWPCB in physical memory
//   3. Load new IPR state <- new HWPCB from physical memory
//   4. Conditionally flush TLB (if PTBR or ASN changed)
//   5. Update PCBB IPR
//   6. Return old PCBB in R0
//
// Parameters:
//   cpuId          - CPU performing the context switch
//   hwpcb          - Pointer to this CPU's internal HWPCB struct
//   oldPCBB_PA     - Current PCBB IPR value (PA of old HWPCB)
//   newPCBB_PA     - R16 value (PA of new HWPCB)
//   guestMem       - GuestMemory for physical reads/writes
//   tlb            - TLB for invalidation
//   hwCycleCounter - Current hardware cycle counter (PCC<31:0>)
//   currentR30     - Current value of R30 (stack pointer)
//
// Returns:
//   SwapContextResult with old PCBB and TLB flush indicators
//
// The caller is responsible for:
//   - Writing R0 = result.oldPCBB
//   - Loading R30 = hwpcb->loadSP(hwpcb->cm) after this returns
//   - Updating PCBB IPR = newPCBB_PA
//   - Issuing memory barrier / pipeline flush
//
// ============================================================================

// ############################################################################
//                    SECTION 2: HWPCB (Process Control Block)
// ############################################################################


// ============================================================================
// Physical HWPCB Layout Constants (guest memory, fixed by architecture)
// ============================================================================
namespace HWPCBLayout {
    constexpr quint64 KSP = 0x00;
    constexpr quint64 ESP = 0x08;
    constexpr quint64 SSP = 0x10;
    constexpr quint64 USP = 0x18;
    constexpr quint64 PTBR = 0x20;
    constexpr quint64 ASN = 0x28;
    constexpr quint64 ASTSR_EN = 0x30;
    constexpr quint64 FEN = 0x38;
    constexpr quint64 PCC = 0x40;
    constexpr quint64 UNQ = 0x48;
    constexpr quint64 DAT = 0x50;
    constexpr quint64 HWPCB_SIZE = 0x58;
    constexpr quint64 ALIGNMENT_MASK = 0x7F;
}


#pragma endregion HWPCB Context

// ############################################################################
//                    SECTION 1: REGISTER STORAGE STRUCTS
// ############################################################################


// ============================================================================
// IPRStorage_IntRegs — Per-CPU Integer Registers (256 bytes, 4 cache lines)
// ============================================================================
struct alignas(64) IPRStorage_IntRegs final
{
    quint64 r[32]{};

    AXP_HOT AXP_ALWAYS_INLINE void clear() noexcept
    {
        std::memset(r, 0, sizeof(r));
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 read(quint8 regNum) const noexcept
    {
        return (regNum == 31) ? 0ULL : r[regNum & 31u];
    }

    AXP_HOT AXP_ALWAYS_INLINE void write(quint8 regNum, quint64 value) noexcept
    {
        if (regNum != 31) r[regNum & 31u] = value;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64* raw() noexcept { return r; }
    AXP_HOT AXP_ALWAYS_INLINE const quint64* raw() const noexcept { return r; }
};


// ============================================================================
// IPRStorage_FloatRegs — Per-CPU Float Registers (256 bytes, 4 cache lines)
// FPCR occupies the F31 physical slot.
// ============================================================================
struct alignas(64) IPRStorage_FloatRegs final
{
    quint64 f[31]{};
    quint64 fpcr{};

    AXP_HOT AXP_ALWAYS_INLINE void clear() noexcept
    {
        std::memset(f, 0, sizeof(f));
        fpcr = 0;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 read(quint8 regNum) const noexcept
    {
        return (regNum == 31) ? 0ULL : f[regNum & 31u];
    }

    AXP_HOT AXP_ALWAYS_INLINE void write(quint8 regNum, quint64 value) noexcept
    {
        if (regNum != 31) f[regNum & 31u] = value;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 readFPCR() const noexcept { return fpcr; }
    AXP_HOT AXP_ALWAYS_INLINE void writeFPCR(quint64 value) noexcept { fpcr = value; }

    AXP_HOT AXP_ALWAYS_INLINE quint64* raw() noexcept { return f; }
    AXP_HOT AXP_ALWAYS_INLINE const quint64* raw() const noexcept { return f; }
};


// ============================================================================
// IPRStorage_PalShadow — PAL Shadow Register Banks
// CPU hardware — NOT saved/restored by SWPCTX.
//   SDE<0>: 27 registers (R8-R11, R24-R27 mapping)
//   SDE<1>: 23 registers (R4-R7, R20-R23 mapping)
// ============================================================================
struct alignas(64) IPRStorage_PalShadow final
{
    quint64 bank0[27]{};
    quint64 bank1[23]{};
    bool    enabled{ false };
    quint8  pad[7]{};

    AXP_HOT AXP_ALWAYS_INLINE void clear() noexcept
    {
        std::memset(bank0, 0, sizeof(bank0));
        std::memset(bank1, 0, sizeof(bank1));
        enabled = false;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 readBank0(quint8 idx) const noexcept
    {
        return (idx < 27) ? bank0[idx] : 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void writeBank0(quint8 idx, quint64 value) noexcept
    {
        if (idx < 27) bank0[idx] = value;
    }
    AXP_HOT AXP_ALWAYS_INLINE quint64 readBank1(quint8 idx) const noexcept
    {
        return (idx < 23) ? bank1[idx] : 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void writeBank1(quint8 idx, quint64 value) noexcept
    {
        if (idx < 23) bank1[idx] = value;
    }
};



// ============================================================================
// HWPCB Structure — Cache-Line Optimized Internal Layout
// ============================================================================
//
//  CACHE LINE 0 (0-63): Per-instruction hot path
//  ┌────────┬────────┬────────┬──────────┐
//  │  pc(0) │  ps(8) │ptbr(16)│exc_addr24│
//  ├──┬──┬──┼──┬──┬──┼────────┼──────────┤
//  │cm│ip│vm│as│fn│pd│va_fault│  pcc(48) │
//  │32│33│34│35│36│37│  (40)  │          │
//  ├──┴──┴──┴──┴──┴──┼────────┼──────────┤
//  │                 │        │  ksp(56) │
//  └─────────────────┴────────┴──────────┘
//
//  CACHE LINE 1 (64-127): PAL / context switch
//  ┌────────┬────────┬────────┬──────────┐
//  │ esp(64)│ ssp(72)│ usp(80)│  unq(88) │
//  ├────────┼────────┼──┬──┬──┼──────────┤
//  │datfx96 │        │at│as│pd│ fpe(112) │
//  │        │        │104 105 │          │
//  ├────────┼────────┼──┴──┴──┼──────────┤
//  │        │        │        │ ppce(120)│
//  └────────┴────────┴────────┴──────────┘
//
//  Stack pointer array: &ksp + (mode & 3) -> ksp/esp/ssp/usp
// ============================================================================
struct alignas(64) HWPCB
{
    // ================================================================
    // CACHE LINE 0: Per-instruction hot path (64 bytes)
    // ================================================================
    quint64 pc{};           //  0: Program Counter (bit[0] = PAL mode)
    quint64 ps{};           //  8: Processor Status (raw)
    quint64 ptbr{};         // 16: Page Table Base Register
    quint64 exc_addr{};     // 24: Exception Address

    quint8  cm{};           // 32: Current Mode (PS[1:0])
    quint8  ipl{};          // 33: IPL (PS[7:3])
    quint8  vmm{};          // 34: VMM flag (PS[8])
    quint8  asn{};          // 35: Address Space Number (0-255)
    quint8  fen{};          // 36: Floating-point Enable (0 or 1)
    quint8  pad0[3]{};      // 37-39: align to 8

    quint64 va_fault{};     // 40: Faulting VA (sign-extended)
    quint64 pcc{};          // 48: Process Cycle Counter offset
    quint64 ksp{};          // 56: Kernel SP (contiguous with esp)

    // ================================================================
    // CACHE LINE 1: PAL / context switch path (64 bytes)
    // ================================================================
    quint64 esp{};          // 64: Executive SP
    quint64 ssp{};          // 72: Supervisor SP
    quint64 usp{};          // 80: User SP
    quint64 unq{};          // 88: Unique Processor Value
    quint64 datfx{};        // 96: Data Alignment Trap Fixup
    quint8  aster{};        //104: AST Enable  (4 bits: K/E/S/U)
    quint8  astsr{};        //105: AST Summary (4 bits: K/E/S/U)
    quint64 fpe{};          //112: FP Exceptions Enable
    quint64 ppce{};         //120: Process Perf Counter Enable
    quint16 sisr{ 0 };    // Software Interrupt Summary Register (bits 15:1, bit 0 unused)
    quint16 sirr{ 0 };
    // ================================================================
    // CACHE LINE 2+: Cold path
    // ================================================================
    quint64 PALScratch[6]{};  //128-175: PAL-private scratch
    char processorSerial[16]{}; //176-191: 10-char ASCII + pad

    // ================================================================
    // RESET
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE void reset() noexcept
    {
        pc = 0; ps = 0; ptbr = 0; exc_addr = 0;
        cm = 0; ipl = 0; vmm = 0; asn = 0; fen = 0;
        va_fault = 0; pcc = 0; ksp = 0;
        esp = 0; ssp = 0; usp = 0; unq = 0; datfx = 0;
        aster = 0; astsr = 0;
        fpe = 0; ppce = 0; sisr = 0; sirr = 0;
        std::memset(PALScratch, 0, sizeof(PALScratch));
        std::memset(processorSerial, 0, sizeof(processorSerial));
    }

    AXP_HOT AXP_ALWAYS_INLINE void advancePC(quint64 newPC) noexcept
    {
        pc = (newPC & ~1ULL) | (pc & 1ULL);  // preserve PAL bit
    }

    // ================================================================
    // VA SIGN EXTENSION (43-bit -> 64-bit)
    // ================================================================
    static constexpr int VA_IMPL_BITS = 43;

    static AXP_HOT AXP_ALWAYS_INLINE
        quint64 sextVA(quint64 va) noexcept
    {
        constexpr quint64 signBit = 1ULL << (VA_IMPL_BITS - 1);
        constexpr quint64 mask = (1ULL << VA_IMPL_BITS) - 1;
        return ((va & mask) ^ signBit) - signBit;
    }

    AXP_HOT AXP_ALWAYS_INLINE void setVA_Fault(quint64 va) noexcept
    {
        va_fault = sextVA(va);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 getVA_Fault() const noexcept
    {
        return va_fault;
    }

    // PS<IV> - Integer Overflow Trap Enable
    // When set, integer arithmetic instructions with /V
    // generate an arithmetic exception on overflow.
    static constexpr quint64 PS_IV_BIT = (1ULL << 6);
    /**
    * @brief Check if integer overflow trap is enabled
    * @return true if PS.IV bit is set
    *
    * Alpha PS register bit layout:
    *   Bit 31: IV (Integer oVerflow trap enable)
    */
    AXP_HOT AXP_ALWAYS_INLINE  bool isIntegerOverflowTrapEnabled() const noexcept {
        return (ps & PS_IV_BIT) != 0;
    }

    /**
     * @brief Enable/disable integer overflow trap
     * @param enable true to enable IV trap
     */
    AXP_ALWAYS_INLINE  void setIntegerOverflowTrapEnable(bool enable) noexcept {
        if (enable) {
            ps |= PS_IV_BIT;
        }
        else {
            ps &= ~PS_IV_BIT;
        }
    }



    // ================================================================
    // STACK POINTER HELPERS — Array-indexed, branchless
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE quint64* spSlot(quint8 mode) noexcept
    {
        static_assert(offsetof(HWPCB, esp) == offsetof(HWPCB, ksp) + 8, "ksp->esp contiguous");
        static_assert(offsetof(HWPCB, ssp) == offsetof(HWPCB, ksp) + 16, "ksp->ssp contiguous");
        static_assert(offsetof(HWPCB, usp) == offsetof(HWPCB, ksp) + 24, "ksp->usp contiguous");
        return (&ksp) + (mode & 0x3);
    }

    AXP_HOT AXP_ALWAYS_INLINE void saveSP(quint8 mode, quint64 r30) noexcept
    {
        *spSlot(mode) = r30;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 loadSP(quint8 mode) const noexcept
    {
        return const_cast<HWPCB*>(this)->spSlot(mode)[0];
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 swapSP(quint8 oldMode, quint8 newMode, quint64 r30) noexcept
    {
        saveSP(oldMode, r30);
        return loadSP(newMode);
    }

    // ================================================================
    // PROCESSOR STATUS (PS) HELPERS
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPS() const noexcept { return ps; }

    AXP_HOT AXP_ALWAYS_INLINE void setPS(quint64 value) noexcept
    {
        ps = value;
        cm = static_cast<quint8>(value & 0x3);
        ipl = static_cast<quint8>((value >> 3) & 0x1F);
        vmm = static_cast<quint8>((value >> 8) & 0x1);
    }

    AXP_HOT AXP_ALWAYS_INLINE void setCM(quint8 mode) noexcept
    {
        ps = (ps & ~0x3ULL) | (mode & 0x3);
        cm = mode & 0x3;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint8 getCM() const noexcept { return cm; }
    AXP_HOT AXP_ALWAYS_INLINE quint8 getIPL() const noexcept { return ipl; }

    // IPL setter — caller MUST sync IRQController separately
    AXP_HOT AXP_ALWAYS_INLINE void setIPL_Unsynced(quint8 ipl_) noexcept
    {
        ps = (ps & ~0xF8ULL) | ((ipl_ & 0x1F) << 3);
        ipl = ipl_ & 0x1F;
    }

    // ================================================================
    // PROGRAM COUNTER HELPERS
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPC() const noexcept { return pc; }
    AXP_HOT AXP_ALWAYS_INLINE void setPC(quint64 v) noexcept { pc = v; }
    AXP_HOT AXP_ALWAYS_INLINE void forcePalPC(quint64 v) noexcept { pc = v | 0x1ULL; }
    AXP_HOT AXP_ALWAYS_INLINE void forceUserPC(quint64 v) noexcept { pc = v & ~0x1ULL; }
    AXP_HOT AXP_ALWAYS_INLINE bool inPalMode() const noexcept { return (pc & 0x1) != 0; }

    // ================================================================
    // AST PACK/UNPACK (Physical HWPCB offset 0x30)
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE quint64 packAstSrEn() const noexcept
    {
        return static_cast<quint64>(astsr & 0x0F)
            | (static_cast<quint64>(aster & 0x0F) << 4);
    }

    AXP_HOT AXP_ALWAYS_INLINE void unpackAstSrEn(quint64 packed) noexcept
    {
        astsr = static_cast<quint8>(packed & 0x0F);
        aster = static_cast<quint8>((packed >> 4) & 0x0F);
    }

    // ================================================================
    // PCC SAVE/RESTORE
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE quint64 savePCC(quint64 hwCounter) const noexcept
    {
        quint32 hw = static_cast<quint32>(hwCounter & 0xFFFFFFFF);
        quint32 off = static_cast<quint32>(pcc & 0xFFFFFFFF);
        return static_cast<quint64>((hw + off) & 0xFFFFFFFF);
    }

    AXP_HOT AXP_ALWAYS_INLINE void restorePCC(quint64 stored, quint64 hwCounter) noexcept
    {
        quint32 s = static_cast<quint32>(stored & 0xFFFFFFFF);
        quint32 hw = static_cast<quint32>(hwCounter & 0xFFFFFFFF);
        pcc = static_cast<quint64>((s - hw) & 0xFFFFFFFF);
    }

    // ================================================================
    // SERIAL NUMBER
    // ================================================================
    AXP_HOT AXP_ALWAYS_INLINE QString getProcessorSerialString() const noexcept
    {
        char buf[11];
        std::memcpy(buf, processorSerial, 10);
        buf[10] = '\0';
        return QString::fromLatin1(buf);
    }

    AXP_HOT AXP_ALWAYS_INLINE void setProcessorSerialString(const QString& serial) noexcept
    {
        auto latin = serial.toLatin1();
        const int count = qMin(latin.size(), 10);
        std::memcpy(processorSerial, latin.constData(), count);
        if (count < 10) std::memset(processorSerial + count, ' ', 10 - count);
        std::memset(processorSerial + 10, 0, 6);
    }
};


// ############################################################################
//          SECTION 3: RUN-LOOP IPRs (Deduplicated from Hot64)
// ############################################################################
// Only fields NOT already in HWPCB or FloatRegs.
// Removed: ps, ipl, cm, asn, fpcr, pal_mode (all duplicates)
// ############################################################################


// PCC state machine
struct PccState64 final
{
    quint32 pccOff{ 0 };
    quint32 pccCnt{ 0 };
    quint8  pccDivN{ 1 };
    bool    rpccForceZero{ false };
    quint8  pccFrac{ 0 };
    quint8  pad{ 0 };
    quint64 lastSysCC{ 0 };
};

AXP_HOT AXP_ALWAYS_INLINE quint8 clampPccDivN(quint8 n) noexcept
{
    if (n < 1)  return 1;
    if (n > 16) return 16;
    return n;
}

AXP_HOT AXP_ALWAYS_INLINE void pccAdvanceFromSysCC(PccState64& pcc, quint64 sysCC_now) noexcept
{
    quint64 delta = sysCC_now - pcc.lastSysCC;
    pcc.lastSysCC = sysCC_now;
    if (delta == 0) return;
    const quint8 N = clampPccDivN(pcc.pccDivN);
    quint64 total = static_cast<quint64>(pcc.pccFrac) + delta;
    pcc.pccCnt = static_cast<quint32>(pcc.pccCnt + static_cast<quint32>(total / N));
    pcc.pccFrac = static_cast<quint8>(total % N);
}

AXP_HOT AXP_ALWAYS_INLINE quint64 pccRead64(PccState64& pcc, quint64 sysCC_now) noexcept
{
    if (pcc.rpccForceZero) return 0;
    pccAdvanceFromSysCC(pcc, sysCC_now);
    return (static_cast<quint64>(pcc.pccOff) << 32) | static_cast<quint64>(pcc.pccCnt);
}


// ============================================================================
// IPRStorage_RunLoop — Per-instruction cycle state (deduplicated)
// ============================================================================
// Contains ONLY what is not already stored in HWPCB or FloatRegs.
// Hot64 fields ps/ipl/cm/asn/fpcr/pal_mode were all duplicates — removed.
// ============================================================================
struct alignas(64) IPRStorage_RunLoop final
{
    quint64 cc{};                     //  0: System cycle counter
    quint64 cc_ctl{};                 //  8: CC_CTL — bit 0 enables, bit 1 freezes in PAL
    PccState64 pccState{};            // 16: Process cycle counter state (24 bytes)
    quint8  intrFlag{ 0 };              // 40: Per-CPU interrupt flag
    quint8  pal_personality{ 0 };       // 41: 0=OpenVMS, 1=Unix/Tru64
    quint8  halt_code{ 0 };             // 42: Halt reason (0=running)
    bool    halted{ false };            // 43: CPU halted flag
    quint8  pad[20]{};                // 44-63: pad to cache line

    AXP_HOT AXP_ALWAYS_INLINE void reset() noexcept
    {
        cc = 0;
        cc_ctl = 0;
        pccState = PccState64{};
        intrFlag = 0;
        pal_personality = 0;
        halt_code = 0;
        halted = false;
    }

    AXP_HOT AXP_ALWAYS_INLINE void haltCPU(quint8 code) noexcept
    {
        halt_code = code;
        halted = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE void clearHalt() noexcept
    {
        halt_code = 0;
        halted = false;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool isHalted() const noexcept
    {
        return halted;
    }
};

static_assert(sizeof(IPRStorage_RunLoop) == 64,
    "RunLoop must be exactly 64 bytes (one cache line)");


// ############################################################################
//       SECTION 4: PAL IPRs (Deduplicated from HotExt)
// ############################################################################
// Removed: exc_addr, ptbr, va, ksp/esp/ssp/usp, asten (all in HWPCB)
// Removed: duplicate hwpcb field (use pcbb only)
// ############################################################################


// Write buffer for deferred IPR commits
struct IPRWriteBuffer
{
    std::atomic<bool> hasPendingCC{ false };
    std::atomic<bool> hasPendingPerfCounters{ false };
    std::atomic<bool> hasPendingTLBStaging{ false };
    std::atomic<bool> hasPendingExceptionState{ false };

    inline void reset() noexcept
    {
        hasPendingCC.store(false, std::memory_order_relaxed);
        hasPendingPerfCounters.store(false, std::memory_order_relaxed);
        hasPendingTLBStaging.store(false, std::memory_order_relaxed);
        hasPendingExceptionState.store(false, std::memory_order_relaxed);
    }
};

// TLB staging scratch
struct DTBTagScratch {
    union {
        quint64 raw{ 0 };
        struct {
            quint64 vpn : 43;
            quint64 asn : 8;
            quint64 gh : 2;
            quint64 bank1 : 1;
            quint64 reserved : 10;
        };
    };
};

struct ITBTagScratch {
    quint64 va{ 0 };
    quint8  asn{ 0 };
    quint8  pad[7]{};
};


// ============================================================================
// IPRStorage_PalIPR — Exception / PAL path storage (deduplicated)
// ============================================================================
struct alignas(64) IPRStorage_PalIPR
{
    CPUIdType m_cpuId{};

    // ── System / PAL base addresses ──
    quint64 vptb{};             // Virtual Page Table Base
    quint64 pal_base{};         // PAL code base address
    quint64 scbb{};             // System Control Block Base
    quint64 pcbb{};             // Process Control Block Base (physical)
    quint64 prbr{};             // Processor Base Register
    quint64 virbnd{};           // Virtual Address Boundary (superpage)
    quint64 sysptbr{};          // System Page Table Base Register
    quint64 mces{};             // Machine Check Error Summary
    quint64 whami{};            // Who Am I (CPU ID)

    // ── Box control registers ──
    quint64 iccsr{};            // Ibox Control/Status (EV5 compat)
    quint64 i_ctl{};            // I-box Control (EV6)
    quint64 m_ctl{};            // M-box Control
    quint64 dc_ctl{};           // Dcache Control
    quint64 va_ctl{};           // VA Control

    // ── Exception state ──
    quint64 exc_sum{};          // Exception Summary
    quint64 exc_mask{};         // Exception Mask
    quint64 mm_stat{};          // Memory Management Status

    // ── TLB staging (write-only IPRs) ──
    DTBTagScratch dtbTagScratch{};
    ITBTagScratch itbTagScratch{};
    quint64 dtb_pte_temp{};
    quint64 itb_pte_temp{};
    quint64 dtb_is{};
    quint64 itb_is{};
    quint64 dtb_ia{};
    quint64 dtb_iap{};
    quint64 dtb_asn{};
    quint64 dtb_tag{};
    quint64 dtb_pte{};
    quint64 itb_tag{};
    quint64 itb_pte{};
    quint64 itb_asn{};
    quint64 itb_ia{};
    quint64 itb_iap{};

    // ── Performance monitoring ──
    quint64 perfmon{};

    // ── Write buffer ──
    mutable IPRWriteBuffer m_writeBuffer{};
    mutable std::atomic<bool> m_hasPendingMemoryOrderingWrites{ false };
    mutable std::atomic<quint32> m_memoryOrderingMask{ 0 };
    quint64 va{};       // Virtual Address
    // ── PAL temporaries ──
    quint64 pal_temp[32]{};

    // ── Accessors ──

   
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPalTemp(int idx) const noexcept
    {
        return (idx >= 0 && idx < 32) ? pal_temp[idx] : 0;
    }
    AXP_HOT AXP_ALWAYS_INLINE void setPalTemp(int idx, quint64 value) noexcept
    {
        if (idx >= 0 && idx < 32) pal_temp[idx] = value;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingWrites() const noexcept
    {
        return m_writeBuffer.hasPendingCC.load(std::memory_order_relaxed)
            || m_writeBuffer.hasPendingPerfCounters.load(std::memory_order_relaxed)
            || m_writeBuffer.hasPendingTLBStaging.load(std::memory_order_relaxed)
            || m_writeBuffer.hasPendingExceptionState.load(std::memory_order_relaxed);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingMemoryOrderingWrites() const noexcept
    {
        return m_hasPendingMemoryOrderingWrites.load(std::memory_order_acquire);
    }

    AXP_ALWAYS_INLINE void completePendingMemoryOrderingWrites() noexcept
    {
        if (m_hasPendingMemoryOrderingWrites.exchange(false, std::memory_order_acq_rel)) {
            quint32 mask = m_memoryOrderingMask.exchange(0, std::memory_order_acq_rel);
            (void)mask; // TODO: flush specific IPRs based on mask bits
        }
    }

    AXP_ALWAYS_INLINE void bufferedWriteTLB() noexcept
    {
        m_writeBuffer.hasPendingTLBStaging.store(true, std::memory_order_release);
    }

    AXP_ALWAYS_INLINE void bufferedWriteCC() noexcept
    {
        m_writeBuffer.hasPendingCC.store(true, std::memory_order_release);
    }

    // flushPendingWrites() defined externally (requires TLB headers)
    // Signature: void flushPendingWrites(IPRStorage_PalIPR& ipr, CPUIdType cpuId)

    inline void reset() noexcept
    {
        vptb = 0; pal_base = 0; scbb = 0; pcbb = 0; prbr = 0;
        virbnd = 0; sysptbr = 0; mces = 0; whami = 0;
        iccsr = 0; i_ctl = 0; m_ctl = 0; dc_ctl = 0; va_ctl = 0;
        exc_sum = 0; exc_mask = 0; mm_stat = 0;
        dtbTagScratch.raw = 0;
        itbTagScratch = ITBTagScratch{};
        dtb_pte_temp = 0; itb_pte_temp = 0;
        dtb_is = 0; itb_is = 0; dtb_ia = 0; dtb_iap = 0; dtb_asn = 0;
        dtb_tag = 0; dtb_pte = 0;
        itb_tag = 0; itb_pte = 0; itb_asn = 0; itb_ia = 0; itb_iap = 0;
        perfmon = 0;
        std::memset(pal_temp, 0, sizeof(pal_temp));
        m_writeBuffer.reset();
        m_hasPendingMemoryOrderingWrites.store(false, std::memory_order_relaxed);
        m_memoryOrderingMask.store(0, std::memory_order_relaxed);
    }
};


// ############################################################################
//    SECTION 4b: PAL PERSONALITY IPRs (OSF/Tru64 Unix)
// ############################################################################
// These IPRs are defined by the PAL personality, not the silicon.
// OSF/1 (Tru64 Unix) PALcode defines entry vectors and kernel GP.
// OpenVMS would define a different set. Only one personality is
// active per CPU at a time (selected by pal_personality in RunLoop).
//
// Single-writer: CPU run loop only (CALL_PAL WRENT, etc.)
// ############################################################################

struct alignas(64) IPRStorage_OSF final
{
    // ── OSF/1 Virtual Page Table Pointer ──
    quint64 vptptr{};               //  0: VA of page table pointer (OSF-specific)

    // ── WRENT entry vectors (set by CALL_PAL WRENT) ──
    quint64 ent_int{};              //  8: Interrupt entry point
    quint64 ent_arith{};            // 16: Arithmetic exception entry
    quint64 ent_mm{};               // 24: Memory management fault entry
    quint64 ent_fault{};            // 32: General fault entry (IF/OPCDEC)
    quint64 ent_una{};              // 40: Unaligned access entry
    quint64 ent_sys{};              // 48: System call (CALL_PAL callsys) entry

    // ── Kernel global pointer ──
    quint64 wrkgp{};                // 56: Written by CALL_PAL WRKGP

    // TOTAL: 64 bytes (1 cache line)

    AXP_HOT AXP_ALWAYS_INLINE void reset() noexcept
    {
        vptptr = 0;
        ent_int = 0;
        ent_arith = 0;
        ent_mm = 0;
        ent_fault = 0;
        ent_una = 0;
        ent_sys = 0;
        wrkgp = 0;
    }
};

static_assert(sizeof(IPRStorage_OSF) == 64,
    "OSF must be exactly 64 bytes (1 cache line)");
static_assert(alignof(IPRStorage_OSF) == 64,
    "OSF must be 64-byte aligned");


// ############################################################################
//               SECTION 5: GlobalCPUState SINGLETON
// ############################################################################

class GlobalCPUState final
{
public:
    GlobalCPUState() noexcept
    {
        m_cpuCount = 1;
        resetAll();
    }

    ~GlobalCPUState() noexcept = default;
    GlobalCPUState(const GlobalCPUState&) = delete;
    GlobalCPUState& operator=(const GlobalCPUState&) // should be a  NOOP
    {
        GlobalCPUState noOP;
        return noOP;
    }

    // ================================================================
    // CPU Count
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE void setCpuCount(quint32 count) noexcept
    {
        if (count == 0) count = 1;
        if (count > static_cast<quint32>(MAX_CPUS)) count = static_cast<quint32>(MAX_CPUS);
        m_cpuCount = count;
        resetAll();
    }


    void initialize(quint32 cpuCount) noexcept
    {
        setCpuCount(cpuCount);
        for (quint32 i = 0; i < m_cpuCount; ++i) {
            m_palIPR[i].m_cpuId = static_cast<CPUIdType>(i);
            m_palIPR[i].whami = static_cast<quint64>(i);
        }
    }
    AXP_HOT AXP_ALWAYS_INLINE quint32 cpuCount() const noexcept { return m_cpuCount; }

    // ================================================================
    // Per-CPU Accessors — Active State
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_IntRegs& intRegs(CPUIdType id) noexcept
    {
        return m_intRegs[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_IntRegs& intRegs(CPUIdType id) const noexcept
    {
        return m_intRegs[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_FloatRegs& floatRegs(CPUIdType id) noexcept
    {
        return m_floatRegs[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_FloatRegs& floatRegs(CPUIdType id) const noexcept
    {
        return m_floatRegs[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_PalShadow& palShadow(CPUIdType id) noexcept
    {
        return m_palShadow[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_PalShadow& palShadow(CPUIdType id) const noexcept
    {
        return m_palShadow[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE HWPCB& hwpcb(CPUIdType id) noexcept
    {
        return m_hwpcb[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const HWPCB& hwpcb(CPUIdType id) const noexcept
    {
        return m_hwpcb[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_RunLoop& runLoop(CPUIdType id) noexcept
    {
        return m_runLoop[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_RunLoop& runLoop(CPUIdType id) const noexcept
    {
        return m_runLoop[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_PalIPR& palIPR(CPUIdType id) noexcept
    {
        return m_palIPR[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_PalIPR& palIPR(CPUIdType id) const noexcept
    {
        return m_palIPR[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_OSF& osf(CPUIdType id) noexcept
    {
        return m_osf[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_OSF& osf(CPUIdType id) const noexcept
    {
        return m_osf[idx(id)];
    }

    // ================================================================
    // Per-CPU Accessors — Snapshot State
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_IntRegs& intRegs_Snap(CPUIdType id) noexcept
    {
        return m_intRegs_snap[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_IntRegs& intRegs_Snap(CPUIdType id) const noexcept
    {
        return m_intRegs_snap[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE IPRStorage_FloatRegs& floatRegs_Snap(CPUIdType id) noexcept
    {
        return m_floatRegs_snap[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPRStorage_FloatRegs& floatRegs_Snap(CPUIdType id) const noexcept
    {
        return m_floatRegs_snap[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE HWPCB& hwpcb_Snap(CPUIdType id) noexcept
    {
        return m_hwpcb_snap[idx(id)];
    }

    AXP_HOT AXP_ALWAYS_INLINE const HWPCB& hwpcb_Snap(CPUIdType id) const noexcept
    {
        return m_hwpcb_snap[idx(id)];
    }

    // ================================================================
    // Direct Register Read/Write (convenience — bypasses View)
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE quint64 readInt(CPUIdType id, quint8 r) const noexcept
    {
        return m_intRegs[idx(id)].read(r);
    }

    AXP_HOT AXP_ALWAYS_INLINE void writeInt(CPUIdType id, quint8 r, quint64 v) noexcept
    {
        m_intRegs[idx(id)].write(r, v);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 readFloat(CPUIdType id, quint8 r) const noexcept
    {
        return m_floatRegs[idx(id)].read(r);
    }

    AXP_HOT AXP_ALWAYS_INLINE void writeFloat(CPUIdType id, quint8 r, quint64 v) noexcept
    {
        m_floatRegs[idx(id)].write(r, v);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 readFPCR(CPUIdType id) const noexcept
    {
        return m_floatRegs[idx(id)].readFPCR();
    }

    AXP_HOT AXP_ALWAYS_INLINE void writeFPCR(CPUIdType id, quint64 v) noexcept
    {
        m_floatRegs[idx(id)].writeFPCR(v);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 readShadow(CPUIdType id, quint8 bank, quint8 i) const noexcept
    {
        const auto& s = m_palShadow[idx(id)];
        return (bank == 0) ? s.readBank0(i) : s.readBank1(i);
    }

    AXP_HOT AXP_ALWAYS_INLINE void writeShadow(CPUIdType id, quint8 bank, quint8 i, quint64 v) noexcept
    {
        auto& s = m_palShadow[idx(id)];
        (bank == 0) ? s.writeBank0(i, v) : s.writeBank1(i, v);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool isShadowEnabled(CPUIdType id) const noexcept
    {
        return m_palShadow[idx(id)].enabled;
    }

    AXP_HOT AXP_ALWAYS_INLINE void setShadowEnabled(CPUIdType id, bool e) noexcept
    {
        m_palShadow[idx(id)].enabled = e;
    }

   

    // ================================================================
    // Context Save / Restore — ALL snapshottable state
    // ================================================================
    // Saves: IntRegs, FloatRegs (incl FPCR), HWPCB
    // Does NOT save: PalShadow (CPU hw), RunLoop (cycle state), PalIPR
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE void saveContext(CPUIdType id) noexcept
    {
        const auto i = idx(id);
        std::memcpy(&m_intRegs_snap[i], &m_intRegs[i], sizeof(IPRStorage_IntRegs));
        std::memcpy(&m_floatRegs_snap[i], &m_floatRegs[i], sizeof(IPRStorage_FloatRegs));
        std::memcpy(&m_hwpcb_snap[i], &m_hwpcb[i], sizeof(HWPCB));
    }

    AXP_HOT AXP_ALWAYS_INLINE void restoreContext(CPUIdType id) noexcept
    {
        const auto i = idx(id);
        std::memcpy(&m_intRegs[i], &m_intRegs_snap[i], sizeof(IPRStorage_IntRegs));
        std::memcpy(&m_floatRegs[i], &m_floatRegs_snap[i], sizeof(IPRStorage_FloatRegs));
        std::memcpy(&m_hwpcb[i], &m_hwpcb_snap[i], sizeof(HWPCB));
    }

    // ================================================================
    // Reset
    // ================================================================

    AXP_HOT AXP_ALWAYS_INLINE void resetCPU(CPUIdType id) noexcept
    {
        const auto i = idx(id);
        m_intRegs[i].clear();
        m_floatRegs[i].clear();
        m_palShadow[i].clear();
        m_hwpcb[i].reset();
        m_runLoop[i].reset();
        m_palIPR[i].reset();
        m_osf[i].reset();
        m_intRegs_snap[i].clear();
        m_floatRegs_snap[i].clear();
        m_hwpcb_snap[i].reset();
    }

    AXP_HOT AXP_ALWAYS_INLINE void resetAll() noexcept
    {
        for (quint32 i = 0; i < m_cpuCount; ++i)
            resetCPU(static_cast<CPUIdType>(i));
    }

private:
    AXP_HOT AXP_ALWAYS_INLINE quint32 idx(CPUIdType id) const noexcept
    {
        return static_cast<quint32>(id) % static_cast<quint32>(MAX_CPUS);
    }

private:
    quint32 m_cpuCount{ 1 };

    // ── Active state (pipeline reads/writes) ──
    alignas(64) IPRStorage_IntRegs     m_intRegs[MAX_CPUS];
    alignas(64) IPRStorage_FloatRegs   m_floatRegs[MAX_CPUS];
    alignas(64) IPRStorage_PalShadow   m_palShadow[MAX_CPUS];
    alignas(64) HWPCB                  m_hwpcb[MAX_CPUS];
    alignas(64) IPRStorage_RunLoop     m_runLoop[MAX_CPUS];
    alignas(64) IPRStorage_PalIPR      m_palIPR[MAX_CPUS];
    alignas(64) IPRStorage_OSF         m_osf[MAX_CPUS];

    // ── Snapshot state (exception save/restore) ──
    alignas(64) IPRStorage_IntRegs     m_intRegs_snap[MAX_CPUS];
    alignas(64) IPRStorage_FloatRegs   m_floatRegs_snap[MAX_CPUS];
    alignas(64) HWPCB                  m_hwpcb_snap[MAX_CPUS];
    // No snapshot for: PalShadow (CPU hw), RunLoop (cycle), PalIPR (PAL state)
};



// ############################################################################
//              SECTION 6: CPUStateView (CPU-Bound Cached Pointers)
// ############################################################################
// Created once per CPU at init. Eliminates repeated singleton + idx lookups.
//
// Usage:
//   CPUStateView cpu;
//   cpu.bind(globalCPUState(), cpuId);
//   quint64 pc = cpu.h->getPC();
//   cpu.writeInt(16, value);
//   cpu.saveContext();
// ############################################################################

struct CPUStateView final
{
    CPUIdType               cpuId{};
    GlobalCPUState* state{ nullptr };
    IPRStorage_IntRegs* i{ nullptr };     // integer registers
    IPRStorage_FloatRegs* f{ nullptr };     // float registers + FPCR
    IPRStorage_PalShadow* p{ nullptr };     // PAL shadow banks
    HWPCB* h{ nullptr };     // process control block
    IPRStorage_RunLoop* r{ nullptr };     // run-loop IPRs (cc, pcc)
    IPRStorage_PalIPR* x{ nullptr };     // PAL/exception IPRs
    IPRStorage_OSF* o{ nullptr };     // OSF personality IPRs

    AXP_HOT AXP_ALWAYS_INLINE
        void bind(GlobalCPUState& s, CPUIdType id) noexcept
    {
        state = &s;
        cpuId = id;
        i = &s.intRegs(id);
        f = &s.floatRegs(id);
        p = &s.palShadow(id);
        h = &s.hwpcb(id);
        r = &s.runLoop(id);
        x = &s.palIPR(id);
        o = &s.osf(id);
    }

    // ── Integer Registers ──
    AXP_HOT AXP_ALWAYS_INLINE quint64 readInt(quint8 rn) const noexcept { return i->read(rn); }
    AXP_HOT AXP_ALWAYS_INLINE void writeInt(quint8 rn, quint64 v) noexcept { i->write(rn, v); }

    // ── Float Registers ──
    AXP_HOT AXP_ALWAYS_INLINE quint64 readFloat(quint8 fn) const noexcept { return f->read(fn); }
    AXP_HOT AXP_ALWAYS_INLINE void writeFloat(quint8 fn, quint64 v) noexcept { f->write(fn, v); }

    // ── FPCR ──
    AXP_HOT AXP_ALWAYS_INLINE quint64 readFPCR() const noexcept { return f->readFPCR(); }
    AXP_HOT AXP_ALWAYS_INLINE void writeFPCR(quint64 v) noexcept { f->writeFPCR(v); }

    // ── PAL Shadow Banks ──
    AXP_HOT AXP_ALWAYS_INLINE
        quint64 readBank(quint8 bank, quint8 rn) const noexcept
    {
        return (bank == 0) ? p->readBank0(rn) : p->readBank1(rn);
    }
    AXP_HOT AXP_ALWAYS_INLINE
        void writeBank(quint8 bank, quint8 rn, quint64 v) noexcept
    {
        (bank == 0) ? p->writeBank0(rn, v) : p->writeBank1(rn, v);
    }
    AXP_HOT AXP_ALWAYS_INLINE bool isShadowEnabled() const noexcept { return p->enabled; }
    AXP_HOT AXP_ALWAYS_INLINE void setShadowEnabled(bool e) noexcept { p->enabled = e; }

    // ── HWPCB shortcuts (most common fields) ──
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPC() const noexcept { return h->getPC(); }
    AXP_HOT AXP_ALWAYS_INLINE void setPC(quint64 v) noexcept { h->setPC(v); }
    AXP_HOT AXP_ALWAYS_INLINE quint8 getCM() const noexcept { return h->getCM(); }
    AXP_HOT AXP_ALWAYS_INLINE quint8 getIPL() const noexcept { return h->getIPL(); }
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPS() const noexcept { return h->getPS(); }
    AXP_HOT AXP_ALWAYS_INLINE void setPS(quint64 v) noexcept { h->setPS(v); }

    // ── Context Save/Restore (delegates to singleton) ──
    AXP_HOT AXP_ALWAYS_INLINE void saveContext() noexcept { state->saveContext(cpuId); }
    AXP_HOT AXP_ALWAYS_INLINE void restoreContext() noexcept { state->restoreContext(cpuId); }

    // ── Raw pointer access (memcpy, DMA, debug) ──
    AXP_HOT AXP_ALWAYS_INLINE quint64* intRaw() noexcept { return i->raw(); }
    AXP_HOT AXP_ALWAYS_INLINE const quint64* intRaw() const noexcept { return i->raw(); }
    AXP_HOT AXP_ALWAYS_INLINE quint64* floatRaw() noexcept { return f->raw(); }
    AXP_HOT AXP_ALWAYS_INLINE const quint64* floatRaw() const noexcept { return f->raw(); }

    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE bool isInPalMode() const noexcept {
        return (h->pc & 0x1) != 0;
    }
    // Set or clear PC[0] while preserving all other bits.
    AXP_HOT AXP_ALWAYS_INLINE quint64 setPalMode( bool enable) noexcept
    {
        if (enable) {
            return (h->pc | 0x1ULL);   // set bit0
        }
        return (h->pc & ~0x1ULL);      // clear bit0
    }

    // Before attempting TLB lookup
    AXP_HOT AXP_ALWAYS_INLINE bool isPhysicalMode() const noexcept {
        const quint64 vaCtl = x->va_ctl;
        return (vaCtl & 0x2) == 0;  // Bit 1 = VA_MODE
    }


    static AXP_HOT AXP_ALWAYS_INLINE bool isKseg(quint64 va) noexcept {
        return va >= 0xFFFFFC0000000000ULL;
    }

    static AXP_HOT AXP_ALWAYS_INLINE bool isPhysicalSegment(quint64 va) noexcept {
        return va < 0x100000000ULL;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto isIllegalCallPal(const quint8 func) const noexcept -> bool
    {
        // Condition 1: Reserved unprivileged range 0x40-0x7F
        if (func >= 0x40 && func <= 0x7F)
        {
            return true;
        }

        // Condition 2: Beyond valid range (> 0xBF)
        if (func > 0xBF)
        {
            return true;
        }

        // Condition 3: Privileged CALL_PAL (0x00-0x3F) when not in kernel mode
        if (func <= 0x3F)
        {
            // Check IER_CM[CM] - current mode field
            // IER_CM[CM] bits [1:0] indicate current mode:
            // 0 = Kernel mode
            // 1 = Executive mode
            // 2 = Supervisor mode
            // 3 = User mode
            const CMType currentMode = getCM();

            // If not in kernel mode (CM != 0), privileged CALL_PAL is illegal
            if (currentMode != 0)
            {
                return true;
            }
        }

        // All checks passed - legal CALL_PAL
        return false;
    }


  

    AXP_HOT AXP_ALWAYS_INLINE auto computeExceptionVector( PalVectorId_EV6 PVectorId) const noexcept -> quint64
    {
        const quint64 palBase = x->pal_base;
        quint16       vectorId = static_cast<quint16>(PVectorId);
        quint64       pc = palBase & ~0x7FFFULL;
        pc |= (vectorId & 0x7FFE);
        pc |= 0x1ULL;
        return pc;
    }

    AXP_HOT AXP_ALWAYS_INLINE auto computeCallPalEntry(const quint32 func) const noexcept -> quint64
    {
        if (isIllegalCallPal(func))
        {
            return computeExceptionVector(PalVectorId_EV6::OPCDEC);
        }
        const quint64 palBase = x->pal_base;
        quint64       pc = palBase & ~0x7FFFULL;
        pc |= (1ULL << 13);
        pc |= (static_cast<quint64>(func >> 7) << 12);
        pc |= (static_cast<quint64>(func & 0x3F) << 6);
        pc |= 0x1ULL;
        return pc;
    }

    // Context Swap
    /*
        SWPCTX hot path cache line usage:
        h-> pointer           — already in register from PAL entry
        h->ksp/esp/ssp/usp   — HWPCB cache line 1 (single line, 4 stores)
        h->pc/ps/cm/asn/ptbr  — HWPCB cache line 0 (already hot from PAL entry)
        hwLoad/hwStore         — GuestMemory::readPA/writePA (cold, but unavoidable)
    */

    AXP_HOT inline SwapContextResult hwpcbSwapContext(
        quint64     oldPCBB_PA,
        quint64     newPCBB_PA,
        GuestMemory* guestMem,
        quint64     hwCycleCounter,
        quint64     currentR30) noexcept
    {
        using namespace HWPCBPhysical;
        using namespace HWPCBLayout;

        SwapContextResult result{};
        result.oldPCBB = oldPCBB_PA;
        result.success = true;

        // ================================================================
        // 1. ALIGNMENT CHECK
        // ================================================================
        // R16<6:0> must be zero (128-byte aligned)
        if (newPCBB_PA & ALIGNMENT_MASK) {
            result.success = false;
            return result;
        }

        // ================================================================
        // 2. SAVE CURRENT STATE -> OLD HWPCB (physical memory)
        // ================================================================
        // Save R30 into the correct mode-specific slot first
        h->saveSP(h->cm, currentR30);

        // Stack pointers
        hwStore(guestMem, oldPCBB_PA + KSP, h->ksp);
        hwStore(guestMem, oldPCBB_PA + ESP, h->esp);
        hwStore(guestMem, oldPCBB_PA + SSP, h->ssp);
        hwStore(guestMem, oldPCBB_PA + USP, h->usp);

        // AST state (packed: ASTSR<3:0> | ASTEN<7:4>)
        hwStore(guestMem, oldPCBB_PA + ASTSR_EN, h->packAstSrEn());

        // Process cycle counter (accumulated value)
        hwStore(guestMem, oldPCBB_PA + PCC, h->savePCC(hwCycleCounter));

        // Process unique value
        hwStore(guestMem, oldPCBB_PA + UNQ, h->unq);

        hwStore(guestMem, oldPCBB_PA + FEN, h->fen);
        hwStore(guestMem, oldPCBB_PA + DAT, h->datfx);

        // Note: PTBR and ASN are NOT saved -- they are already in the
        // old HWPCB from when they were last loaded. Only mutable
        // per-process state is written back.

        // ================================================================
        // 3. SNAPSHOT OLD STATE FOR TLB DECISION
        // ================================================================
        const quint64 oldPTBR = h->ptbr;
        const quint8  oldASN = h->asn;

        // ================================================================
        // 4. LOAD NEW STATE <- NEW HWPCB (physical memory)
        // ================================================================
        // Stack pointers
        h->ksp = hwLoad(guestMem, newPCBB_PA + KSP);
        h->esp = hwLoad(guestMem, newPCBB_PA + ESP);
        h->ssp = hwLoad(guestMem, newPCBB_PA + SSP);
        h->usp = hwLoad(guestMem, newPCBB_PA + USP);


        // Address translation
        h->ptbr = hwLoad(guestMem, newPCBB_PA + PTBR);
        h->asn = static_cast<quint8>(hwLoad(guestMem, newPCBB_PA + ASN) & 0xFF);

        // AST state (unpack from single quadword)
        h->unpackAstSrEn(hwLoad(guestMem, newPCBB_PA + ASTSR_EN));

        // Floating-point enable (bit 0 only)
        h->fen = static_cast<quint8>(hwLoad(guestMem, newPCBB_PA + FEN) & 0x1);

        // Process cycle counter (restore offset relative to hardware counter)
        h->restorePCC(hwLoad(guestMem, newPCBB_PA + PCC), hwCycleCounter);

        // Process unique value
        h->unq = hwLoad(guestMem, newPCBB_PA + UNQ);

        // Data alignment trap enable
        h->datfx = hwLoad(guestMem, newPCBB_PA + DAT);

        // ================================================================
        // 5. TLB UPDATE — EV6 implements ASNs
        // ================================================================
        // EV6 TLB entries are tagged with ASN. On context switch, loading
        // the new ASN is sufficient — old entries won't match lookups.
        // No explicit TLB invalidation occurs during SWPCTX.
        //
        // (Non-ASN implementations would flush non-ASM entries when PTBR
        // changes, but EV6 never takes that path.)
        // ================================================================
        result.ptbrChanged = (h->ptbr != oldPTBR);
        result.asnChanged = (h->asn != oldASN);

        // No TLB flush — ASN tagging handles isolation

        return result;
    }
};



// ============================================================================
// Singleton Accessor
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE
GlobalCPUState& globalCPUState() noexcept
{
    static GlobalCPUState s_instance;
    return s_instance;
}

// ============================================================================
// Global Convenience — Create CPU-Bound View
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE
CPUStateView globalCPUView(CPUIdType cpuId) noexcept
{
    CPUStateView v;
    v.bind(globalCPUState(), cpuId);
    return v;
}


// ============================================================================
// Legacy / Migration Convenience Accessors
// ============================================================================
// These bridge old call sites. New code should use globalCPUState() or
// CPUStateView directly.
// ============================================================================

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_IntRegs& globalIntRegs(CPUIdType id) noexcept
{
    return globalCPUState().intRegs(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_FloatRegs& globalFloatRegs(CPUIdType id) noexcept
{
    return globalCPUState().floatRegs(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_PalShadow& globalPalShadow(CPUIdType id) noexcept
{
    return globalCPUState().palShadow(id);
}

AXP_HOT AXP_ALWAYS_INLINE HWPCB& globalHWPCB(CPUIdType id) noexcept
{
    return globalCPUState().hwpcb(id);
}

AXP_HOT AXP_ALWAYS_INLINE HWPCB& globalHWPCBController(CPUIdType id) noexcept
{
    return globalCPUState().hwpcb(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_RunLoop& globalRunLoop(CPUIdType id) noexcept
{
    return globalCPUState().runLoop(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_PalIPR& globalPalIPR(CPUIdType id) noexcept
{
    return globalCPUState().palIPR(id);
}

// Bridge for old globalIPRHot(cpuId) and globalIPRHotExt(cpuId) call sites
AXP_HOT AXP_ALWAYS_INLINE IPRStorage_RunLoop& globalIPRHot(CPUIdType id) noexcept
{
    return globalCPUState().runLoop(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_PalIPR& globalIPRHotExt(CPUIdType id) noexcept
{
    return globalCPUState().palIPR(id);
}

AXP_HOT AXP_ALWAYS_INLINE IPRStorage_OSF& globalIPRHot_OSF(CPUIdType id) noexcept
{
    return globalCPUState().osf(id);
}







inline CPUStateView* getCPUStateView(CPUIdType cpuId)
{
    static std::array<CPUStateView, MAX_CPUS> s_views{};
    static std::array<bool, MAX_CPUS> s_bound{};

    if (!s_bound[cpuId])
    {
        s_views[cpuId].bind(globalCPUState(), cpuId);
        s_bound[cpuId] = true;
    }

    return &s_views[cpuId];
}


// ============================================================================
// Compile-Time Verification
// ============================================================================

static_assert(sizeof(IPRStorage_IntRegs) == 256,
    "IntRegs must be 256 bytes (4 cache lines)");

static_assert(sizeof(IPRStorage_FloatRegs) == 256,
    "FloatRegs must be 256 bytes (4 cache lines)");

static_assert(sizeof(IPRStorage_RunLoop) == 64,
    "RunLoop must be 64 bytes (1 cache line)");

static_assert(alignof(IPRStorage_IntRegs) == 64, "IntRegs 64-byte aligned");
static_assert(alignof(IPRStorage_FloatRegs) == 64, "FloatRegs 64-byte aligned");
static_assert(alignof(IPRStorage_PalShadow) == 64, "PalShadow 64-byte aligned");
static_assert(alignof(HWPCB) == 64, "HWPCB 64-byte aligned");
static_assert(alignof(IPRStorage_RunLoop) == 64, "RunLoop 64-byte aligned");
static_assert(alignof(IPRStorage_PalIPR) == 64, "PalIPR 64-byte aligned");
static_assert(alignof(IPRStorage_OSF) == 64, "OSF 64-byte aligned");
static_assert(sizeof(IPRStorage_OSF) == 64, "OSF exactly 1 cache line");

// Verify HWPCB stack pointer contiguity
static_assert(offsetof(HWPCB, esp) == offsetof(HWPCB, ksp) + 8, "ksp->esp");
static_assert(offsetof(HWPCB, ssp) == offsetof(HWPCB, ksp) + 16, "ksp->ssp");
static_assert(offsetof(HWPCB, usp) == offsetof(HWPCB, ksp) + 24, "ksp->usp");

// Verify HWPCB hot fields in cache line 0
static_assert(offsetof(HWPCB, pc) == 0, "pc at offset 0");
static_assert(offsetof(HWPCB, ps) == 8, "ps at offset 8");
static_assert(offsetof(HWPCB, ksp) == 56, "ksp at end of cache line 0");

#endif // GLOBAL_REGISTERMASTER_HOT_H