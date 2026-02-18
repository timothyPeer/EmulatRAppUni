#ifndef AST_CORE_INL_H
#define AST_CORE_INL_H
#include "coreLib/Axp_Attributes_core.h"
#include <QtGlobal>
#include <QStringList>
#include "coreLib_core.h"
#include "../pteLib/alpha_pte_core.h"

/**
 * @brief Get AST User mode status (bit 10)
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool getASTU(quint64 isum) noexcept
{
    return (isum & ASTER::ASTU_MASK) != 0;
}

/**
 * @brief Get AST Supervisor mode status (bit 9)
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool getASTS(quint64 isum) noexcept
{
    return (isum & ASTER::ASTS_MASK) != 0;
}

/**
 * @brief Get AST Executive mode status (bit 4)
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool getASTE(quint64 isum) noexcept
{
    return (isum & ASTER::ASTE_MASK) != 0;
}

/**
 * @brief Get AST Kernel mode status (bit 3)
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool getASTK(quint64 isum) noexcept
{
    return (isum & ASTER::ASTK_MASK) != 0;
}

AXP_HOT	AXP_ALWAYS_INLINE
    quint8 getAllAST(quint64 isum) noexcept
{
    quint8 ast = 0;
    if (getASTU(isum)) ast |= 0x8;
    if (getASTS(isum)) ast |= 0x4;
    if (getASTE(isum)) ast |= 0x2;
    if (getASTK(isum)) ast |= 0x1;
    return ast;
}


/**
 * @brief Check if any AST is pending
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool isAnyASTPending(quint64 isum) noexcept
{
    return (isum & ASTER::AST_ALL_MASK) != 0;
}

/**
 * @brief Check if AST is pending for current mode
 * @param isum Current ISUM value
 * @param currentMode Current privilege mode (0=K, 1=E, 2=S, 3=U)
 */
AXP_HOT	AXP_ALWAYS_INLINE
    bool isASTPendingForMode(quint64 isum, quint8 currentMode) noexcept
{
    Q_ASSERT(currentMode <= 3);
    switch (currentMode) {
    case 0: return getASTK(isum);  // Kernel
    case 1: return getASTE(isum);  // Executive
    case 2: return getASTS(isum);  // Supervisor
    case 3: return getASTU(isum);  // User
    default: return false;
    }
}

// ============================================================================
// ASTER Setters (PAL-only usage)
// ============================================================================

/**
 * @brief Set AST delivery enable for User mode
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTU(quint64& aster, bool enable) noexcept
{
    if (enable) aster |= ASTER::ASTU_MASK;
    else        aster &= ~ASTER::ASTU_MASK;
}

/**
 * @brief Set AST delivery enable for Supervisor mode
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTS(quint64& aster, bool enable) noexcept
{
    if (enable) aster |= ASTER::ASTS_MASK;
    else        aster &= ~ASTER::ASTS_MASK;
}

/**
 * @brief Set AST delivery enable for Executive mode
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTE(quint64& aster, bool enable) noexcept
{
    if (enable) aster |= ASTER::ASTE_MASK;
    else        aster &= ~ASTER::ASTE_MASK;
}

/**
 * @brief Set AST delivery enable for Kernel mode
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTK(quint64& aster, bool enable) noexcept
{
    if (enable) aster |= ASTER::ASTK_MASK;
    else        aster &= ~ASTER::ASTK_MASK;
}

/**
 * @brief Set AST delivery enable for specific mode
 * @param mode 0=K, 1=E, 2=S, 3=U
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTForMode(quint64& aster, quint8 mode, bool enable) noexcept
{
    Q_ASSERT(mode <= 3);
    switch (mode) {
    case 0: setASTK(aster, enable); break;
    case 1: setASTE(aster, enable); break;
    case 2: setASTS(aster, enable); break;
    case 3: setASTU(aster, enable); break;
    }
}

/**
 * @brief Get AST enable for specific mode
 */
