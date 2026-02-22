// ============================================================================
// ExecutionCoordinator.cpp - MINIMAL (QObject requirements only)
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

#include "ExecutionCoordinator.h"
#include "../cpuCoreLib/AlphaCPU.h"
#include "../coreLib/LoggingMacros.h"

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ExecutionCoordinator::ExecutionCoordinator(quint16 cpuCount, QObject* parent)
    : QObject(parent)
    , m_cpuCount(cpuCount)
    , m_reservationManager(cpuCount)
{
    Q_ASSERT(cpuCount > 0 && cpuCount <= MAX_CPUS);
    INFO_LOG(QString("ExecutionCoordinator: Initializing with %1 CPUs").arg(cpuCount));

    // Initialize CBoxes for active CPUs only
    for (quint16 i = 0; i < m_cpuCount; ++i) {
        m_cboxes[i] = std::make_unique<CBox>(i);
        DEBUG_LOG(QString("ExecutionCoordinator: Initialized CBox for CPU %1").arg(i));
    }

    createWorkers();

    INFO_LOG("ExecutionCoordinator: Initialization complete");
}

ExecutionCoordinator::~ExecutionCoordinator()
{
    INFO_LOG("ExecutionCoordinator: Shutting down");
    stop();
    destroyWorkers();
    INFO_LOG("ExecutionCoordinator: Shutdown complete");
}

// ============================================================================
// EXECUTION CONTROL
// ============================================================================

void ExecutionCoordinator::start()
{
    if (m_systemRunning.load()) {
        return;
    }

    INFO_LOG("ExecutionCoordinator: Starting system");

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        if (m_workers[i].alphaCPU) {
            m_workers[i].alphaCPU->start();
        }
    }

    m_systemRunning.store(true);
    m_systemPaused.store(false);

    emit systemStarted();
}

void ExecutionCoordinator::pause()
{
    if (!m_systemRunning.load() || m_systemPaused.load()) {
        return;
    }

    INFO_LOG("ExecutionCoordinator: Pausing system");

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        if (m_workers[i].alphaCPU) {
            m_workers[i].alphaCPU->pause();
        }
    }

    m_systemPaused.store(true);
    emit systemPaused();
}

void ExecutionCoordinator::stop()
{
    if (!m_systemRunning.load()) {
        return;
    }

    INFO_LOG("ExecutionCoordinator: Stopping system");

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        if (m_workers[i].alphaCPU) {
            m_workers[i].alphaCPU->stop();
        }
    }

    m_systemRunning.store(false);
    m_systemPaused.store(false);

    emit systemStopped();
}

void ExecutionCoordinator::reset()
{
    INFO_LOG("ExecutionCoordinator: Resetting system");

    stop();

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        if (m_workers[i].alphaCPU) {
            m_workers[i].alphaCPU->reset();
        }
        m_cpuStateManager.resetCPU(i);
    }

    m_reservationManager.breakAllReservations();
}

// ============================================================================
// SLOTS
// ============================================================================

void ExecutionCoordinator::onCPUHalted(CPUIdType cpuId)
{
    INFO_LOG(QString("ExecutionCoordinator: CPU %1 halted").arg(cpuId));
    emit cpuHalted(cpuId);
}

void ExecutionCoordinator::onCPUError(CPUIdType cpuId, QString reason)
{
    ERROR_LOG(QString("ExecutionCoordinator: CPU %1 error: %2").arg(cpuId).arg(reason));
    emit cpuError(cpuId, reason);
}

// ============================================================================
// WORKER MANAGEMENT
// ============================================================================

void ExecutionCoordinator::createWorkers()
{
    QMutexLocker lock(&m_workersMutex);

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        // Create thread
        m_workers[i].thread = std::make_unique<QThread>();
        m_workers[i].thread->setObjectName(QString("CPU-%1-Thread").arg(i));


        DEBUG_LOG(QString("Created worker for CPU %1 with CBox").arg(i));
    }
}

AlphaCPU* ExecutionCoordinator::getAlphaBootProcessor() const noexcept
{
    return static_cast<AlphaCPU*>(m_workers[0].alphaCPU.get());
}
void ExecutionCoordinator::destroyWorkers()
{
    QMutexLocker lock(&m_workersMutex);

    for (quint16 i = 0; i < m_cpuCount; ++i) {
        if (m_workers[i].thread && m_workers[i].thread->isRunning()) {
            m_workers[i].thread->quit();
            m_workers[i].thread->wait();
        }
        m_workers[i].alphaCPU.reset();
        m_workers[i].thread.reset();

        DEBUG_LOG(QString("Destroyed worker for CPU %1").arg(i));
    }
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

QString ExecutionCoordinator::getSystemStatus() const noexcept
{
    QString status;
    status += QString("System Running: %1\n").arg(m_systemRunning.load() ? "Yes" : "No");
    status += QString("System Paused: %1\n").arg(m_systemPaused.load() ? "Yes" : "No");
    status += QString("CPU Count: %1\n").arg(m_cpuCount);
    status += QString("Active CPUs: %1\n").arg(getActiveCPUCount());
    return status;
}

QString ExecutionCoordinator::getCPUStatus(CPUIdType cpuId) const noexcept
{
    if (!isValidCPU(cpuId)) {
        return QString("Invalid CPU %1").arg(cpuId);
    }
    return m_cpuStateManager.getCPUStateString(cpuId);
}

// ============================================================================
// PER-CPU CONTROL
// ============================================================================

void ExecutionCoordinator::pauseCPU(CPUIdType cpuId) const
{
    if (isValidCPU(cpuId) && m_workers[cpuId].alphaCPU) {
        m_workers[cpuId].alphaCPU->pause();
    }
}

void ExecutionCoordinator::resumeCPU(CPUIdType cpuId) const
{
    if (isValidCPU(cpuId) && m_workers[cpuId].alphaCPU) {
        m_workers[cpuId].alphaCPU->resume();
    }
}

void ExecutionCoordinator::stopCPU(CPUIdType cpuId) const
{
    if (isValidCPU(cpuId) && m_workers[cpuId].alphaCPU) {
        m_workers[cpuId].alphaCPU->stop();
    }
}

// ============================================================================
// MEMORY ORDERING
// ============================================================================

void ExecutionCoordinator::waitForBarrierAcknowledge(CPUIdType cpuId) noexcept
{
    global_MemoryBarrierCoordinator().waitForBarrierAcknowledge(cpuId);
}

void ExecutionCoordinator::acknowledgeMemoryBarrier(CPUIdType cpuId) noexcept
{
    global_MemoryBarrierCoordinator().acknowledgeMemoryBarrier(cpuId);
}

bool ExecutionCoordinator::isMemoryBarrierInProgress() noexcept
{
    return global_MemoryBarrierCoordinator().isBarrierInProgress();
}