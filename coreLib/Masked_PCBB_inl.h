#ifndef MASKED_PCBB_INL_H
#define MASKED_PCBB_INL_H

// ============================================================================
// Masked_PCBB_inl.h
// ============================================================================
// Process Control Block Base (PCBB) Register inline helpers
//
// PCBB Register Layout (EV6):
// Bits [43:7] - Physical address of HWPCB (128-byte aligned)
// Bits [6:0]  - Reserved (RAZ/WI, alignment bits)
//
// The PCBB register points to the current Hardware Process Control Block
// (HWPCB) which contains saved processor state for context switching.
//
// Alignment: 128 bytes (2^7), address bits [6:0] are always zero
//
// Reference: Alpha Architecture Reference Manual, HWPCB structure
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "Axp_Attributes_core.h"
#include <QString>

// ============================================================================
// PCBB Constants
// ============================================================================

namespace PCBB {
static constexpr quint64 ADDR_MASK  = 0x00000FFFFFF80ULL;  // Bits 43:7
static constexpr quint64 ADDR_SHIFT = 7;

static constexpr quint64 ALIGNMENT  = 128;  // 2^7 bytes
static constexpr quint64 ALIGN_MASK = 0x7F; // Bits 6:0
}

// ============================================================================
// PCBB Operations
// ============================================================================

/**
 * @brief Get PCBB physical address
 */
AXP_PURE AXP_FLATTEN
    inline quint64 getPCBB_Address(quint64 pcbb) noexcept
{
    return pcbb & PCBB::ADDR_MASK;
}

/**
 * @brief Set PCBB physical address (auto-aligns to 128 bytes)
 */
AXP_FLATTEN
    inline quint64 setPCBB_Address(quint64 physicalAddr) noexcept
{
    // Align to 128 bytes
    return physicalAddr & PCBB::ADDR_MASK;
}

/**
 * @brief Check if address is properly aligned for PCBB
 */
AXP_PURE AXP_FLATTEN
    inline bool isPCBB_Aligned(quint64 address) noexcept
{
    return (address & PCBB::ALIGN_MASK) == 0;
}

/**
 * @brief Validate PCBB value
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidPCBB(quint64 pcbb) noexcept
{
    // Check that reserved bits [6:0] are zero
    return (pcbb & PCBB::ALIGN_MASK) == 0;
}

/**
 * @brief Sanitize PCBB value (clear reserved bits)
 */
AXP_FLATTEN
    inline quint64 sanitizePCBB(quint64 pcbb) noexcept
{
    return pcbb & PCBB::ADDR_MASK;
}

// ============================================================================
// PCBB Display
// ============================================================================

inline QString formatPCBB(quint64 pcbb) noexcept
{
    return QString("PCBB=0x%1 (HWPCB @ PA 0x%2)")
    .arg(pcbb, 16, 16, QChar('0'))
        .arg(getPCBB_Address(pcbb), 11, 16, QChar('0'));
}

inline QString formatPCBB_Detailed(quint64 pcbb) noexcept
{
    QString result = QString("PCBB=0x%1\n").arg(pcbb, 16, 16, QChar('0'));
    result += QString("  Physical Address: 0x%1\n")
                  .arg(getPCBB_Address(pcbb), 11, 16, QChar('0'));
    result += QString("  Alignment: %1 bytes\n").arg(PCBB::ALIGNMENT);
    result += QString("  Aligned: %1\n").arg(isPCBB_Aligned(pcbb) ? "Yes" : "No");

    return result;
}

#endif // MASKED_PCBB_INL_H
