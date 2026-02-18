#ifndef MEMORYBARRIERKIND_ENUM_H
#define MEMORYBARRIERKIND_ENUM_H
#include <QtGlobal>
// ============================================================================
// Memory Barrier Kind (Opcode 0x18 - Memory Barrier Instructions)
// ============================================================================

/**
 * @brief Memory barrier and cache hint operations for Alpha AXP architecture.
 *
 * Defines barrier types for enforcing memory ordering, synchronizing exceptions,
 * and providing cache management hints. Barriers are serialization points that
 * stall the pipeline until release conditions are met (prior operations complete,
 * write buffers drain, exceptions deliver). Cache hints (FETCH, ECB) are
 * performance optimizations with no architectural ordering guarantees.
 *
 * Ordering strength: PAL > MB/MB2 > EXCB > WMB > TRAPB > cache hints
 * Enum ordered by value; values reflect Alpha instruction encodings where applicable.
 *
 * @note Hex values 0x0000-0xE800 are Alpha instruction encodings; 0x0001-0x0003
 *       are ordering primitives; 0xFFFF is internal PAL super-barrier.
 */
enum class MemoryBarrierKind : quint16
{
    TRAPB = 0x0000,   // Trap Barrier - Sync arithmetic trap delivery
    LOADLOAD = 0x0001,   // Load-load ordering primitive
    STOSTO = 0x0002,   // Store-store ordering primitive
    STOLOAD = 0x0003,   // Store-load ordering primitive
    EXCB = 0x0400,   // Exception Barrier - Sync exception state (for EXC_ADDR/EXC_SUM)
    MB = 0x4000,   // Memory Barrier - Full fence (loads + stores ordered)
    WMB = 0x4400,   // Write Memory Barrier - Store-store ordering (loads bypass)
    FETCH = 0x8000,   // Prefetch data - Cache hint (read intent)
    MB2 = 0x8400,   // Memory Barrier - Full fence (alternate encoding)
    FETCH_M = 0xA000,   // Prefetch data - Cache hint (modify intent)
    RS = 0xC000,   // Read and Set - Mark cache line (VAX legacy)
    ECB = 0xE800,   // Evict Cache Block - Cache hint (flush line)
    PAL = 0xFFFF    // PAL serialization barrier - Strongest (MB + EXCB + pipeline flush)
};
#endif // MEMORYBARRIERKIND_ENUM_H
