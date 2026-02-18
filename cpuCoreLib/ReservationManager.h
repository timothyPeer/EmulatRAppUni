// ============================================================================
// ReservationManager.h - Optimized with Actual CPU Count
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

#ifndef RESERVATION_MANAGER_H
#define RESERVATION_MANAGER_H

#include "../coreLib/types_core.h"
#include "../coreLib/Axp_Attributes_core.h"
#include <QtGlobal>
#include <array>



class ReservationManager final
{
public:
    static constexpr quint64 CACHE_LINE_SIZE = 64;
    static constexpr quint64 CACHE_LINE_MASK = ~(CACHE_LINE_SIZE - 1);

    // Constructor now takes CPU count from settings
    explicit ReservationManager(quint16 cpuCount = MAX_CPUS) noexcept
        : m_cpuCount(cpuCount)
    {
        Q_ASSERT(cpuCount > 0 && cpuCount <= MAX_CPUS);
    }

    ~ReservationManager() = default;

    ReservationManager(const ReservationManager&) = delete;
    ReservationManager& operator=(const ReservationManager&) = delete;
    ReservationManager(ReservationManager&&) = delete;
    ReservationManager& operator=(ReservationManager&&) = delete;

    // ====================================================================
    // setReservation - LDL_L / LDQ_L
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE void setReservation(
        CPUIdType cpuId,
        quint64 pa) noexcept
    {
        Q_ASSERT(cpuId < m_cpuCount);  // Validate against ACTIVE CPUs

        quint64 cacheLine = pa & CACHE_LINE_MASK;

        m_reservations[cpuId].reservedCacheLine = cacheLine;
        m_reservations[cpuId].hasReservation = true;
    }

    // ====================================================================
    // checkAndClearReservation - STL_C / STQ_C
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE bool checkAndClearReservation(
        CPUIdType cpuId,
        quint64 pa) noexcept
    {
        Q_ASSERT(cpuId < m_cpuCount);

        quint64 cacheLine = pa & CACHE_LINE_MASK;
        CPUReservation& res = m_reservations[cpuId];

        bool valid = res.hasReservation &&
            (res.reservedCacheLine == cacheLine);

        res.hasReservation = false;

        return valid;
    }

    // ====================================================================
    // breakReservation - Explicit Clear (Single CPU)
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE void breakReservation(CPUIdType cpuId) noexcept
    {
        Q_ASSERT(cpuId < m_cpuCount);
        m_reservations[cpuId].hasReservation = false;
    }

    // ====================================================================
    // breakReservationsOnCacheLine - Cache Coherency (OPTIMIZED)
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE void breakReservationsOnCacheLine(
        quint64 pa) noexcept
    {
        quint64 cacheLine = pa & CACHE_LINE_MASK;

        // Only check ACTIVE CPUs (not all MAX_CPUS)
        for (CPUIdType i = 0; i < m_cpuCount; i++) {  // Only active CPUs!
            if (m_reservations[i].hasReservation &&
                m_reservations[i].reservedCacheLine == cacheLine) {
                m_reservations[i].hasReservation = false;
            }
        }
    }

    // ====================================================================
    // breakAllReservations - System-wide Clear (OPTIMIZED)
    // ====================================================================
    AXP_ALWAYS_INLINE void breakAllReservations() noexcept
    {
        // Only clear ACTIVE CPUs
        for (CPUIdType i = 0; i < m_cpuCount; i++) {  //  Only active CPUs!
            m_reservations[i].hasReservation = false;
        }
    }

    // ====================================================================
    // Query State (Debug/Testing)
    // ====================================================================
    AXP_ALWAYS_INLINE bool hasReservation(CPUIdType cpuId) const noexcept
    {
        Q_ASSERT(cpuId < m_cpuCount);
        return m_reservations[cpuId].hasReservation;
    }

    AXP_ALWAYS_INLINE quint64 getReservedCacheLine(CPUIdType cpuId) const noexcept
    {
        Q_ASSERT(cpuId < m_cpuCount);
        return m_reservations[cpuId].reservedCacheLine;
    }

    AXP_ALWAYS_INLINE quint16 getCPUCount() const noexcept {
        return m_cpuCount;
    }

private:
    struct CPUReservation {
        quint64 reservedCacheLine{ 0 };
        bool hasReservation{ false };
    };

    // Fixed-size array (safety) but only use m_cpuCount entries (efficiency)
    std::array<CPUReservation, MAX_CPUS> m_reservations;
    quint16 m_cpuCount;  // Actual number of active CPUs
};


ReservationManager& globalReservationManager() noexcept;

// Keep these the same:
void initializeReservationManager(quint16 cpuCount) noexcept;
void shutdownReservationManager() noexcept;
#endif // RESERVATION_MANAGER_H