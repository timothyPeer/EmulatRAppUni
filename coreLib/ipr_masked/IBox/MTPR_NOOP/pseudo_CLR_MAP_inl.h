#ifndef PSEUDO_CLR_MAP_INL_H
#define PSEUDO_CLR_MAP_INL_H

// ============================================================================
// pseudo_CLR_MAP_inl.h
// ============================================================================
// Clear Memory Mapping (CLR_MAP) pseudo-register inline helpers
//
// CLR_MAP Register (EV6):
// This is a WRITE-ONLY pseudo-register accessed via HW_MTPR instruction.
// Writing to CLR_MAP clears/invalidates memory mapping state, typically
// related to physical-to-virtual address translation or memory region mappings.
//
// CLR_MAP is implementation-specific and not fully documented in the
// Alpha Architecture Reference Manual. Known uses include:
// - Clearing memory mapping caches
// - Invalidating physical memory region descriptors
// - Resetting memory controller state
//
// Implementation Note:
// In a functional emulator, this is typically a NOOP because:
// - No memory mapping cache to clear
// - Physical memory access is direct
// - Memory controller state is not modeled
//
// However, the interface is provided for:
// - PAL code compatibility
// - Future cycle-accurate implementations
// - Debug/trace logging
//
// Reference: EV6 Hardware Reference Manual (implementation-specific)
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "../../types_core.h"
#include "Axp_Attributes_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/types_core.h"

// ============================================================================
// CLR_MAP Operations (NOOP in functional emulator)
// ============================================================================

/**
 * @brief Execute CLR_MAP operation
 * @param cpuId CPU performing the clear
 * @param value Value written to CLR_MAP (typically ignored or implementation-specific)
 *
 * In a functional emulator, this is a NOOP.
 * In a cycle-accurate model, this might:
 * - Clear memory mapping cache
 * - Invalidate memory region descriptors
 * - Reset memory controller state
 * - Clear physical address translation buffers
 */
AXP_FLATTEN
    inline void executeCLR_MAP(CPUIdType cpuId, quint64 value) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(value);

    // NOOP in functional emulator
    // No memory mapping cache to clear
}

/**
 * @brief Execute CLR_MAP with logging (debug builds)
 * @param cpuId CPU performing the clear
 * @param value Value written to CLR_MAP
 */
inline void executeCLR_MAP_Logged(CPUIdType cpuId, quint64 value) noexcept
{
    // Log clear operation in debug builds
    DEBUG_LOG(QString("CPU %1: CLR_MAP(value=0x%2) (NOOP in functional emulator)")
                  .arg(cpuId)
                  .arg(value, 16, 16, QChar('0')));

    // NOOP - no actual clear performed
}

/**
 * @brief Execute CLR_MAP for specific memory region (implementation-specific)
 * @param cpuId CPU performing the clear
 * @param regionId Memory region identifier
 *
 * Some implementations may support region-specific clearing.
 * NOOP in functional emulator.
 */
inline void executeCLR_MAP_Region(CPUIdType cpuId, quint32 regionId) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(regionId);

    // NOOP in functional emulator
}

// ============================================================================
// CLR_MAP Status Queries
// ============================================================================

/**
 * @brief Check if CLR_MAP operation is in progress
 * @return Always false in functional emulator
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool isCLR_MAP_InProgress() noexcept
{
    return false;  // NOOP completes immediately
}

/**
 * @brief Check if CLR_MAP is required after memory reconfiguration
 * @return Always false in functional emulator (no state to clear)
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool isCLR_MAP_Required() noexcept
{
    return false;  // No memory mapping state to clear
}

/**
 * @brief Check if memory mapping cache exists
 * @return Always false in functional emulator
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool hasMemoryMappingCache() noexcept
{
    return false;  // Functional emulator doesn't model mapping cache
}

// ============================================================================
// CLR_MAP Helpers for Memory Management
// ============================================================================

/**
 * @brief Clear memory map after physical memory reconfiguration
 * @param cpuId CPU performing the operation
 *
 * Called after hot-plug memory changes or memory controller reconfig.
 * NOOP in functional emulator.
 */
AXP_FLATTEN
    inline void clearMapAfterMemoryReconfig(CPUIdType cpuId) noexcept
{
    Q_UNUSED(cpuId);

    // NOOP - no memory mapping state to clear
}

/**
 * @brief Clear memory map during initialization
 * @param cpuId CPU performing initialization
 *
 * Called during PAL initialization to clear any stale mapping state.
 * NOOP in functional emulator.
 */
AXP_FLATTEN
    inline void clearMapDuringInit(CPUIdType cpuId) noexcept
{
    Q_UNUSED(cpuId);

    // NOOP - no initialization state to clear
}

