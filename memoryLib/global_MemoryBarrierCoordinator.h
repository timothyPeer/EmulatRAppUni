// ============================================================================
// global_MemoryBarrierCoordinator.h - Simplified
// ============================================================================

#ifndef GLOBAL_MEMORYBARRIERCOORDINATOR_H
#define GLOBAL_MEMORYBARRIERCOORDINATOR_H

#include "MemoryBarrierCoordinator.h"

// Simple inline accessor to singleton
inline MemoryBarrierCoordinator& global_MemoryBarrierCoordinator() noexcept
{
    return MemoryBarrierCoordinator::instance();
}

#endif // GLOBAL_MEMORYBARRIERCOORDINATOR_H