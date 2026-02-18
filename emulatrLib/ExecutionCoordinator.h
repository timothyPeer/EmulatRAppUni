// ============================================================================
// ExecutionCoordinator.h - SMP CPU Execution Coordinator
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Coordinates multi-CPU execution, IPI messaging, LL/SC reservations,
//   and memory barriers. Owns CBox instances and manages CPU workers.
//   Most methods inline in header; CTOR/DTOR/slots in .cpp for MOC.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef EXECUTIONCOORDINATOR_H
#define EXECUTIONCOORDINATOR_H

#include <memory>
#include <array>
#include <atomic>

#include "CPUStateManager.h"
#include "IPIManager.h"
#include "global_IPIManager.h"
#include "../coreLib/types_core.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../coreLib/LoggingMacros.h"
#include "../coreLib/IPI_core.h"
#include "../cpuCoreLib/ReservationManager.h"
#include "../memoryLib/global_MemoryBarrierCoordinator.h"
#include "../CBoxLib/CBoxBase.h"
#include "cpuCoreLib/AlphaCPU.h"

// Forward declarations
class AlphaCPU;

// ============================================================================
// CPU Worker Structure
// ============================================================================

struct CPUWorker {
    std::unique_ptr<QThread> thread;
    std::unique_ptr<AlphaCPU> alphaCPU;
};

// ============================================================================
// ExecutionCoordinator - SMP Execution Management
// ============================================================================

class ExecutionCoordinator : public QObject {
    Q_OBJECT

public:
    // ====================================================================
    // Construction (In .cpp for MOC)
    // ====================================================================

    explicit ExecutionCoordinator(quint16 cpuCount, QObject* parent = nullptr);
    ~ExecutionCoordinator() override;

    Q_DISABLE_COPY(ExecutionCoordinator)

        // ====================================================================
        // System Control (In .cpp - emit signals)
        // ====================================================================

        void start();
    void pause();
    void stop();
    void reset();

    // ====================================================================
    // CBox Access (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE CBox* getCBox(CPUIdType cpuId) noexcept
    {
        if (!isValidCPU(cpuId)) {
            return nullptr;
        }
        return m_cboxes[cpuId].get();
    }

    AXP_HOT AXP_ALWAYS_INLINE const CBox* getCBox(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return nullptr;
        }
        return m_cboxes[cpuId].get();
    }

    // ====================================================================
// Subsystem Access (For Global Accessors)
// ====================================================================

