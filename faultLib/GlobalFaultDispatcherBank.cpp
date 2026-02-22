// ============================================================================
// GlobalFaultDispatcherBank.cpp - ============================================================================
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

#include "../faultLib/GlobalFaultDispatcherBank.h"
#include "../faultLib/FaultDispatcher.h"
#include "../coreLib/CurrentCpuTls.h"
#include "../coreLib/LoggingMacros.h"
#include <memory>
#include <atomic>

// ============================================================================
// Internal State Management
// ============================================================================

namespace {
    // Thread-safe singleton state
    std::atomic<FaultDispatcher**> g_dispatchers{nullptr};
    std::atomic<quint16> g_cpuCount{0};
    std::atomic<bool> g_initialized{false};

    // Memory management
    std::unique_ptr<FaultDispatcher*[]> g_dispatcherStorage;
}

// ============================================================================
// Core Implementation
// ============================================================================

void GlobalFaultDispatcherBank::ensureInitialized() noexcept
{
    if (g_initialized.load(std::memory_order_acquire)) {
        return; // Fast path - already initialized
    }

    // Auto-initialize with default CPU count
    constexpr quint16 DEFAULT_CPU_COUNT = 4;
    initialize(DEFAULT_CPU_COUNT);
}

void GlobalFaultDispatcherBank::initialize(quint16 cpuCount) noexcept
{
    if (g_initialized.load(std::memory_order_acquire)) {
        WARN_LOG("GlobalFaultDispatcherBank already initialized");
        return;
    }

    Q_ASSERT(cpuCount > 0 && cpuCount <= MAX_CPUS);

    // Create dispatcher array
    g_dispatcherStorage = std::make_unique<FaultDispatcher*[]>(cpuCount);

    // Initialize each dispatcher
    for (quint16 i = 0; i < cpuCount; ++i) {
        g_dispatcherStorage[i] = new FaultDispatcher(i);
    }

    // Publish the array and count atomically
    g_cpuCount.store(cpuCount, std::memory_order_release);
    g_dispatchers.store(g_dispatcherStorage.get(), std::memory_order_release);
    g_initialized.store(true, std::memory_order_release);

    DEBUG_LOG(QString("GlobalFaultDispatcherBank initialized for %1 CPUs").arg(cpuCount));
}

void GlobalFaultDispatcherBank::shutdown() noexcept
{
    if (!g_initialized.load(std::memory_order_acquire)) {
        return;
    }

    DEBUG_LOG("GlobalFaultDispatcherBank shutdown");

    // Mark as not initialized first
    g_initialized.store(false, std::memory_order_release);

    // Clean up dispatchers
    FaultDispatcher** dispatchers = g_dispatchers.load(std::memory_order_acquire);
    quint16 count = g_cpuCount.load(std::memory_order_acquire);

    if (dispatchers) {
        for (quint16 i = 0; i < count; ++i) {
            delete dispatchers[i];
        }
    }

    // Clear storage
    g_dispatcherStorage.reset();
    g_dispatchers.store(nullptr, std::memory_order_release);
    g_cpuCount.store(0, std::memory_order_release);
}

quint16 GlobalFaultDispatcherBank::getCpuCount() noexcept
{
    ensureInitialized();
    return g_cpuCount.load(std::memory_order_acquire);
}

FaultDispatcher& GlobalFaultDispatcherBank::getDispatcher(CPUIdType cpuId) noexcept
{
    ensureInitialized();

    FaultDispatcher** dispatchers = g_dispatchers.load(std::memory_order_acquire);
    quint16 count = g_cpuCount.load(std::memory_order_acquire);

    Q_ASSERT(dispatchers != nullptr);
    Q_ASSERT(cpuId < count);

    return *dispatchers[cpuId];
}



// ============================================================================
// TLS Proxy Functions - Compatibility Layer
// ============================================================================

auto globalFaultDispatcher() noexcept -> FaultDispatcher&
{
    // Use CurrentCpuTLS to get current CPU automatically
    CPUIdType currentCpu = CurrentCpuTLS::get();

    // Validate TLS is set
    if (Q_UNLIKELY(!CurrentCpuTLS::isSet()))
    {
        ERROR_LOG("globalFaultDispatcher() called but CurrentCpuTLS not set!");
        // Fall back to CPU 0 for safety (better than crash)
        currentCpu = 0;
    }

    return GlobalFaultDispatcherBank::getDispatcher(currentCpu);
}

FaultDispatcher& globalFaultDispatcherSink() noexcept
{
    return globalFaultDispatcher(); // FaultDispatcher is-a IFaultSink
}

// ============================================================================
// Explicit CPU Functions - Preferred for New Code
// ============================================================================

FaultDispatcher& globalFaultDispatcher(CPUIdType cpuId) noexcept
{
    return GlobalFaultDispatcherBank::getDispatcher(cpuId);
}

// ============================================================================
// Array Access - For Efficiency in Hot Paths
// ============================================================================

FaultDispatcher* const* globalFaultDispatcherBank() noexcept
{
    GlobalFaultDispatcherBank::ensureInitialized();
    return g_dispatchers.load(std::memory_order_acquire);
}
