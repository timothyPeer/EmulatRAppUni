// ============================================================================
// global_MemoryBarrierCoordinator.h - Simplified
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