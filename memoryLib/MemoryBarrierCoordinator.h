// ============================================================================
// MemoryBarrierCoordinator.h - NO ExecutionCoordinator dependency!
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
// MemoryBarrierCoordinator.h - NO ExecutionCoordinator dependency!
// ============================================================================

#ifndef MEMORYBARRIERCOORDINATOR_H
#define MEMORYBARRIERCOORDINATOR_H

#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <atomic>
#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/LoggingMacros.h"
#include "coreLib/IPI_core.h"

#include "coreLib/MCES_helpers.h"
#include "coreLib/enum_MCES.h"
#include "grainFactoryLib/MemoryBarrier_core.h"
#include "faultLib/PendingEvent_refined.h"
#include "faultLib/global_faultDispatcher.h"
#include "exceptionLib/ExceptionFactory.h"
#include "faultLib/FaultDispatcher.h"

// NO INCLUDE of ExecutionCoordinator!

class MemoryBarrierCoordinator final {
public:
    // ====================================================================
    // Singleton Access
    // ====================================================================

    static AXP_HOT AXP_ALWAYS_INLINE MemoryBarrierCoordinator& instance() noexcept
    {
        static MemoryBarrierCoordinator s_instance;
        return s_instance;
    }

    ~MemoryBarrierCoordinator() = default;
    Q_DISABLE_COPY(MemoryBarrierCoordinator)

        // ====================================================================
        // Memory Barrier Coordination (NO IPI SENDING)
        // ====================================================================

