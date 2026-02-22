// ============================================================================
// global_mmioManager.h - Get global MMIOManager singleton instance.
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

#ifndef GLOBAL_MMIOMANAGER_H
#define GLOBAL_MMIOMANAGER_H

// Forward declaration
class MMIOManager;

/**
 * @brief Get global MMIOManager singleton instance.
 *
 * Meyer's singleton with thread-safe initialization.
 * Manages memory-mapped I/O device registration and routing.
 *
 * @return Reference to global MMIOManager instance
 */
MMIOManager& global_MMIOManager() noexcept;

#endif // GLOBAL_MMIOMANAGER_H
