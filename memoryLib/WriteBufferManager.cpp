// ============================================================================
// WriteBufferManager.cpp - Per-CPU Write Buffer Implementation
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#include "WriteBufferManager.h"

#include <algorithm>
#include "../coreLib/LoggingMacros.h"

// ============================================================================
// INITIALIZATION
// ============================================================================

void WriteBufferManager::initializeCPU(CPUIdType cpuId)
{
    if (cpuId >= m_cpuCount) {
        ERROR_LOG(QString("WriteBufferManager::initializeCPU: Invalid CPU ID %1 (max %2)")
            .arg(cpuId).arg(m_cpuCount - 1));
        return;
    }

    auto& buffer = m_cpuBuffers[cpuId];
    QMutexLocker lock(&buffer.mutex);

    // Clear all entries
    for (int i = 0; i < CPUWriteBufferState::MAX_ENTRIES; ++i) {
        buffer.writeBufferEntries[i].clear();
    }

    // Reset state
    buffer.pendingCount.store(0, std::memory_order_release);
    buffer.drainRequested.store(false, std::memory_order_release);
    buffer.drainInProgress.store(false, std::memory_order_release);
    buffer.cycleCounter = 0;

    DEBUG_LOG(QString("WriteBufferManager: Initialized CPU %1 write buffer").arg(cpuId));
}

// ============================================================================
// WRITE OPERATIONS
// ============================================================================

bool WriteBufferManager::addEntry(CPUIdType cpuId, quint64 physAddr, quint64 data,
    quint8 size, quint64 timestamp, bool isMMIO)
{
    if (cpuId >= m_cpuCount) {
        ERROR_LOG(QString("WriteBufferManager::addEntry: Invalid CPU ID %1").arg(cpuId));
        return false;
    }

    // Use internal bufferWrite implementation
    const bool success = bufferWrite(cpuId, physAddr, data, size, isMMIO);

    if (!success) {
        DEBUG_LOG(QString("CPU%1: Write buffer full - PA=0x%2 size=%3")
            .arg(cpuId)
            .arg(physAddr, 16, 16, QChar('0'))
            .arg(size));
    }

    return success;
}