        /**
         * @brief Initiate global memory barrier
         * @param cpuId Initiating CPU
         * @param activeCpuCount Number of active CPUs
         * @return true if barrier initiated, false if already in progress
         */
        AXP_HOT AXP_ALWAYS_INLINE bool initiateGlobalMemoryBarrier(
            CPUIdType cpuId,
            quint16 activeCpuCount) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            WARN_LOG(QString("MemoryBarrierCoordinator: Invalid CPU %1").arg(cpuId));
            return false;
        }

        DEBUG_LOG(QString("MemoryBarrierCoordinator: CPU %1 requesting barrier").arg(cpuId));

        QMutexLocker locker(&m_requestMutex);

        if (m_barrierState.barrierInProgress.load(std::memory_order_acquire)) {
            DEBUG_LOG(QString("MemoryBarrierCoordinator: Barrier in progress, CPU %1 joining").arg(cpuId));
            return false;
        }

        if (activeCpuCount <= 1) {
            DEBUG_LOG("MemoryBarrierCoordinator: Single CPU, skipping");
            return false;
        }

        // Initialize barrier state
        m_barrierState.waitingCpus.store(0, std::memory_order_release);
        m_barrierState.acknowledgedCpus.store(0, std::memory_order_release);
        m_barrierState.participatingCpus = activeCpuCount;
        m_barrierState.initiatingCpu = cpuId;
        m_barrierState.barrierInProgress.store(true, std::memory_order_release);

        DEBUG_LOG(QString("MemoryBarrierCoordinator: %1 CPUs participating").arg(activeCpuCount));

        // Initiating CPU acknowledges immediately
        acknowledgeMemoryBarrier(cpuId);

        return true;  // Caller should send IPIs
    }

    AXP_HOT AXP_ALWAYS_INLINE void waitForBarrierAcknowledge(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return;
        }

        DEBUG_LOG(QString("MemoryBarrierCoordinator: CPU %1 waiting").arg(cpuId));

        QMutexLocker locker(&m_barrierState.mutex);
        constexpr int BARRIER_TIMEOUT_MS = 2000;
        IPRStorage_PalIPR& iprHot = globalIPRHotExt(cpuId);
        while (m_barrierState.barrierInProgress.load(std::memory_order_acquire)) {
            if (!m_barrierState.barrierComplete.wait(&m_barrierState.mutex, BARRIER_TIMEOUT_MS)) {
                WARN_LOG(QString("MemoryBarrierCoordinator: Timeout for CPU %1").arg(cpuId));

                CPUIdType initiatingCpu = m_barrierState.initiatingCpu;
                quint32 participatingCpus = m_barrierState.participatingCpus;
                qsizetype acknowledgedCpus = static_cast<qsizetype>(
                    m_barrierState.acknowledgedCpus.load(std::memory_order_acquire));

                PendingEvent ev = makeSmpBarrierTimeoutEvent(
                    cpuId, initiatingCpu, participatingCpus, acknowledgedCpus);

    
                bool mceEnabled = (iprHot.mces & MCES_MASK_MME) != 0;

                iprHot.mces = setMCESFields(iprHot.mces, MachineCheckReason::SMP_BARRIER_TIMEOUT, 0);
        
                if (!mceEnabled) {
                    WARN_LOG("MCES<MME>=0: SMP barrier timeout suppressed");
                    return;
                }

                global_faultDispatcher().raiseFault(ev);
                return;
            }
        }

        DEBUG_LOG(QString("MemoryBarrierCoordinator: CPU %1 acknowledged").arg(cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE void acknowledgeMemoryBarrier(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return;
        }

        if (!m_barrierState.barrierInProgress.load(std::memory_order_acquire)) {
            return;
        }

        DEBUG_LOG(QString("MemoryBarrierCoordinator: CPU %1 acknowledging").arg(cpuId));

        Alpha::Memory::fullBarrier();

        m_barrierState.waitingCpus.fetch_add(1, std::memory_order_acq_rel);
        quint32 acknowledged = m_barrierState.acknowledgedCpus.fetch_add(1, std::memory_order_acq_rel) + 1;
        quint32 participating = m_barrierState.participatingCpus;

        DEBUG_LOG(QString("MemoryBarrierCoordinator: %1/%2 CPUs acknowledged")
            .arg(acknowledged).arg(participating));

        if (acknowledged == participating) {
            QMutexLocker locker(&m_barrierState.mutex);
            if (m_barrierState.barrierInProgress.load(std::memory_order_acquire)) {
                completeGlobalMemoryBarrier();
            }
        }
    }

    // ====================================================================
    // Status Queries
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE bool isMemoryBarrierInProgress() const noexcept
    {
        return m_barrierState.barrierInProgress.load(std::memory_order_acquire);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool isBarrierInProgress() const noexcept
    {
        return isMemoryBarrierInProgress();
    }

    AXP_HOT AXP_ALWAYS_INLINE quint32 getAcknowledgedCount() const noexcept
    {
        return m_barrierState.acknowledgedCpus.load(std::memory_order_acquire);
    }

    AXP_HOT AXP_ALWAYS_INLINE quint32 getParticipatingCount() const noexcept
    {
        return m_barrierState.participatingCpus;
    }

    AXP_HOT AXP_ALWAYS_INLINE CPUIdType getInitiatingCpu() const noexcept
    {
        return m_barrierState.initiatingCpu;
    }

    // ====================================================================
    // State Management
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void resetBarrierState() const noexcept
    {
        QMutexLocker requestLocker(&m_requestMutex);
        QMutexLocker barrierLocker(&m_barrierState.mutex);

        DEBUG_LOG("MemoryBarrierCoordinator: Resetting barrier state");

        m_barrierState.waitingCpus.store(0, std::memory_order_release);
        m_barrierState.acknowledgedCpus.store(0, std::memory_order_release);
        m_barrierState.barrierInProgress.store(false, std::memory_order_release);
        m_barrierState.participatingCpus = 0;
        m_barrierState.initiatingCpu = 0;

        m_barrierState.barrierComplete.wakeAll();
    }

    AXP_HOT AXP_ALWAYS_INLINE QString getBarrierStatus() const noexcept
    {
        QString status;
        status += "MemoryBarrierCoordinator Status:\n";
        status += QString("  Barrier in progress: %1\n").arg(isBarrierInProgress() ? "Yes" : "No");
        status += QString("  Participating CPUs: %1\n").arg(m_barrierState.participatingCpus);
        status += QString("  Acknowledged CPUs: %1\n").arg(getAcknowledgedCount());
        status += QString("  Initiating CPU: %1\n").arg(m_barrierState.initiatingCpu);
        return status;
    }

private:
    AXP_HOT AXP_ALWAYS_INLINE MemoryBarrierCoordinator() noexcept
    {
        m_barrierState.waitingCpus.store(0, std::memory_order_release);
        m_barrierState.acknowledgedCpus.store(0, std::memory_order_release);
        m_barrierState.barrierInProgress.store(false, std::memory_order_release);
        m_barrierState.participatingCpus = 0;
        m_barrierState.initiatingCpu = 0;

        DEBUG_LOG("MemoryBarrierCoordinator: Initialized");
    }

    struct MemoryBarrierState {
        QMutex mutex;
        QWaitCondition barrierComplete;
        std::atomic<quint32> waitingCpus{ 0 };
        std::atomic<quint32> acknowledgedCpus{ 0 };
        std::atomic<bool> barrierInProgress{ false };
        quint32 participatingCpus{ 0 };
        CPUIdType initiatingCpu{ 0 };
    };

    mutable MemoryBarrierState m_barrierState;
    mutable QMutex m_requestMutex;

    // ReSharper disable once CppMemberFunctionMayBeStatic
    AXP_HOT AXP_ALWAYS_INLINE bool isValidCPU(CPUIdType cpuId) const noexcept
    {
        return cpuId < MAX_CPUS;
    }

    AXP_HOT AXP_ALWAYS_INLINE void completeGlobalMemoryBarrier() const noexcept
    {
        DEBUG_LOG("MemoryBarrierCoordinator: Completing barrier");

        m_barrierState.barrierInProgress.store(false, std::memory_order_release);
        m_barrierState.participatingCpus = 0;

        m_barrierState.barrierComplete.wakeAll();

        DEBUG_LOG("MemoryBarrierCoordinator: Barrier completed");
    }
};

#endif // MEMORYBARRIERCOORDINATOR_H