// ============================================================================
// global_PalVectorTable.h - Get global PalVectorTable singleton instance.
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

#ifndef GLOBAL_PALVECTORTABLE_H
#define GLOBAL_PALVECTORTABLE_H

// Forward declaration
class PalVectorTable;

// ============================================================================
// Global PalVectorTable Accessor
// ============================================================================

/**
 * @brief Get global PalVectorTable singleton instance.
 *
 * Provides access to the system-wide PAL vector table for exception/interrupt
 * dispatch. The table is shared across all CPUs in SMP systems.
 *
 * Thread-safe Meyer's singleton pattern with guaranteed initialization.
 *
 * @return Reference to global PalVectorTable instance
 */
PalVectorTable& global_PalVectorTable() noexcept;

#endif // GLOBAL_PALVECTORTABLE_H
