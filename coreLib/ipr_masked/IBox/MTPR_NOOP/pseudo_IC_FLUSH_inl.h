#ifndef PSEUDO_IC_FLUSH_INL_H
#define PSEUDO_IC_FLUSH_INL_H

// ============================================================================
// pseudo_IC_FLUSH_inl.h
// ============================================================================
// Instruction Cache Flush (IC_FLUSH) pseudo-register inline helpers
//
// IC_FLUSH Register (EV6):
// This is a WRITE-ONLY pseudo-register that triggers instruction cache flush.
// Writing any value to IC_FLUSH initiates a flush of the I-cache.
//
// Implementation Note:
// In a functional emulator, I-cache operations are typically NOOPs since:
// - Instruction fetch is immediate (no cache hierarchy)
// - Cache coherency is implicit (unified memory view)
// - Performance impact is not modeled
//
// However, the interface is provided for:
// - PAL code compatibility
// - Cycle-accurate implementations
// - Debug/trace logging
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "../../types_core.h"
#include "Axp_Attributes_core.h"

// ============================================================================
// IC_FLUSH Operations (NOOP in functional emulator)
// ============================================================================

/**
 * @brief Execute IC_FLUSH operation
 * @param cpuId CPU performing the flush
 * @param value Value written to IC_FLUSH (typically ignored)
 *
 * In a functional emulator, this is a NOOP.
 * In a cycle-accurate model, this would:
 * - Invalidate all I-cache lines
 * - Flush instruction pipeline
 * - Stall until flush completes
 */
AXP_FLATTEN
     void executeIC_FLUSH(CPUIdType cpuId, quint64 value) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(value);

    // NOOP in functional emulator
    // I-cache is not modeled
}

/**
 * @brief Execute IC_FLUSH with logging (debug builds)
 * @param cpuId CPU performing the flush
 * @param value Value written to IC_FLUSH
 */
inline void executeIC_FLUSH_Logged(CPUIdType cpuId, quint64 value) noexcept
{
    Q_UNUSED(value);

    // Log flush operation in debug builds
    DEBUG_LOG(QString("CPU %1: IC_FLUSH (NOOP in functional emulator)").arg(cpuId));

    // NOOP - no actual flush performed
}

/**
 * @brief Execute IC_FLUSH for specific address range (future extension)
 * @param cpuId CPU performing the flush
 * @param startAddr Start of address range
 * @param endAddr End of address range
 *
 * Some implementations support range-based I-cache invalidation.
 * This is a NOOP in functional emulator.
 */
AXP_FLATTEN
     void executeIC_FLUSH_Range(CPUIdType cpuId,
                          quint64 startAddr,
                          quint64 endAddr) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(startAddr);
    Q_UNUSED(endAddr);

    // NOOP in functional emulator
}

// ============================================================================
// IC_FLUSH Status Queries (always return "not busy" in functional model)
// ============================================================================

/**
 * @brief Check if IC_FLUSH operation is in progress
 * @return Always false in functional emulator
 */
AXP_PURE AXP_FLATTEN
    constexpr bool isIC_FLUSH_InProgress() noexcept
{
    return false;  // NOOP completes immediately
}

/**
 * @brief Check if IC_FLUSH is required for coherency
 * @param selfModifyingCode True if code has been modified
 * @return Always false in functional emulator (coherency is implicit)
 */
AXP_PURE AXP_FLATTEN
    constexpr bool isIC_FLUSH_Required(bool selfModifyingCode = false) noexcept
{
    Q_UNUSED(selfModifyingCode);
    return false;  // No I-cache to flush
}

// ============================================================================
// IC_FLUSH Helpers for Self-Modifying Code
// ============================================================================

/**
 * @brief Flush I-cache after code modification
 * @param cpuId CPU that modified code
 * @param modifiedAddr Address of modified instruction(s)
 *
 * Called after self-modifying code or dynamic code generation.
 * NOOP in functional emulator.
 */
AXP_FLATTEN
     void flushAfterCodeModification(CPUIdType cpuId, quint64 modifiedAddr) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(modifiedAddr);

    // NOOP - functional emulator always fetches from current memory
}