AXP_HOT	AXP_ALWAYS_INLINE
 bool getASTEnableForMode(quint32 asten, Mode_Privilege mode) noexcept
{
    switch (mode) {
    case Mode_Privilege::Kernel:     return getASTK(asten);
    case Mode_Privilege::Executive:  return getASTE(asten);
    case Mode_Privilege::Supervisor: return getASTS(asten);
    case Mode_Privilege::User:       return getASTU(asten);
    default:                        return false;
    }
}

/**
 * @brief Set AST enable for specific mode
 */
AXP_HOT AXP_ALWAYS_INLINE
 void setASTEnableForMode(quint64& asten, Mode_Privilege mode, bool enable) noexcept
{
    switch (mode) {
    case Mode_Privilege::Kernel:     setASTK(asten, enable); break;
    case Mode_Privilege::Executive:  setASTE(asten, enable); break;
    case Mode_Privilege::Supervisor: setASTS(asten, enable); break;
    case Mode_Privilege::User:       setASTU(asten, enable); break;
    }
}


/**
 * @brief Clear all AST delivery enables
 */
AXP_HOT AXP_ALWAYS_INLINE
 void clearAllAST(quint64& aster) noexcept
{
    aster &= ~ASTER::AST_ALL_MASK;
}

// ============================================================================
// ASTER Analysis Helpers
// ============================================================================

/**
 * @brief Determine if any AST delivery is enabled
 */
AXP_HOT	AXP_ALWAYS_INLINE
 bool isAnyASTEnabled(quint64 aster) noexcept
{
    return (aster & ASTER::AST_ALL_MASK) != 0;
}

// ============================================================================
// ASTER Display / Debug Helpers
// ============================================================================

/**
 * @brief Format ASTER for debugging
 */
AXP_HOT	AXP_ALWAYS_INLINE QString formatASTER(quint64 aster) noexcept
{
    QStringList parts;

    if (getASTK(aster)) parts << "K";
    if (getASTE(aster)) parts << "E";
    if (getASTS(aster)) parts << "S";
    if (getASTU(aster)) parts << "U";

    return QString("ASTER[%1]")
        .arg(parts.isEmpty() ? "none" : parts.join(""));
}
// ============================================================================
// ASTEN Validation
// ============================================================================

AXP_HOT	AXP_ALWAYS_INLINE
 bool isValidASTEN(quint32 asten) noexcept
{
	return (asten & ~ASTEN::AST_ALL_MASK) == 0;
}

AXP_HOT	AXP_ALWAYS_INLINE
 quint32 sanitizeASTEN(quint32 asten) noexcept
{
	return asten & ASTEN::AST_ALL_MASK;
}

// ============================================================================
// ASTEN Display
// ============================================================================

AXP_HOT	AXP_ALWAYS_INLINE QString formatASTEN(quint32 asten) noexcept
{
	QStringList parts;
	if (getASTK(asten)) parts << "ASTK";
	if (getASTE(asten)) parts << "ASTE";
	if (getASTS(asten)) parts << "ASTS";
	if (getASTU(asten)) parts << "ASTU";

	if (parts.isEmpty()) {
		return "ASTEN[none]";
	}

	return QString("ASTEN[%1]").arg(parts.join(" "));
}

AXP_HOT	AXP_ALWAYS_INLINE
quint64 buildISUMFromASTSR(quint8 astsr) noexcept
{
	quint64 isum = 0;

	if (astsr & 0x1) setASTK(isum, true);
	if (astsr & 0x2) setASTE(isum, true);
	if (astsr & 0x4) setASTS(isum, true);
	if (astsr & 0x8) setASTU(isum, true);

	return isum;
}

AXP_HOT AXP_ALWAYS_INLINE
quint8 astsrFromISUM(quint64 isum) noexcept
{
	quint8 astsr = 0;

	if (getASTK(isum)) astsr |= 0x1;
	if (getASTE(isum)) astsr |= 0x2;
	if (getASTS(isum)) astsr |= 0x4;
	if (getASTU(isum)) astsr |= 0x8;

	return astsr;
}

AXP_HOT AXP_ALWAYS_INLINE
void updateASTSRFromISUM(quint8& astsr, quint64 isum) noexcept
{
	astsr = astsrFromISUM(isum);
}

#endif // AST_CORE_INL_H
