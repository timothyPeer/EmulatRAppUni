// ============================================================================
// global_writeBufferManager.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global singleton accessor for WriteBufferManager
// Provides SMP-wide write buffer coordination
// ============================================================================

#ifndef GLOBAL_WRITEBUFFERMANAGER_H
#define GLOBAL_WRITEBUFFERMANAGER_H

#include <QtGlobal>

// Forward declarations
class WriteBufferManager;

// ============================================================================
// Global WriteBufferManager Accessor
// ============================================================================

/**
 * @brief Get global WriteBufferManager instance.
 *
 * Matches pattern of other global accessors:
 *   - global_GuestMemory()
 *   - global_MMIOManager()
 *   - global_SMPManager()
 *   - global_IRQController()
 *
 * Must be initialized via initializeGlobalWriteBufferManager() before use.
 *
 * @return Reference to singleton WriteBufferManager
 */
WriteBufferManager& global_WriteBufferManager() noexcept;

/**
 * @brief Initialize global WriteBufferManager (called from EmulatRInit).
 *
 * Creates the singleton instance with the specified CPU count.
 * Must be called during system initialization before any access.
 *
 * @param cpuCount Number of CPUs in system
 * @return Pointer to created instance
 */
WriteBufferManager* initializeGlobalWriteBufferManager(quint16 cpuCount) noexcept;

/**
 * @brief Shutdown global WriteBufferManager (called during cleanup).
 *
 * Destroys the singleton instance and releases resources.
 * Should be called during system shutdown.
 */
void shutdownGlobalWriteBufferManager() noexcept;

/**
 * @brief Check if global WriteBufferManager is initialized.
 *
 * Useful for startup code to verify dependencies.
 *
 * @return true if initialized, false otherwise
 */
bool global_WriteBufferManager_IsInitialized() noexcept;

#endif // GLOBAL_WRITEBUFFERMANAGER_H