/**
 * @brief Flush I-cache on all CPUs (for SMP code modification)
 * @param modifiedAddr Address of modified instruction(s)
 *
 * In real hardware, modified code must be flushed from all CPUs' I-caches.
 * NOOP in functional emulator.
 */
inline void flushAllCPUsAfterCodeModification(quint64 modifiedAddr) noexcept
{
    Q_UNUSED(modifiedAddr);

    // NOOP - no cache to flush
}

// ============================================================================
// IC_FLUSH Integration with Memory Barriers
// ============================================================================

/**
 * @brief Execute memory barrier followed by IC_FLUSH
 * @param cpuId CPU performing the operation
 *
 * Ensures all prior stores are visible before flushing I-cache.
 * NOOP in functional emulator.
 */
AXP_FLATTEN
     void executeMemoryBarrierAndIC_FLUSH(CPUIdType cpuId) noexcept
{
    Q_UNUSED(cpuId);

    // Memory barrier is implicit in functional model
    // IC_FLUSH is NOOP
}

// ============================================================================
// IC_FLUSH Display / Debug Helpers
// ============================================================================

/**
 * @brief Format IC_FLUSH operation for debugging
 */
inline QString formatIC_FLUSH(CPUIdType cpuId, quint64 value) noexcept
{
    return QString("IC_FLUSH[CPU=%1, value=0x%2] (NOOP)")
    .arg(cpuId)
        .arg(value, 16, 16, QChar('0'));
}

/**
 * @brief Get IC_FLUSH operation description
 */
inline QString getIC_FLUSH_Description() noexcept
{
    return "Instruction Cache Flush - invalidates all I-cache entries "
           "(NOOP in functional emulator - I-cache not modeled)";
}

/**
 * @brief Format IC_FLUSH status
 */
inline QString formatIC_FLUSH_Status() noexcept
{
    return "IC_FLUSH Status: Not applicable (functional emulator, no I-cache)";
}

// ============================================================================
// IC_FLUSH Statistics (for cycle-accurate implementations)
// ============================================================================

/**
 * @brief IC_FLUSH statistics structure (for future cycle-accurate model)
 */
struct IC_FLUSH_Stats {
    quint64 flushCount;       // Total number of flushes
    quint64 linesInvalidated; // Total cache lines invalidated
    quint64 cyclesStalled;    // Total cycles stalled for flush

    IC_FLUSH_Stats() noexcept
        : flushCount(0)
        , linesInvalidated(0)
        , cyclesStalled(0)
    {}

    void recordFlush(quint64 lines = 0, quint64 cycles = 0) noexcept
    {
        ++flushCount;
        linesInvalidated += lines;
        cyclesStalled += cycles;
    }

    void reset() noexcept
    {
        flushCount = 0;
        linesInvalidated = 0;
        cyclesStalled = 0;
    }
};

/**
 * @brief Get IC_FLUSH statistics
 * @return Always returns zeroed stats in functional emulator
 */
inline IC_FLUSH_Stats getIC_FLUSH_Stats(CPUIdType cpuId) noexcept
{
    Q_UNUSED(cpuId);
    return IC_FLUSH_Stats();  // Always zero in functional model
}

// ============================================================================
// IC_FLUSH Validation (for PAL code correctness)
// ============================================================================

/**
 * @brief Validate IC_FLUSH write value
 * @return Always true (any value is valid for IC_FLUSH)
 */
AXP_PURE AXP_FLATTEN
    constexpr bool isValidIC_FLUSH_Write(quint64 value) noexcept
{
    Q_UNUSED(value);
    return true;  // IC_FLUSH accepts any value
}

/**
 * @brief Check if IC_FLUSH is supported
 * @return Always true (operation is architecturally defined)
 */
AXP_PURE AXP_FLATTEN
    constexpr bool isIC_FLUSH_Supported() noexcept
{
    return true;  // Architecturally required
}

// ============================================================================
// IC_FLUSH Placeholder for Cycle-Accurate Implementation
// ============================================================================

