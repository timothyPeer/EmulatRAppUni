// ============================================================================
// IPIManager.h - FIXED VERSION
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#ifndef IPIMANAGER_H
#define IPIMANAGER_H

#include <QtGlobal>
#include <QString>
#include <array>
#include <atomic>
#include "../coreLib/types_core.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../coreLib/IPI_core.h"

// ============================================================================
// IPIManager - Simplified IPI Data Storage
// ============================================================================

class IPIManager final {
public:
    IPIManager() noexcept = default;
    ~IPIManager() = default;

    Q_DISABLE_COPY(IPIManager)

        // ====================================================================
        // Core IPI Operations
        // ====================================================================

        /**
         * @brief Post IPI data to target CPU
         * @param cpuId Target CPU
         * @param ipiData Encoded IPI command + data
         * @return true if posted
         */
        AXP_HOT AXP_ALWAYS_INLINE bool postIPI(CPUIdType cpuId, quint64 ipiData) noexcept
    {
        if (cpuId >= MAX_CPUS) {
            return false;
        }

        // Store IPI data (latest IPI wins - Alpha semantics)
        m_ipiData[cpuId].store(ipiData, std::memory_order_release);

        // Track statistics
        m_ipiStats[cpuId].totalCount.fetch_add(1, std::memory_order_relaxed);

        return true;
    }

    /**
     * @brief Fetch and clear IPI data
     * @param cpuId CPU ID
     * @return IPI data (0 if none)
     */
    AXP_HOT AXP_ALWAYS_INLINE quint64 fetchIPI(CPUIdType cpuId) noexcept
    {
        if (cpuId >= MAX_CPUS) {
            return 0;
        }

        return m_ipiData[cpuId].exchange(0, std::memory_order_acq_rel);
    }

    /**
     * @brief Peek at IPI data without clearing
     * @param cpuId CPU ID
     * @return IPI data (0 if none)
     */
    AXP_HOT AXP_ALWAYS_INLINE quint64 peekIPI(CPUIdType cpuId) const noexcept
    {
        if (cpuId >= MAX_CPUS) {
            return 0;
        }

        return m_ipiData[cpuId].load(std::memory_order_acquire);
    }

    /**
     * @brief Check if IPI pending
     * @param cpuId CPU ID
     * @return true if IPI data != 0
     */
    AXP_HOT AXP_ALWAYS_INLINE bool hasIPIPending(CPUIdType cpuId) const noexcept
    {
        return peekIPI(cpuId) != 0;
    }

    /**
     * @brief Clear IPI data
     * @param cpuId CPU ID
     */
    AXP_HOT AXP_ALWAYS_INLINE void clearIPI(CPUIdType cpuId) noexcept
    {
        if (cpuId < MAX_CPUS) {
            m_ipiData[cpuId].store(0, std::memory_order_release);
        }
    }

    // ====================================================================
    // Statistics
    // ====================================================================

    /**
     * @brief Get total IPI count for CPU
     */
    quint64 getTotalIPICount(CPUIdType cpuId) const noexcept
    {
        if (cpuId >= MAX_CPUS) {
            return 0;
        }
        return m_ipiStats[cpuId].totalCount.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset statistics for all CPUs
     */
    void resetIPIStatistics() noexcept
    {
        for (auto& stats : m_ipiStats) {
            stats.totalCount.store(0, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Get IPI statistics for CPU
     */
    QString getIPIStatistics(CPUIdType cpuId) const noexcept
    {
        if (cpuId >= MAX_CPUS) {
            return QString("Invalid CPU %1").arg(cpuId);
        }

        quint64 total = m_ipiStats[cpuId].totalCount.load(std::memory_order_relaxed);

        return QString("IPI Statistics for CPU %1:\n  Total: %2\n")
            .arg(cpuId)
            .arg(total);
    }

    /**
     * @brief Get all IPI statistics
     */
    QString getAllIPIStatistics() const noexcept
    {
        QString result;
        result += "IPI Statistics Summary:\n";
        result += "======================\n";

        quint64 systemTotal = 0;

        for (CPUIdType cpuId = 0; cpuId < MAX_CPUS; ++cpuId) {
            quint64 count = m_ipiStats[cpuId].totalCount.load(std::memory_order_relaxed);
            if (count > 0) {
                result += QString("CPU %1: %2 total IPIs\n")
                    .arg(cpuId, 2)
                    .arg(count);
                systemTotal += count;
            }
        }

        result += QString("\nSystem Total: %1 IPIs\n").arg(systemTotal);
        return result;
    }

private:
    // ====================================================================
    // Statistics Structure (Simplified)
    // ====================================================================
    struct IPIStats {
        std::atomic<quint64> totalCount{ 0 };
    };

    // ====================================================================
    // Member Data - FIXED: Use MAX_CPUS (compile-time constant)
    // ====================================================================
    std::array<std::atomic<quint64>, MAX_CPUS> m_ipiData{};
    std::array<IPIStats, MAX_CPUS> m_ipiStats{};
};

#endif // IPIMANAGER_H