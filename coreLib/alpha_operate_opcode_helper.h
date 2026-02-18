#ifndef _EMULATRAPPUNI_CORELIB_ALPHA_OPERATE_OPCODE_HELPER_H
#define _EMULATRAPPUNI_CORELIB_ALPHA_OPERATE_OPCODE_HELPER_H

#pragma once
#include <QtGlobal>

#include "Axp_Attributes_core.h"

// ============================================================================
// Alpha AXP Operate Instruction: Trapping Variant Detection
// ----------------------------------------------------------------------------
// For integer operate instructions (opcode 0x10):
//    If function bit <6> (0x40) is set, the /V variant is used.
//    E.g. ADDL  = func 0x00
//         ADDLV = func 0x40
//         SUBL  = func 0x09
//         SUBLV = func 0x49
//
// Reference: AARM, Integer Operate instructions, function field description.
// ============================================================================

inline bool alphaIsTrappingVariant(quint8 opcode, quint16 logFunction) noexcept
{
	// Only OPCODE 0x10 uses the /V pattern for integer operate traps.
	if (opcode != 0x10)
		return false;

	// Function bit <6> indicates the trapping /V variant.
	constexpr quint16 TRAP_BIT = 0x40;

	return (logFunction & TRAP_BIT) != 0;
}

/**
* @brief Sign-extend a 21-bit value to 64 bits (for branch displacements)
*/
AXP_HOT AXP_ALWAYS_INLINE constexpr qint64 signExtend21(quint32 value) noexcept
{
    // Extract lower 21 bits
    const quint32 masked = value & 0x1FFFFF;

    // Check sign bit (bit 20)
    if (masked & 0x100000) {
        // Negative - extend with 1s
        return static_cast<qint64>(masked | 0xFFFFFFFFFFE00000ULL);
    }
    else {
        // Positive - extend with 0s
        return static_cast<qint64>(masked);
    }
}

/**
 * @brief Sign-extend a 16-bit value to 64 bits (for memory displacements)
 */
AXP_HOT AXP_ALWAYS_INLINE constexpr qint64 signExtend16(quint16 value) noexcept
{
    if (value & 0x8000) {
        return static_cast<qint64>(value | 0xFFFFFFFFFFFF0000ULL);
    }
    else {
        return static_cast<qint64>(value);
    }
}

/**
 * @brief Sign-extend a 13-bit value to 64 bits (for literals)
 */
AXP_HOT AXP_ALWAYS_INLINE constexpr qint64 signExtend13(quint16 value) noexcept
{
    const quint16 masked = value & 0x1FFF;
    if (masked & 0x1000) {
        return static_cast<qint64>(masked | 0xFFFFFFFFFFFFE000ULL);
    }
    else {
        return static_cast<qint64>(masked);
    }
}

/**
 * @brief Sign-extend an 8-bit value to 64 bits
 */
AXP_HOT AXP_ALWAYS_INLINE constexpr qint64 signExtend8(quint8 value) noexcept
{
    if (value & 0x80) {
        return static_cast<qint64>(value | 0xFFFFFFFFFFFFFF00ULL);
    }
    else {
        return static_cast<qint64>(value);
    }
}

#endif