/**
 * @brief Clear memory map after memory region change
 * @param cpuId CPU performing the clear
 * @param startAddr Start of changed memory region
 * @param endAddr End of changed memory region
 *
 * Some implementations may need to clear mapping cache for specific regions.
 * NOOP in functional emulator.
 */
inline void clearMapAfterRegionChange(CPUIdType cpuId,
                                      quint64 startAddr,
                                      quint64 endAddr) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(startAddr);
    Q_UNUSED(endAddr);

    // NOOP
}

// ============================================================================
// CLR_MAP SMP Helpers
// ============================================================================

/**
 * @brief Clear memory map on all CPUs
 *
 * After global memory reconfiguration, all CPUs must clear their maps.
 * NOOP in functional emulator.
 */
inline void clearMapOnAllCPUs() noexcept
{
    // NOOP - no memory mapping state on any CPU

    // In real hardware:
    // for (each CPU) {
    //     send IPI to CPU
    //     CPU executes CLR_MAP
    // }
}

/**
 * @brief Broadcast memory map clear via IPI
 * @param sourceCpu CPU initiating the clear
 *
 * Used in SMP systems when one CPU reconfigures memory.
 * NOOP in functional emulator.
 */
inline void broadcastMapClearIPI(CPUIdType sourceCpu) noexcept
{
    Q_UNUSED(sourceCpu);

    // NOOP
}

// ============================================================================
// CLR_MAP Display / Debug Helpers
// ============================================================================

/**
 * @brief Format CLR_MAP operation for debugging
 */
inline QString formatCLR_MAP(CPUIdType cpuId, quint64 value) noexcept
{
    return QString("CLR_MAP[CPU=%1, value=0x%2] (NOOP)")
    .arg(cpuId)
        .arg(value, 16, 16, QChar('0'));
}

/**
 * @brief Get CLR_MAP operation description
 */
inline QString getCLR_MAP_Description() noexcept
{
    return "Clear Memory Mapping - invalidates memory mapping cache "
           "(NOOP in functional emulator - no mapping cache modeled)";
}

/**
 * @brief Format CLR_MAP status
 */
inline QString formatCLR_MAP_Status() noexcept
{
    return "CLR_MAP Status: Not applicable (functional emulator, no mapping cache)";
}

// ============================================================================
// CLR_MAP Statistics (for cycle-accurate implementations)
// ============================================================================

/**
 * @brief CLR_MAP statistics structure (for future cycle-accurate model)
 */
struct CLR_MAP_Stats {
    quint64 clearCount;           // Total number of clears
    quint64 entriesCleared;       // Total mapping entries cleared
    quint64 cyclesStalled;        // Total cycles stalled for clear

    CLR_MAP_Stats() noexcept
        : clearCount(0)
        , entriesCleared(0)
        , cyclesStalled(0)
    {}

    void recordClear(quint64 entries = 0, quint64 cycles = 0) noexcept
    {
        ++clearCount;
        entriesCleared += entries;
        cyclesStalled += cycles;
    }

    void reset() noexcept
    {
        clearCount = 0;
        entriesCleared = 0;
        cyclesStalled = 0;
    }
};

/**
 * @brief Get CLR_MAP statistics
 * @return Always returns zeroed stats in functional emulator
 */
inline CLR_MAP_Stats getCLR_MAP_Stats(CPUIdType cpuId) noexcept
{
    Q_UNUSED(cpuId);
    return CLR_MAP_Stats();  // Always zero in functional model
}

// ============================================================================
// CLR_MAP Validation
// ============================================================================

/**
 * @brief Validate CLR_MAP write value
 * @return Always true (any value is valid for CLR_MAP)
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool isValidCLR_MAP_Write(quint64 value) noexcept
{
    Q_UNUSED(value);
    return true;  // CLR_MAP accepts any value (implementation-specific)
}

/**
 * @brief Check if CLR_MAP is supported
 * @return Always true (operation is implementation-defined)
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool isCLR_MAP_Supported() noexcept
{
    return true;  // Supported on EV6
}

// ============================================================================
// CLR_MAP Placeholder for Cycle-Accurate Implementation
// ============================================================================

/**
 * @brief Execute CLR_MAP with full memory mapping model (cycle-accurate)
 * @param cpuId CPU performing the clear
 * @param value Value written to CLR_MAP
 * @return Number of cycles required for clear (0 in functional model)
 *
 * Placeholder for cycle-accurate implementation.
 * In that model, this would:
 * - Walk memory mapping cache
 * - Invalidate all entries (or specific entries based on value)
 * - Count cycles based on cache size
 * - Potentially stall memory operations
 */
