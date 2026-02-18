// ============================================================================
// WriteBufferManager.h - Per-CPU Write Buffer Coordination
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Manages write buffers for all CPUs. Coordinates write ordering and
//   provides drain operations for memory barriers.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef WRITEBUFFERMANAGER_H
#define WRITEBUFFERMANAGER_H

#include <QtGlobal>
#include <QMutex>
#include <QWaitCondition>
#include <functional>
#include <array>
#include "../coreLib/types_core.h"
#include "../coreLib/WriteBufferEntry.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../coreLib/LoggingMacros.h"

// ============================================================================
// Per-CPU Write Buffer State
// ============================================================================

struct CPUWriteBufferState {
    static constexpr int MAX_ENTRIES = 8;

    mutable QMutex mutex;
    QWaitCondition drainedCondition;

    WriteBufferEntry writeBufferEntries[MAX_ENTRIES];

    std::atomic<quint32> pendingCount{ 0 };
    std::atomic<bool> drainRequested{ false };
    std::atomic<bool> drainInProgress{ false };

    quint64 cycleCounter{ 0 };
};

// ============================================================================
// WriteBufferManager
// ============================================================================

class WriteBufferManager final {
public:
    // ====================================================================
    // Construction
    // ====================================================================

    explicit WriteBufferManager(quint16 cpuCount) noexcept
        : m_cpuCount(cpuCount)
    {
        Q_ASSERT(cpuCount > 0 && cpuCount <= MAX_CPUS);

        DEBUG_LOG(QString("WriteBufferManager: Initialized for %1 CPUs").arg(cpuCount));

        // Initialize all CPU buffers
        for (quint16 i = 0; i < m_cpuCount; ++i) {
            initializeCPU(i);
        }
    }

    ~WriteBufferManager() = default;

    Q_DISABLE_COPY(WriteBufferManager)

        // ====================================================================
        // Initialization
        // ====================================================================

        void initializeCPU(CPUIdType cpuId);

    // In public section, after drainCPU()

/**
 * @brief Flush all write buffers for all CPUs
 *
 * Called during shutdown to ensure all pending writes are committed.
 * Drains each CPU's buffer in order using the provided callback.
 *
 * @param commitCallback Callback to commit each write entry
 */
    void flushAllBuffers(std::function<void(CPUIdType, const WriteBufferEntry&)> commitCallback)
    {
        if (!commitCallback) {
            ERROR_LOG("WriteBufferManager::flushAllBuffers: Null callback");
            return;
        }

        INFO_LOG(QString("WriteBufferManager: Flushing all CPU write buffers (%1 CPUs)").arg(m_cpuCount));

        quint32 totalDrained = 0;

        for (quint16 cpuId = 0; cpuId < m_cpuCount; ++cpuId) {
            auto& buffer = m_cpuBuffers[cpuId];

            const quint32 pending = buffer.pendingCount.load(std::memory_order_acquire);
            if (pending == 0) {
                continue;
            }

            DEBUG_LOG(QString("  CPU%1: Flushing %2 pending writes...").arg(cpuId).arg(pending));

            // Drain this CPU's buffer
            drainCPU(cpuId, [cpuId, &commitCallback](const WriteBufferEntry& entry) {
                commitCallback(cpuId, entry);
                });

            totalDrained += pending;
        }

        INFO_LOG(QString("WriteBufferManager: Flushed %1 total write buffer entries").arg(totalDrained));
    }

    // ====================================================================
    // Write Operations (MISSING - Now Added)
    // ====================================================================

    /**
     * @brief Get number of pending writes for CPU
     * CRITICAL: This was missing and CBox calls it!
     */
    AXP_HOT AXP_ALWAYS_INLINE quint32 getPendingWriteCount(CPUIdType cpuId) const noexcept
    {
        if (cpuId >= m_cpuCount) {
            return 0;
        }
        return m_cpuBuffers[cpuId].pendingCount.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if CPU has pending writes
     */
    AXP_HOT AXP_ALWAYS_INLINE bool hasPendingWrites(CPUIdType cpuId) const noexcept
    {
        return getPendingWriteCount(cpuId) > 0;
    }

    /**
     * @brief Add write entry (called by CBox)
     */
    bool addEntry(CPUIdType cpuId, quint64 physAddr, quint64 data,
        quint8 size, quint64 timestamp, bool isMMIO);

    /**
     * @brief Check if CPU has pending MMIO writes
     */
    bool hasPendingMMIO(CPUIdType cpuId) const;

    // ====================================================================
    // Drain Operations
    // ====================================================================

    /**
     * @brief Drain CPU write buffer with callback
     * Called by CBox::drainWriteBuffers()
     */
    void drainCPU(CPUIdType cpuId,
        std::function<void(const WriteBufferEntry&)> commitCallback);

    /**
     * @brief Request drain (asynchronous)
     */
    AXP_HOT AXP_ALWAYS_INLINE void requestDrain(CPUIdType cpuId) noexcept
    {
        if (cpuId >= m_cpuCount) {
            return;
        }
        m_cpuBuffers[cpuId].drainRequested.store(true, std::memory_order_release);
    }

    /**
     * @brief Check if drain is in progress
     */
    AXP_HOT AXP_ALWAYS_INLINE bool isDrainInProgress(CPUIdType cpuId) const noexcept
    {
        if (cpuId >= m_cpuCount) {
            return false;
        }
        return m_cpuBuffers[cpuId].drainInProgress.load(std::memory_order_acquire);
    }

    // ====================================================================
    // Internal Methods (Used by .cpp)
    // ====================================================================

    /**
     * @brief Buffer a write (internal)
     */
    bool bufferWrite(CPUIdType cpuId, quint64 physAddr, quint64 data,
        quint8 size, bool isMMIO);

    /**
     * @brief Dequeue oldest write entry
     */
    bool dequeueWrite(CPUIdType cpuId, WriteBufferEntry& entry);

private:
    // ====================================================================
    // Member Data
    // ====================================================================

    quint16 m_cpuCount;
    std::array<CPUWriteBufferState, MAX_CPUS> m_cpuBuffers;
};

#endif // WRITEBUFFERMANAGER_H