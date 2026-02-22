// ============================================================================
// CPUStateManager.h - ============================================================================
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

// ============================================================================
// CPUStateManager.h
// ============================================================================
// Clean separation: CPU halt, quiescence, and state management only
// Single Responsibility: Track CPU execution states and transitions
// ============================================================================

#ifndef CPUSTATEMANAGER_H
#define CPUSTATEMANAGER_H

#include "emulatrLib_global.h"
#include <QtGlobal>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include "../coreLib/types_core.h"
#include "../coreLib/LoggingMacros.h"

// ============================================================================
// CPU STATE MANAGER - Single responsibility: CPU execution state tracking
// ============================================================================

class CPUStateManager {
public:
    // CPU execution states
    enum class CPUState : quint8 {
        Running,    // Normal execution
        Halted,     // CPU halted (HALT instruction)
        Waiting,    // CPU in wait state
        Quiesced,   // CPU memory operations drained
        Reset       // CPU being reset
    };

    // Constructor/Destructor
    CPUStateManager() noexcept;
    ~CPUStateManager() = default;

    // Non-copyable
    Q_DISABLE_COPY(CPUStateManager)

    // CPU halt state management
    void setCPUHalted(CPUIdType cpuId, bool halted) noexcept;
    bool isCPUHalted(CPUIdType cpuId) const noexcept;
    void notifyCPUHalted(CPUIdType cpuId, quint32 haltCode) noexcept;

    // CPU wait state management
    void setCPUWaiting(CPUIdType cpuId, bool waiting) noexcept;
    bool isCPUWaiting(CPUIdType cpuId) const noexcept;

    // CPU quiescence management
    void requestQuiescence(CPUIdType cpuId) noexcept;
    void waitForQuiescence(CPUIdType cpuId) noexcept;
    void signalQuiescence(CPUIdType cpuId) noexcept;
    bool isQuiescent(CPUIdType cpuId) const noexcept;

    // Pending store tracking
    void registerPendingStore(CPUIdType cpuId) noexcept;
    void completePendingStore(CPUIdType cpuId) noexcept;
    quint32 getPendingStoreCount(CPUIdType cpuId) const noexcept;

    // CPU state queries
    CPUState getCPUState(CPUIdType cpuId) const noexcept;
    quint64 getActiveCPUMask() const noexcept;
    quint16 getActiveCPUCount() const noexcept;

    // Reset support
    void resetCPU(CPUIdType cpuId) noexcept;
    void resetAllCPUs() noexcept;

    // Diagnostics
    QString getCPUStateString(CPUIdType cpuId) const noexcept;
    QString getAllCPUStatesString() const noexcept;

private:
    // CPU quiescence state
    struct CPUQuiescenceState {
        std::atomic<bool> drainRequested{false};
        std::atomic<quint32> pendingStores{0};
        QMutex mutex;
        QWaitCondition quiescedCondition;
        CPUState state{CPUState::Running};
    };

    // CPU state bitmasks
    std::atomic<quint64> m_haltedCpuMask{0};
    std::atomic<quint64> m_waitingCpuMask{0};
    std::array<CPUQuiescenceState, MAX_CPUS> m_quiescenceState;
    // Helper methods
    bool isValidCPU(CPUIdType cpuId) const noexcept { return cpuId < MAX_CPUS; }
    void updateCPUMask(std::atomic<quint64>& mask, CPUIdType cpuId, bool set) noexcept;
};

#endif // CPUSTATEMANAGER_H
