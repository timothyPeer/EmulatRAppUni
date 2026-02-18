// ============================================================================
// global_GuestMemory.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global GuestMemory singleton accessor
// Provides thread-safe initialization and access to the shared memory subsystem
// ============================================================================

#ifndef GLOBAL_GUESTMEMORY_H
#define GLOBAL_GUESTMEMORY_H

// Forward declarations
class GuestMemory;

/**
 * @brief Get global GuestMemory instance with thread-safe initialization.
 *
 * First call initializes the singleton with all required subsystems attached.
 * Subsequent calls return the same instance with minimal overhead.
 *
 * @return Reference to global GuestMemory instance
 */
GuestMemory& global_GuestMemory() noexcept;

/**
 * @brief Check if global GuestMemory is initialized.
 *
 * Useful for startup code that wants to verify dependencies without
 * triggering initialization.
 *
 * @return true if initialized, false otherwise
 */
bool global_GuestMemory_IsInitialized() noexcept;

/**
 * @brief Force re-initialization of global GuestMemory.
 *
 * WARNING: Only use during testing or shutdown. Not thread-safe with
 * concurrent access to GuestMemory.
 */
void global_GuestMemory_Reset() noexcept;

#endif // GLOBAL_GUESTMEMORY_H
