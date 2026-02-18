#ifndef CALLPALHELPERS_INL_H
#define CALLPALHELPERS_INL_H

#include <QtGlobal>
#include "PALVectorId_Refined.h"

// ============================================================================
// CALL_PAL Helper Functions
// ============================================================================
// Alpha AXP CALL_PAL instruction processing per Architecture Manual.
//
// CALL_PAL encodes a function code in bits [25:0] (6 bits used: [5:0] and [7]).
// Valid ranges:
//   0x00-0x3F: Privileged (kernel mode only)
//   0x80-0xBF: Unprivileged (any mode)
//   0x40-0x7F, 0xC0-0xFF: Invalid (raise OPCDEC)
//

/**
 * @brief Calculate PAL entry PC for CALL_PAL instruction
 *
 * From Alpha AXP Architecture Manual Section 4.7.3:
 * PC[63:15] = PAL_BASE[63:15]
 * PC[14]    = 0
 * PC[13]    = 1
 * PC[12]    = function[7]
 * PC[11:6]  = function[5:0]
 * PC[5:1]   = 0
 * PC[0]     = 1 (PALmode)
 *
 * @param palBase Value of PAL_BASE IPR
 * @param palFunction Function code from CALL_PAL instruction bits [7:0]
 * @return Entry PC with PALmode bit set
 */
inline quint64 calculateCallPalEntryPC(quint64 palBase, quint8 palFunction) noexcept
{
    quint64 pc = palBase & 0xFFFFFFFFFFFF8000ULL;  // Preserve [63:15]
    pc |= 0x2000ULL;                                // Set bit 13
    pc |= ((palFunction & 0x80) ? 0x1000ULL : 0);  // Bit 12 = function[7]
    pc |= (static_cast<quint64>(palFunction & 0x3F) << 6);  // Bits [11:6] = function[5:0]
    pc |= 0x1ULL;                                   // Set bit 0 (PALmode)
    return pc;
}

/**
 * @brief Validate CALL_PAL function code
 *
 * Invalid conditions (raise OPCDEC):
 * - Function in range 0x40-0x7F
 * - Function >= 0xC0
 * - Privileged function (0x00-0x3F) when not in kernel mode
 *
 * @param palFunction Function code from instruction
 * @param currentMode Current privilege mode (0=kernel, 1=exec, 2=super, 3=user)
 * @return true if valid, false if should raise OPCDEC
 */
inline bool isValidCallPalFunction(quint8 palFunction, quint8 currentMode) noexcept
{
    // Invalid ranges: 0x40-0x7F, 0xC0-0xFF
    if ((palFunction >= 0x40 && palFunction <= 0x7F) || palFunction >= 0xC0)
        return false;

    // Privileged (0x00-0x3F) requires kernel mode
    if (palFunction <= 0x3F && currentMode != 0)
        return false;

    return true;
}

/**
 * @brief Check if CALL_PAL function is privileged
 * @param palFunction Function code
 * @return true if function requires kernel mode
 */
inline bool isPrivilegedCallPal(quint8 palFunction) noexcept
{
    return (palFunction <= 0x3F);
}

/**
 * @brief Check if CALL_PAL function is unprivileged
 * @param palFunction Function code
 * @return true if function can execute in any mode
 */
inline bool isUnprivilegedCallPal(quint8 palFunction) noexcept
{
    return (palFunction >= 0x80 && palFunction <= 0xBF);
}

/**
 * @brief Get the PAL function code name (for common vectors)
 * @param palFunction Function code
 * @return String name or nullptr if not a named function
 */
inline const char* getCallPalName(quint8 palFunction) noexcept
{
    // Common unprivileged CALL_PALs
    switch (palFunction)
    {
    case 0x80: return "BPT";
    case 0x81: return "BUGCHK";
    case 0x82: return "CHME";
    case 0x83: return "CHMK";
    case 0x84: return "CHMS";
    case 0x85: return "CHMU";
    case 0x86: return "IMB";
    case 0x9E: return "RDUNIQUE";
    case 0x9F: return "WRUNIQUE";
    case 0xAA: return "GENTRAP";

    // Common privileged CALL_PALs
    case 0x00: return "HALT";
    case 0x01: return "CFLUSH";
    case 0x02: return "DRAINA";
    case 0x09: return "CSERVE";
    case 0x0A: return "SWPPAL";
    case 0x0D: return "WRIPIR";
    case 0x10: return "RDMCES";
    case 0x11: return "WRMCES";
    case 0x2B: return "WRFEN";
    case 0x2D: return "WRVPTPTR";
    case 0x30: return "SWPCTX";
    case 0x31: return "WRVAL";
    case 0x32: return "RDVAL";
    case 0x33: return "TBI";
    case 0x34: return "WRENT";
    case 0x35: return "SWPIPL";
    case 0x36: return "RDPS";
    case 0x37: return "WRKGP";
    case 0x38: return "WRUSP";
    case 0x39: return "WRPERFMON";
    case 0x3A: return "RDUSP";
    case 0x3C: return "WHAMI";
    case 0x3D: return "RETSYS";
    case 0x3E: return "WTINT";
    case 0x3F: return "RTI";

    default:   return nullptr;
    }
}
#endif // CALLPALHELPERS_INL_H
