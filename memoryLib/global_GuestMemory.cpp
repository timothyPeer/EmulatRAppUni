// ============================================================================
// global_GuestMemory.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global GuestMemory singleton implementation
// ============================================================================

#include "global_GuestMemory.h"
#include "GuestMemory.h"
#include <atomic>

namespace {
    std::atomic<bool> g_initialized{false};
}

GuestMemory& global_GuestMemory() noexcept
{
    static GuestMemory instance;
    g_initialized.store(true, std::memory_order_release);
    return instance;
}

bool global_GuestMemory_IsInitialized() noexcept
{
    return g_initialized.load(std::memory_order_acquire);
}

void global_GuestMemory_Reset() noexcept
{
    g_initialized.store(false, std::memory_order_release);
    // Note: Actual reset would require more complex logic
    // This is primarily for testing purposes
}
