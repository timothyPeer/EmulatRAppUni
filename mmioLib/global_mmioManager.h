// ============================================================================
// global_mmioManager.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global singleton accessor for MMIOManager
// Provides system-wide memory-mapped I/O management
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