/**
 * @brief Get ReservationManager instance
 * Used by global_ReservationManager()
 */
    AXP_HOT AXP_ALWAYS_INLINE ReservationManager& getReservationManager() noexcept
    {
        return m_reservationManager;
    }

    AXP_HOT AXP_ALWAYS_INLINE const ReservationManager& getReservationManager() const noexcept
    {
        return m_reservationManager;
    }

    /**
     * @brief Get IPIManager instance
     * Used by global_IPIManager() if it exists
     */
    AXP_HOT AXP_ALWAYS_INLINE IPIManager& getIPIManager() noexcept
    {
        return global_IPIManager();
    }

    AXP_HOT AXP_ALWAYS_INLINE const IPIManager& getIPIManager() const noexcept
    {
        return global_IPIManager();
    }

    /**
     * @brief Get CPUStateManager instance
     * Used by global_CPUStateManager() if it exists
     */
    AXP_HOT AXP_ALWAYS_INLINE CPUStateManager& getCPUStateManager() noexcept
    {
        return m_cpuStateManager;
    }

    AXP_HOT AXP_ALWAYS_INLINE const CPUStateManager& getCPUStateManager() const noexcept
    {
        return m_cpuStateManager;
    }



    // ====================================================================
    // Write Buffer Management (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void drainWriteBuffers(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId) || !m_cboxes[cpuId]) {
            return;
        }
        m_cboxes[cpuId]->drainWriteBuffers();
        DEBUG_LOG(QString("CPU %1: Write buffers drained").arg(cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingWrites(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId) || !m_cboxes[cpuId]) {
            return false;
        }
        return m_cboxes[cpuId]->hasPendingWrites();
    }

    AXP_HOT AXP_FLATTEN void drainAllWriteBuffers() const noexcept
    {
        DEBUG_LOG("ExecutionCoordinator: Draining all write buffers");

        for (CPUIdType i = 0; i < m_cpuCount; ++i) {
            if (m_cboxes[i]) {
                m_cboxes[i]->drainWriteBuffers();
            }
        }

        DEBUG_LOG("ExecutionCoordinator: All write buffers drained");
    }

    // ====================================================================
    // Memory Barrier Coordination (Inline)
    // ====================================================================

    /**
     * @brief Request global memory barrier
     * Called by CBox when MB instruction executes
     */
    AXP_HOT AXP_FLATTEN bool requestMemoryBarrier(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return false;
        }

        quint16 activeCpuCount = getActiveCPUCount();
        const auto& mbCoord = global_MemoryBarrierCoordinator();

        // Initiate barrier (returns true if IPIs needed)
        if (mbCoord.initiateGlobalMemoryBarrier(cpuId, activeCpuCount)) {
            // Send IPIs to all other CPUs
            quint64 ipiData = encodeIPIData(IPICommand::MEMORY_BARRIER_FULL, 0);

            for (CPUIdType targetCpu = 0; targetCpu < activeCpuCount; ++targetCpu) {
                if (targetCpu != cpuId) {
                    return sendIPI(cpuId, targetCpu, ipiData);
                }
            }

            DEBUG_LOG(QString("CPU %1: Memory barrier IPIs sent to %2 CPUs")
                .arg(cpuId).arg(activeCpuCount - 1));
        }

        return true;
    }

    // Static delegates to global coordinator
    static void waitForBarrierAcknowledge(CPUIdType cpuId) noexcept;
    static void acknowledgeMemoryBarrier(CPUIdType cpuId) noexcept;
    static bool isMemoryBarrierInProgress() noexcept;

    // Initialize CPUs

    AXP_HOT AXP_ALWAYS_INLINE void initializeCPUs() noexcept
    {
        INFO_LOG("ExecutionCoordinator: Initializing CPUs (deferred)...");

        for (quint16 i = 0; i < m_cpuCount; ++i) {
            INFO_LOG(QString("ExecutionCoordinator: Creating AlphaCPU %1...").arg(i));

            // Now it's safe - all globals are initialized
            Q_ASSERT(m_cboxes[i] != nullptr);

            m_workers[i].alphaCPU = std::make_unique<AlphaCPU>(
                i,                      // CPU ID
                m_cboxes[i].get()       // CBox pointer
            );

            INFO_LOG(QString("ExecutionCoordinator: Created AlphaCPU %1").arg(i));
        }

        INFO_LOG("ExecutionCoordinator: All CPUs initialized");
    }

    // ====================================================================
    // IPI Operations (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE bool sendIPI(CPUIdType sourceCpu, CPUIdType targetCpu, quint64 data) const noexcept
    {
        if (!isValidCPU(sourceCpu) || !isValidCPU(targetCpu)) {
            return false;
        }

        // Post IPI data
        if (!global_IPIManager().postIPI(targetCpu, data)) {
            return false;
        }

        // Raise IPI interrupt (would call IRQController)
        // IRQController integration here

        DEBUG_LOG(QString("IPI: CPU %1 -> CPU %2 (data=0x%3)")
            .arg(sourceCpu)
            .arg(targetCpu)
            .arg(data, 16, 16, QChar('0')));

        return true;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 receiveIPI(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return 0;
        }
        return global_IPIManager().fetchIPI(cpuId);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasIPI(CPUIdType cpuId) const noexcept
    {
        if (!isValidCPU(cpuId)) {
            return false;
        }
        return global_IPIManager().hasIPIPending(cpuId);
    }

    // ====================================================================
    // LL/SC Reservation Management (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void setReservation(CPUIdType cpuId, quint64 physAddr) noexcept
    {
        return m_reservationManager.setReservation(cpuId, physAddr);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool checkAndClearReservation(CPUIdType cpuId, quint64 physAddr) noexcept
    {
        return m_reservationManager.checkAndClearReservation(cpuId, physAddr);
    }

    AXP_HOT AXP_ALWAYS_INLINE void breakReservation(CPUIdType cpuId, quint64 physAddr) noexcept
    {
        m_reservationManager.breakReservation(cpuId);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasReservation(CPUIdType cpuId) const noexcept
    {
        return m_reservationManager.hasReservation(cpuId);
    }

    // ====================================================================
    // CPU State Management (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void setCPUHalted(CPUIdType cpuId, bool halted) noexcept
    {
        m_cpuStateManager.setCPUHalted(cpuId, halted);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool isCPUHalted(CPUIdType cpuId) const noexcept
    {
        return m_cpuStateManager.isCPUHalted(cpuId);
    }

    AXP_HOT AXP_ALWAYS_INLINE void setCPUWaiting(CPUIdType cpuId, bool waiting) noexcept
    {
        m_cpuStateManager.setCPUWaiting(cpuId, waiting);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool isCPUWaiting(CPUIdType cpuId) const noexcept
    {
        return m_cpuStateManager.isCPUWaiting(cpuId);
    }

    // ====================================================================
    // Validation (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE bool isValidCPU(CPUIdType cpuId) const noexcept
    {
        return cpuId < m_cpuCount;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint16 getCPUCount() const noexcept
    {
        return m_cpuCount;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint16 getActiveCPUCount() const noexcept
    {
        quint16 active = 0;
        for (quint16 i = 0; i < m_cpuCount; ++i) {
            if (!m_cpuStateManager.isCPUHalted(i)) {
                active++;
            }
        }
        return active;
    }

    AlphaCPU* getAlphaBootProcessor() const noexcept;
    // ====================================================================
    // Diagnostics (Inline)
    // ====================================================================

    QString getSystemStatus() const noexcept;
    QString getCPUStatus(CPUIdType cpuId) const noexcept;

    // ====================================================================
    // Per-CPU Control (.cpp - may emit signals indirectly)
    // ====================================================================

    void pauseCPU(CPUIdType cpuId) const;
    void resumeCPU(CPUIdType cpuId) const;
    void stopCPU(CPUIdType cpuId) const;

signals:
    void systemStarted();
    void systemPaused();
    void systemStopped();
    void cpuHalted(CPUIdType cpuId);
    void cpuError(CPUIdType cpuId, QString reason);

private slots:
    void onCPUHalted(CPUIdType cpuId);
    void onCPUError(CPUIdType cpuId, QString reason);

private:
    // ====================================================================
    // Worker Management (In .cpp - QThread operations)
    // ====================================================================

    void      createWorkers();

    void      destroyWorkers();

    // ====================================================================
    // Member Data
    // ====================================================================

    quint16 m_cpuCount;

    // SMP coordination (direct ownership)
    CPUStateManager m_cpuStateManager;
    ReservationManager m_reservationManager;

    // CBoxes (owned via unique_ptr - sparse array)
    std::array<std::unique_ptr<CBox>, MAX_CPUS> m_cboxes;

    // Workers (owned via unique_ptr)
    std::array<CPUWorker, MAX_CPUS> m_workers;

    // System state
    std::atomic<bool> m_systemRunning{ false };
    std::atomic<bool> m_systemPaused{ false };

    mutable QMutex m_workersMutex;
};

#endif // EXECUTIONCOORDINATOR_H