/**
 * @brief Execute IC_FLUSH with full cache model (cycle-accurate)
 * @param cpuId CPU performing the flush
 * @param value Value written to IC_FLUSH
 * @return Number of cycles required for flush (0 in functional model)
 *
 * Placeholder for cycle-accurate implementation.
 * In that model, this would:
 * - Walk I-cache tag array
 * - Invalidate all valid lines
 * - Count cycles based on cache size/organization
 * - Flush instruction pipeline
 */
inline quint32 executeIC_FLUSH_CycleAccurate(CPUIdType cpuId, quint64 value) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(value);

    // Functional model: completes in 0 cycles (NOOP)
    return 0;

    // Cycle-accurate model would return something like:
    // const quint32 CACHE_LINES = 256;
    // const quint32 CYCLES_PER_LINE = 1;
    // return CACHE_LINES * CYCLES_PER_LINE + PIPELINE_FLUSH_CYCLES;
}

// ============================================================================
// IC_FLUSH Integration Notes
// ============================================================================

/*
 * IC_FLUSH Usage in PAL Code:
 *
 * 1. After modifying code (JIT, dynamic patching):
 *    - Store new instruction(s)
 *    - Execute memory barrier (MB/WMB)
 *    - Write to IC_FLUSH
 *    - Execute instruction memory barrier (IMB)
 *
 * 2. After DMA to instruction memory:
 *    - Wait for DMA completion
 *    - Write to IC_FLUSH
 *
 * 3. During context switch (optional):
 *    - Some implementations flush I-cache on ASN change
 *
 * 4. SMP coherency:
 *    - Send IPI to other CPUs to flush their I-caches
 *    - Each CPU writes to its own IC_FLUSH
 *
 * Functional Emulator Behavior:
 * - All IC_FLUSH writes are NOOPs
 * - Instruction fetch always reads current memory
 * - No cache coherency protocol needed
 * - Self-modifying code works without explicit flush
 */


#endif // PSEUDO_IC_FLUSH_INL_H

/*
 * USAGE:
 * // MTPR IC_FLUSH
AXP_HOT AXP_FLATTEN
void MBox::executeMTPR_IC_FLUSH(PipelineSlot& slot) noexcept
{
    auto& disp = globalFaultDispatcher(m_cpuId);

    if (!globalIPRHot(m_cpuId).isInPalMode()) {
        PendingEvent ev{};
        ev.kind = PendingEventKind::Exception;
        ev.exceptionClass = ExceptionClass::OPCDEC;
        disp.setPendingEvent(ev);
        return;
    }

    const quint64 value = readIntReg(m_cpuId, slot.di.ra);

    // Execute IC_FLUSH (NOOP in functional emulator)
    executeIC_FLUSH(m_cpuId, value);

    // Optional: Log in debug builds
    #ifdef DEBUG_PAL_OPS
    executeIC_FLUSH_Logged(m_cpuId, value);
    #endif

    slot.needsWriteback = false;
}

// Self-modifying code example
void patchInstruction(quint64 addr, quint32 newInstruction) noexcept
{
    // Write new instruction
    writeMemory(addr, newInstruction);

    // Memory barrier (ensures write completes)
    executeMemoryBarrier();

    // Flush I-cache (NOOP in functional emulator)
    flushAfterCodeModification(cpuId, addr);
}

// SMP code modification
void patchInstructionAllCPUs(quint64 addr, quint32 newInstruction) noexcept
{
    // Write instruction
    writeMemory(addr, newInstruction);

    // Flush all CPUs (NOOP in functional emulator)
    flushAllCPUsAfterCodeModification(addr);
}

// Check if flush needed (always false)
if (isIC_FLUSH_Required(selfModifying)) {
    executeIC_FLUSH(cpuId, 0);
}

// Debug output
DEBUG_LOG(formatIC_FLUSH(cpuId, 0));
// Output: "IC_FLUSH[CPU=0, value=0x0000000000000000] (NOOP)"

INFO_LOG(getIC_FLUSH_Description());
// Output: "Instruction Cache Flush - invalidates all I-cache entries (NOOP in functional emulator - I-cache not modeled)"
 *
 */
