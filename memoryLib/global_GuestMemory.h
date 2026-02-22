// ============================================================================
// global_GuestMemory.h - Get global GuestMemory instance with thread-safe initialization.
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