inline quint32 executeCLR_MAP_CycleAccurate(CPUIdType cpuId, quint64 value) noexcept
{
    Q_UNUSED(cpuId);
    Q_UNUSED(value);

    // Functional model: completes in 0 cycles (NOOP)
    return 0;

    // Cycle-accurate model would return something like:
    // const quint32 MAPPING_ENTRIES = 64;
    // const quint32 CYCLES_PER_ENTRY = 1;
    // return MAPPING_ENTRIES * CYCLES_PER_ENTRY;
}

// ============================================================================
// CLR_MAP Integration with Other Operations
// ============================================================================

/**
 * @brief Check if CLR_MAP should be combined with TLB invalidation
 * @return Always false in functional emulator (independent operations)
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool shouldCombineWithTLBInvalidation() noexcept
{
    return false;  // Independent operations
}

/**
 * @brief Check if CLR_MAP affects cache coherency
 * @return Always false in functional emulator (no cache modeled)
 */
AXP_HOT AXP_ALWAYS_INLINE 
    constexpr bool affectsCacheCoherency() noexcept
{
    return false;  // No cache to affect
}

// ============================================================================
// CLR_MAP Integration Notes
// ============================================================================

/*
 * CLR_MAP Usage in PAL Code:
 *
 * 1. During PAL initialization:
 *    - Clear any stale memory mapping state from previous boot
 *    - Write to CLR_MAP to reset mapping cache
 *
 * 2. After memory hot-plug/hot-unplug:
 *    - Physical memory configuration changed
 *    - Write to CLR_MAP to invalidate old mappings
 *
 * 3. After memory controller reconfiguration:
 *    - Memory interleaving, striping, or region changes
 *    - Write to CLR_MAP to clear cached mappings
 *
 * 4. During memory error recovery:
 *    - After remapping memory regions away from bad pages
 *    - Write to CLR_MAP to flush stale mappings
 *
 * 5. In SMP systems:
 *    - One CPU writes CLR_MAP
 *    - Sends IPI to other CPUs to execute CLR_MAP
 *    - Ensures consistent view across all CPUs
 *
 * Implementation-Specific Details:
 * - Exact behavior is implementation-defined
 * - May interact with memory controller
 * - May affect physical memory region descriptors
 * - Not directly related to virtual memory (TLB)
 *
 * Functional Emulator Behavior:
 * - All CLR_MAP writes are NOOPs
 * - No memory mapping cache to clear
 * - Physical memory access is direct
 * - Memory reconfiguration doesn't require invalidation
 */

// ============================================================================
// CLR_MAP Relationship to Other IPRs
// ============================================================================

/*
 * CLR_MAP is distinct from:
 *
 * - TLB invalidation (TBIA, TBIAP, TBIS):
 *   - TLB: Virtual-to-physical translation
 *   - CLR_MAP: Physical memory region mapping
 *
 * - Cache flush (IC_FLUSH, DC_FLUSH):
 *   - Cache: Instruction/data cache
 *   - CLR_MAP: Memory mapping descriptors
 *
 * - Memory barriers (MB, WMB):
 *   - Barriers: Ordering of memory operations
 *   - CLR_MAP: Clearing mapping state
 *
 * CLR_MAP is lower-level than TLB and operates on physical memory subsystem.
 */

#endif // PSEUDO_CLR_MAP_INL_H

/*
 * USAGE: // MTPR CLR_MAP
AXP_HOT AXP_FLATTEN
void MBox::executeMTPR_CLR_MAP(PipelineSlot& slot) noexcept
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

    // Execute CLR_MAP (NOOP in functional emulator)
    executeCLR_MAP(m_cpuId, value);

    // Optional: Log in debug builds
    #ifdef DEBUG_PAL_OPS
    executeCLR_MAP_Logged(m_cpuId, value);
    #endif

    slot.needsWriteback = false;
}

// PAL initialization
void palInit() noexcept
{
    // Clear memory mapping state
    clearMapDuringInit(cpuId);

    // Other initialization...
}

// Memory hot-plug event
void handleMemoryHotPlug(quint64 newMemoryBase, quint64 newMemorySize) noexcept
{
    // Reconfigure memory controller
    reconfigureMemory(newMemoryBase, newMemorySize);

    // Clear mapping cache (NOOP in functional emulator)
    clearMapAfterMemoryReconfig(cpuId);

    // Notify other CPUs
    clearMapOnAllCPUs();
}

// Debug output
DEBUG_LOG(formatCLR_MAP(cpuId, 0));
// Output: "CLR_MAP[CPU=0, value=0x0000000000000000] (NOOP)"

INFO_LOG(getCLR_MAP_Description());
// Output: "Clear Memory Mapping - invalidates memory mapping cache (NOOP in functional emulator - no mapping cache modeled)"
 *
 */