bool WriteBufferManager::hasPendingMMIO(CPUIdType cpuId) const
{
    if (cpuId >= m_cpuCount) {
        return false;
    }

    const auto& buffer = m_cpuBuffers[cpuId];
    QMutexLocker lock(&const_cast<QMutex&>(buffer.mutex));

    // Scan all entries for MMIO writes
    for (int i = 0; i < CPUWriteBufferState::MAX_ENTRIES; ++i) {
        if (buffer.writeBufferEntries[i].valid && buffer.writeBufferEntries[i].mmio) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// DRAIN OPERATIONS
// ============================================================================

// In WriteBufferManager.cpp - drainCPU() method

void WriteBufferManager::drainCPU(CPUIdType cpuId,
    std::function<void(const WriteBufferEntry&)> commitCallback)
{
    if (cpuId >= m_cpuCount) {
        ERROR_LOG(QString("WriteBufferManager::drainCPU: Invalid CPU ID %1").arg(cpuId));
        return;
    }

    if (!commitCallback) {
        ERROR_LOG(QString("WriteBufferManager::drainCPU: Null callback for CPU %1").arg(cpuId));
        return;
    }

    auto& buffer = m_cpuBuffers[cpuId];

    // Mark drain in progress
    buffer.drainInProgress.store(true, std::memory_order_release);

    quint32 drained = 0;

    {
        QMutexLocker lock(&buffer.mutex);

        // Collect all valid entries
        QVector<int> validIndices;
        validIndices.reserve(CPUWriteBufferState::MAX_ENTRIES);

        for (int i = 0; i < CPUWriteBufferState::MAX_ENTRIES; ++i) {
            if (buffer.writeBufferEntries[i].valid) {
                validIndices.append(i);
            }
        }

        // Sort by timestamp - DECLARE VARIABLE FIRST
        WriteBufferEntry* entries = buffer.writeBufferEntries;  //  Local variable

        std::ranges::sort(validIndices,
                          [entries](int a, int b) {  //  Simple capture
                              return entries[a].timestamp < entries[b].timestamp;
                          });

        // Call callback for each entry in order
        for (int idx : validIndices) {
            const WriteBufferEntry& entry = buffer.writeBufferEntries[idx];
            commitCallback(entry);
            buffer.writeBufferEntries[idx].clear();
            drained++;
        }

        // Update pending count
        buffer.pendingCount.store(0, std::memory_order_release);
    }

    // Mark drain complete
    buffer.drainInProgress.store(false, std::memory_order_release);
    buffer.drainRequested.store(false, std::memory_order_release);
    buffer.drainedCondition.wakeAll();

    DEBUG_LOG(QString("CPU%1: Drained %2 write buffer entries").arg(cpuId).arg(drained));
}

// ============================================================================
// INTERNAL METHODS (MISSING - Now Implemented)
// ============================================================================

/**
 * @brief Buffer a write (internal implementation)
 *
 * Called by addEntry(). Finds free slot and adds write entry.
 * Generates timestamp automatically.
 *
 * @param cpuId CPU ID
 * @param physAddr Physical address
 * @param data Data to write
 * @param size Write size (1, 2, 4, 8)
 * @param isMMIO True if MMIO write
 * @return true if buffered, false if buffer full
 */
bool WriteBufferManager::bufferWrite(CPUIdType cpuId, quint64 physAddr, quint64 data,
    quint8 size, bool isMMIO)
{
    if (cpuId >= m_cpuCount) {
        return false;
    }

    auto& buffer = m_cpuBuffers[cpuId];
    QMutexLocker lock(&buffer.mutex);

    // Find free slot
    for (int i = 0; i < CPUWriteBufferState::MAX_ENTRIES; ++i) {
        if (!buffer.writeBufferEntries[i].valid) {
            // Found free slot - add entry
            buffer.writeBufferEntries[i].address = physAddr;
            buffer.writeBufferEntries[i].bufferData = data;
            buffer.writeBufferEntries[i].bufferSize = size;
            buffer.writeBufferEntries[i].timestamp = buffer.cycleCounter++;
            buffer.writeBufferEntries[i].mmio = isMMIO;
            buffer.writeBufferEntries[i].valid = true;

            // Increment pending count
            buffer.pendingCount.fetch_add(1, std::memory_order_release);

            DEBUG_LOG(QString("CPU%1: Buffered write PA=0x%2 size=%3 slot=%4")
                .arg(cpuId)
                .arg(physAddr, 16, 16, QChar('0'))
                .arg(size)
                .arg(i));

            return true;
        }
    }

    // Buffer full
    WARN_LOG(QString("CPU%1: Write buffer FULL - cannot buffer PA=0x%2")
        .arg(cpuId)
        .arg(physAddr, 16, 16, QChar('0')));

    return false;
}

/**
 * @brief Dequeue oldest write entry
 *
 * Helper method for FIFO draining. Finds oldest valid entry
 * based on timestamp, removes it, and returns it.
 *
 * @param cpuId CPU ID
 * @param entry [out] Dequeued entry
 * @return true if entry dequeued, false if buffer empty
 */
bool WriteBufferManager::dequeueWrite(CPUIdType cpuId, WriteBufferEntry& entry)
{
    if (cpuId >= m_cpuCount) {
        return false;
    }

    auto& buffer = m_cpuBuffers[cpuId];
    QMutexLocker lock(&buffer.mutex);

    // Find oldest valid entry
    int oldestIdx = -1;
    quint64 oldestTimestamp = UINT64_MAX;

    for (int i = 0; i < CPUWriteBufferState::MAX_ENTRIES; ++i) {
        if (buffer.writeBufferEntries[i].valid) {
            if (buffer.writeBufferEntries[i].timestamp < oldestTimestamp) {
                oldestTimestamp = buffer.writeBufferEntries[i].timestamp;
                oldestIdx = i;
            }
        }
    }

    // No valid entries found
    if (oldestIdx == -1) {
        return false;
    }

    // Copy entry
    entry = buffer.writeBufferEntries[oldestIdx];

    // Clear slot
    buffer.writeBufferEntries[oldestIdx].clear();

    // Decrement pending count
    buffer.pendingCount.fetch_sub(1, std::memory_order_release);

    DEBUG_LOG(QString("CPU%1: Dequeued write PA=0x%2 size=%3 from slot=%4")
        .arg(cpuId)
        .arg(entry.address, 16, 16, QChar('0'))
        .arg(entry.bufferSize)
        .arg(oldestIdx));

    return true;
}