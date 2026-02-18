// ============================================================================
// global_PalVectorTable.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global accessor for PalVectorTable singleton
// Provides system-wide access to PAL vector table
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
