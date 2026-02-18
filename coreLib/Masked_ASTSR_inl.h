#ifndef MASKED_ASTSR_INL_H
#define MASKED_ASTSR_INL_H

// ============================================================================
// Masked_ASTSR_inl.h
// ============================================================================
// AST Summary Register (ASTSR) inline helpers
//
// ASTSR Register Layout (EV6):
// Bits [3:0] - AST pending bits for each mode
//   Bit [3] - ASTK (Kernel mode AST pending)
//   Bit [2] - ASTE (Executive mode AST pending)
//   Bit [1] - ASTS (Supervisor mode AST pending)
//   Bit [0] - ASTU (User mode AST pending)
// Bits [31:4] - Reserved (RAZ/WI)
//
// ASTSR is READ-ONLY (written via ASTRR IPR)
//
// Reference: Alpha Architecture Reference Manual, AST mechanism
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "Axp_Attributes_core.h"
#include "enum_header.h"
#include "../pteLib/alpha_pte_core.h"
#include "coreLib_core.h"

// ============================================================================
// ASTSR Bit Masks (same as ASTEN)
// ============================================================================



// ============================================================================
// ASTSR Getters (Read-Only Status)
// ============================================================================

AXP_PURE AXP_FLATTEN
    inline bool getASTK_Pending(quint32 astsr) noexcept
{
    return (astsr & ASTSR::ASTK_MASK) != 0;
}

AXP_PURE AXP_FLATTEN
    inline bool getASTE_Pending(quint32 astsr) noexcept
{
    return (astsr & ASTSR::ASTE_MASK) != 0;
}

AXP_PURE AXP_FLATTEN
    inline bool getASTS_Pending(quint32 astsr) noexcept
{
    return (astsr & ASTSR::ASTS_MASK) != 0;
}

AXP_PURE AXP_FLATTEN
    inline bool getASTU_Pending(quint32 astsr) noexcept
{
    return (astsr & ASTSR::ASTU_MASK) != 0;
}

/**
 * @brief Get AST pending for specific mode
 */
AXP_PURE AXP_FLATTEN
    inline bool getASTPendingForMode(quint32 astsr, Mode_Privilege mode) noexcept
{
    switch (mode) {
    case Mode_Privilege::Kernel:     return getASTK_Pending(astsr);
    case Mode_Privilege::Executive:  return getASTE_Pending(astsr);
    case Mode_Privilege::Supervisor: return getASTS_Pending(astsr);
    case Mode_Privilege::User:       return getASTU_Pending(astsr);
    default:                        return false;
    }
}

/**
 * @brief Check if any AST is pending
 */
AXP_PURE AXP_FLATTEN
    inline bool isAnyASTPending(quint32 astsr) noexcept
{
    return (astsr & ASTSR::AST_ALL_MASK) != 0;
}

// ============================================================================
// ASTSR Display
// ============================================================================

inline QString formatASTSR(quint32 astsr) noexcept
{
    QStringList parts;
    if (getASTK_Pending(astsr)) parts << "ASTK";
    if (getASTE_Pending(astsr)) parts << "ASTE";
    if (getASTS_Pending(astsr)) parts << "ASTS";
    if (getASTU_Pending(astsr)) parts << "ASTU";

    if (parts.isEmpty()) {
        return "ASTSR[none pending]";
    }

    return QString("ASTSR[%1 pending]").arg(parts.join(" "));
}

#endif // MASKED_ASTSR_INL_H
