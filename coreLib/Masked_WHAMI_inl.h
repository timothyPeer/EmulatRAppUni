#ifndef MASKED_WHAMI_INL_H
#define MASKED_WHAMI_INL_H

// ============================================================================
// Masked_WHAMI_inl.h
// ============================================================================
// Who Am I (WHAMI) Register inline helpers
//
// WHAMI Register Layout (EV6):
// Bits [7:0]  - CPU ID (0-255)
// Bits [63:8] - Reserved (RAZ)
//
// WHAMI is READ-ONLY and returns the current CPU's ID.
// Used by SMP systems to determine which CPU is executing.
//
// Reference: Alpha Architecture Reference Manual, SMP support
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "Axp_Attributes_core.h"

// ============================================================================
// WHAMI Constants
// ============================================================================

namespace WHAMI {
static constexpr quint64 CPUID_MASK  = 0x00000000000000FFULL;  // Bits 7:0
static constexpr quint64 CPUID_SHIFT = 0;
}

// ============================================================================
// WHAMI Operations
// ============================================================================

/**
 * @brief Get CPU ID from WHAMI register
 */
AXP_PURE AXP_FLATTEN
    inline CPUIdType getWHAMI_CPUID(quint64 whami) noexcept
{
    return static_cast<CPUIdType>(whami & WHAMI::CPUID_MASK);
}

/**
 * @brief Build WHAMI register value from CPU ID
 */
AXP_PURE AXP_FLATTEN
    inline quint64 buildWHAMI(CPUIdType cpuId) noexcept
{
    return static_cast<quint64>(cpuId) & WHAMI::CPUID_MASK;
}

/**
 * @brief Validate WHAMI value
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidWHAMI(quint64 whami) noexcept
{
    // Only bits [7:0] should be set
    return (whami & ~WHAMI::CPUID_MASK) == 0;
}

// ============================================================================
// WHAMI Display
// ============================================================================

inline QString formatWHAMI(quint64 whami) noexcept
{
    return QString("WHAMI[CPU %1]").arg(getWHAMI_CPUID(whami));
}

inline QString formatWHAMI_Detailed(quint64 whami) noexcept
{
    return QString("WHAMI=0x%1 (CPU ID=%2)")
    .arg(whami, 16, 16, QChar('0'))
        .arg(getWHAMI_CPUID(whami));
}

#endif // MASKED_WHAMI_INL_H
