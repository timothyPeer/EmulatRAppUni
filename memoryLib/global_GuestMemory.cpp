// ============================================================================
// global_GuestMemory.cpp - ============================================================================
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
