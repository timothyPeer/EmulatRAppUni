// ============================================================================
// GlobalFaultDispatcherBank.h - Get current CPU's fault dispatcher (TLS proxy)
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

//
// Migration Strategy:
//   - TLS proxy allows 200+ embedded files to work unchanged
//   - New code can use explicit CPU parameters
//   - Gradual migration over time as convenient
// ============================================================================

#ifndef GLOBALFAULTDISPATCHERBANK_H
#define GLOBALFAULTDISPATCHERBANK_H

#include "../coreLib/types_core.h"

// Forward declarations
class FaultDispatcher;
struct IFaultSink;

// ============================================================================
// GlobalFaultDispatcherBank - Per-CPU Dispatcher Management
// ============================================================================

class GlobalFaultDispatcherBank final
{
public:
    // Core API - explicit CPU parameter (preferred for new code)
    static FaultDispatcher& getDispatcher(CPUIdType cpuId) noexcept;


    // Management API
    static void initialize(quint16 cpuCount) noexcept;
    static void shutdown() noexcept;
    static quint16 getCpuCount() noexcept;

    // Internal implementation
    static void ensureInitialized() noexcept;

private:
    GlobalFaultDispatcherBank() = delete;
    ~GlobalFaultDispatcherBank() = delete;
    GlobalFaultDispatcherBank(const GlobalFaultDispatcherBank&) = delete;
    GlobalFaultDispatcherBank& operator=(const GlobalFaultDispatcherBank&) = delete;
};

// ============================================================================
// TLS Proxy Functions - Compatibility Layer for Embedded Code
// ============================================================================

/**
 * @brief Get current CPU's fault dispatcher (TLS proxy)
 *
 * Uses CurrentCpuTLS::get() to determine CPU automatically.
 * Perfect for embedded code that can't easily pass CPU ID.
 *
 * @return FaultDispatcher for current CPU
 */
FaultDispatcher& globalFaultDispatcher() noexcept;

/**
 * @brief Get current CPU's fault dispatcher as sink interface (TLS proxy)
 *
 * @return IFaultSink interface for current CPU
 */
FaultDispatcher& globalFaultDispatcherSink() noexcept;

// ============================================================================
// Explicit CPU Functions - Preferred for New Code
// ============================================================================

/**
 * @brief Get specific CPU's fault dispatcher (explicit CPU)
 *
 * Preferred for new code where CPU ID is readily available.
 * More explicit and testable than TLS version.
 *
 * @param cpuId CPU identifier
 * @return FaultDispatcher for specified CPU
 */
FaultDispatcher& globalFaultDispatcher(CPUIdType cpuId) noexcept;

/**
 * @brief Get specific CPU's fault dispatcher as sink (explicit CPU)
 *
 * @param cpuId CPU identifier
 * @return IFaultSink interface for specified CPU
 */
IFaultSink& globalFaultDispatcherSink(CPUIdType cpuId) noexcept;

// ============================================================================
// Array Access - For Efficiency in Hot Paths
// ============================================================================

/**
 * @brief Get array of all fault dispatchers
 *
 * Useful for operations that need to iterate over all CPUs.
 * Returns nullptr if not initialized.
 *
 * @return Pointer to dispatcher array, or nullptr
 */
FaultDispatcher* const* globalFaultDispatcherBank() noexcept;

#endif // GLOBALFAULTDISPATCHERBANK_H
