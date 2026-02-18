#ifndef MASKED_EXC_SUM_INL_H
#define MASKED_EXC_SUM_INL_H

// ============================================================================
// Masked_EXC_SUM_inl.h
// ============================================================================
// Exception Summary Register (EXC_SUM) inline helpers
//
// EXC_SUM Register Layout (EV6):
// Bits [63:48] - SEXT(SET_IOV)  Sign extension of SET_IOV bit
// Bit  [47]    - SET_IOV        Set Integer Overflow trap enable
// Bit  [46]    - SET_INE        Set Inexact trap enable
// Bit  [45]    - SET_UNF        Set Underflow trap enable
// Bit  [44]    - SET_OVF        Set Overflow trap enable
// Bit  [43]    - SET_DZE        Set Divide by Zero trap enable
// Bit  [42]    - SET_INV        Set Invalid Operation trap enable
// Bit  [41]    - PC_OVFL        Performance Counter Overflow
// Bit  [13]    - BAD_IVA        Bad Instruction Virtual Address
// Bits [12:8]  - REG            Register number (source of exception)
// Bit  [7]     - INT            Integer arithmetic exception
// Bit  [6]     - IOV            Integer Overflow
// Bit  [5]     - INE            Inexact result
// Bit  [4]     - UNF            Underflow
// Bit  [3]     - FOV            Floating Overflow
// Bit  [2]     - DZE            Divide by Zero
// Bit  [1]     - INV            Invalid Operation
// Bit  [0]     - SWC            Software Completion required
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include "../coreLib/types_core.h"
#include "Axp_Attributes_core.h"
#include <QString>
// ============================================================================
// EXC_SUM Bit Masks
// ============================================================================

namespace EXC_SUM {
// Sign extension field (bits 63:48) - read-only, computed from SET_IOV
static constexpr quint64 SEXT_MASK       = 0xFFFF000000000000ULL;
static constexpr quint64 SEXT_SHIFT      = 48;

// Trap enable bits (SET_xxx) - bits 47:42
static constexpr quint64 SET_IOV_MASK    = 0x0000800000000000ULL;
static constexpr quint64 SET_IOV_SHIFT   = 47;
static constexpr quint64 SET_INE_MASK    = 0x0000400000000000ULL;
static constexpr quint64 SET_INE_SHIFT   = 46;
static constexpr quint64 SET_UNF_MASK    = 0x0000200000000000ULL;
static constexpr quint64 SET_UNF_SHIFT   = 45;
static constexpr quint64 SET_OVF_MASK    = 0x0000100000000000ULL;
static constexpr quint64 SET_OVF_SHIFT   = 44;
static constexpr quint64 SET_DZE_MASK    = 0x0000080000000000ULL;
static constexpr quint64 SET_DZE_SHIFT   = 43;
static constexpr quint64 SET_INV_MASK    = 0x0000040000000000ULL;
static constexpr quint64 SET_INV_SHIFT   = 42;

// All trap enable bits
static constexpr quint64 SET_ALL_MASK    = SET_IOV_MASK | SET_INE_MASK | SET_UNF_MASK |
                                        SET_OVF_MASK | SET_DZE_MASK | SET_INV_MASK;

// Performance Counter Overflow (bit 41)
static constexpr quint64 PC_OVFL_MASK    = 0x0000020000000000ULL;
static constexpr quint64 PC_OVFL_SHIFT   = 41;

// Bad Instruction Virtual Address (bit 13)
static constexpr quint64 BAD_IVA_MASK    = 0x0000000000002000ULL;
static constexpr quint64 BAD_IVA_SHIFT   = 13;

// Register number (bits 12:8)
static constexpr quint64 REG_MASK        = 0x0000000000001F00ULL;
static constexpr quint64 REG_SHIFT       = 8;

// Exception summary bits (bits 7:0)
static constexpr quint64 INT_MASK        = 0x0000000000000080ULL;
static constexpr quint64 INT_SHIFT       = 7;
static constexpr quint64 IOV_MASK        = 0x0000000000000040ULL;
static constexpr quint64 IOV_SHIFT       = 6;
static constexpr quint64 INE_MASK        = 0x0000000000000020ULL;
static constexpr quint64 INE_SHIFT       = 5;
static constexpr quint64 UNF_MASK        = 0x0000000000000010ULL;
static constexpr quint64 UNF_SHIFT       = 4;
static constexpr quint64 FOV_MASK        = 0x0000000000000008ULL;
static constexpr quint64 FOV_SHIFT       = 3;
static constexpr quint64 DZE_MASK        = 0x0000000000000004ULL;
static constexpr quint64 DZE_SHIFT       = 2;
static constexpr quint64 INV_MASK        = 0x0000000000000002ULL;
static constexpr quint64 INV_SHIFT       = 1;
static constexpr quint64 SWC_MASK        = 0x0000000000000001ULL;
static constexpr quint64 SWC_SHIFT       = 0;

// All exception summary bits
static constexpr quint64 EXCEPTION_MASK  = INT_MASK | IOV_MASK | INE_MASK | UNF_MASK |
                                          FOV_MASK | DZE_MASK | INV_MASK | SWC_MASK;

// Valid write mask (excludes SEXT which is computed)
static constexpr quint64 WRITE_MASK      = SET_ALL_MASK | PC_OVFL_MASK | BAD_IVA_MASK |
                                      REG_MASK | EXCEPTION_MASK;
}

// ============================================================================
// EXC_SUM Getters - Trap Enable Bits (SET_xxx)
// ============================================================================

/**
 * @brief Get SET_IOV (Integer Overflow trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_IOV(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_IOV_MASK) != 0;
}

/**
 * @brief Get SET_INE (Inexact trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_INE(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_INE_MASK) != 0;
}

/**
 * @brief Get SET_UNF (Underflow trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_UNF(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_UNF_MASK) != 0;
}

/**
 * @brief Get SET_OVF (Overflow trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_OVF(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_OVF_MASK) != 0;
}

/**
 * @brief Get SET_DZE (Divide by Zero trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_DZE(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_DZE_MASK) != 0;
}

/**
 * @brief Get SET_INV (Invalid Operation trap enable)
 */
AXP_PURE AXP_FLATTEN
     bool getSET_INV(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SET_INV_MASK) != 0;
}

/**
 * @brief Get all trap enable bits as 6-bit mask
 * Bit order: [IOV, INE, UNF, OVF, DZE, INV]
 */
AXP_PURE AXP_FLATTEN
     quint8 getAllTrapEnables(quint64 excSum) noexcept
{
    return static_cast<quint8>((excSum >> EXC_SUM::SET_INV_SHIFT) & 0x3F);
}

// ============================================================================
// EXC_SUM Getters - Status Bits
// ============================================================================

/**
 * @brief Get PC_OVFL (Performance Counter Overflow)
 */
AXP_PURE AXP_FLATTEN
     bool getPC_OVFL(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::PC_OVFL_MASK) != 0;
}

/**
 * @brief Get BAD_IVA (Bad Instruction Virtual Address)
 */
AXP_PURE AXP_FLATTEN
     bool getBAD_IVA(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::BAD_IVA_MASK) != 0;
}

/**
 * @brief Get REG (Register number)
 */
AXP_PURE AXP_FLATTEN
     quint8 getREG(quint64 excSum) noexcept
{
    return static_cast<quint8>((excSum & EXC_SUM::REG_MASK) >> EXC_SUM::REG_SHIFT);
}

// ============================================================================
// EXC_SUM Getters - Exception Summary Bits
// ============================================================================

/**
 * @brief Get INT (Integer arithmetic exception)
 */
AXP_PURE AXP_FLATTEN
     bool getINT(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::INT_MASK) != 0;
}

/**
 * @brief Get IOV (Integer Overflow)
 */
AXP_PURE AXP_FLATTEN
     bool getIOV(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::IOV_MASK) != 0;
}

/**
 * @brief Get INE (Inexact result)
 */
AXP_PURE AXP_FLATTEN
     bool getINE(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::INE_MASK) != 0;
}

/**
 * @brief Get UNF (Underflow)
 */
AXP_PURE AXP_FLATTEN
     bool getUNF(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::UNF_MASK) != 0;
}

/**
 * @brief Get FOV (Floating Overflow)
 */
AXP_PURE AXP_FLATTEN
     bool getFOV(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::FOV_MASK) != 0;
}

/**
 * @brief Get DZE (Divide by Zero)
 */
AXP_PURE AXP_FLATTEN
     bool getDZE(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::DZE_MASK) != 0;
}

/**
 * @brief Get INV (Invalid Operation)
 */
AXP_PURE AXP_FLATTEN
     bool getINV(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::INV_MASK) != 0;
}

/**
 * @brief Get SWC (Software Completion required)
 */
AXP_PURE AXP_FLATTEN
     bool getSWC(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::SWC_MASK) != 0;
}

/**
 * @brief Get all exception summary bits as 8-bit mask
 */
AXP_PURE AXP_FLATTEN
     quint8 getAllExceptionBits(quint64 excSum) noexcept
{
    return static_cast<quint8>(excSum & 0xFF);
}

// ============================================================================
// EXC_SUM Setters - Trap Enable Bits
// ============================================================================

/**
 * @brief Set SET_IOV (Integer Overflow trap enable)
 */
AXP_FLATTEN
     void setSET_IOV(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_IOV_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_IOV_MASK;
    }
}

/**
 * @brief Set SET_INE (Inexact trap enable)
 */
AXP_FLATTEN
     void setSET_INE(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_INE_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_INE_MASK;
    }
}

/**
 * @brief Set SET_UNF (Underflow trap enable)
 */
AXP_FLATTEN
     void setSET_UNF(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_UNF_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_UNF_MASK;
    }
}

/**
 * @brief Set SET_OVF (Overflow trap enable)
 */
AXP_FLATTEN
     void setSET_OVF(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_OVF_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_OVF_MASK;
    }
}

/**
 * @brief Set SET_DZE (Divide by Zero trap enable)
 */
AXP_FLATTEN
     void setSET_DZE(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_DZE_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_DZE_MASK;
    }
}

/**
 * @brief Set SET_INV (Invalid Operation trap enable)
 */
AXP_FLATTEN
     void setSET_INV(quint64& excSum, bool enable) noexcept
{
    if (enable) {
        excSum |= EXC_SUM::SET_INV_MASK;
    } else {
        excSum &= ~EXC_SUM::SET_INV_MASK;
    }
}

/**
 * @brief Set all trap enable bits from mask
 */
AXP_FLATTEN
     void setAllTrapEnables(quint64& excSum, quint8 mask) noexcept
{
    excSum = (excSum & ~EXC_SUM::SET_ALL_MASK) |
             ((static_cast<quint64>(mask & 0x3F) << EXC_SUM::SET_INV_SHIFT) & EXC_SUM::SET_ALL_MASK);
}

// ============================================================================
// EXC_SUM Setters - Status Bits
// ============================================================================

/**
 * @brief Set PC_OVFL (Performance Counter Overflow)
 */
AXP_FLATTEN
     void setPC_OVFL(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::PC_OVFL_MASK;
    } else {
        excSum &= ~EXC_SUM::PC_OVFL_MASK;
    }
}

/**
 * @brief Set BAD_IVA (Bad Instruction Virtual Address)
 */
AXP_FLATTEN
     void setBAD_IVA(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::BAD_IVA_MASK;
    } else {
        excSum &= ~EXC_SUM::BAD_IVA_MASK;
    }
}

/**
 * @brief Set REG (Register number)
 */
AXP_FLATTEN
     void setREG(quint64& excSum, quint8 regNum) noexcept
{
    Q_ASSERT(regNum < 32);
    excSum = (excSum & ~EXC_SUM::REG_MASK) |
             ((static_cast<quint64>(regNum & 0x1F) << EXC_SUM::REG_SHIFT) & EXC_SUM::REG_MASK);
}

// ============================================================================
// EXC_SUM Setters - Exception Summary Bits
// ============================================================================

/**
 * @brief Set INT (Integer arithmetic exception)
 */
AXP_FLATTEN
     void setINT(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::INT_MASK;
    } else {
        excSum &= ~EXC_SUM::INT_MASK;
    }
}

/**
 * @brief Set IOV (Integer Overflow)
 */
AXP_FLATTEN
     void setIOV(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::IOV_MASK;
    } else {
        excSum &= ~EXC_SUM::IOV_MASK;
    }
}

/**
 * @brief Set INE (Inexact result)
 */
AXP_FLATTEN
     void setINE(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::INE_MASK;
    } else {
        excSum &= ~EXC_SUM::INE_MASK;
    }
}

/**
 * @brief Set UNF (Underflow)
 */
AXP_FLATTEN
     void setUNF(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::UNF_MASK;
    } else {
        excSum &= ~EXC_SUM::UNF_MASK;
    }
}

/**
 * @brief Set FOV (Floating Overflow)
 */
AXP_FLATTEN
     void setFOV(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::FOV_MASK;
    } else {
        excSum &= ~EXC_SUM::FOV_MASK;
    }
}

/**
 * @brief Set DZE (Divide by Zero)
 */
AXP_FLATTEN
     void setDZE(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::DZE_MASK;
    } else {
        excSum &= ~EXC_SUM::DZE_MASK;
    }
}

/**
 * @brief Set INV (Invalid Operation)
 */
AXP_FLATTEN
     void setINV(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::INV_MASK;
    } else {
        excSum &= ~EXC_SUM::INV_MASK;
    }
}

/**
 * @brief Set SWC (Software Completion required)
 */
AXP_FLATTEN
     void setSWC(quint64& excSum, bool value) noexcept
{
    if (value) {
        excSum |= EXC_SUM::SWC_MASK;
    } else {
        excSum &= ~EXC_SUM::SWC_MASK;
    }
}

/**
 * @brief Set all exception summary bits from mask
 */
AXP_FLATTEN
     void setAllExceptionBits(quint64& excSum, quint8 mask) noexcept
{
    excSum = (excSum & ~EXC_SUM::EXCEPTION_MASK) | (mask & 0xFF);
}

// ============================================================================
// EXC_SUM Sign Extension
// ============================================================================

/**
 * @brief Apply sign extension to EXC_SUM (extends SET_IOV bit to bits 63:48)
 */
AXP_FLATTEN
     void applySignExtension(quint64& excSum) noexcept
{
    if (excSum & EXC_SUM::SET_IOV_MASK) {
        // SET_IOV is 1, set all SEXT bits to 1
        excSum |= EXC_SUM::SEXT_MASK;
    } else {
        // SET_IOV is 0, clear all SEXT bits
        excSum &= ~EXC_SUM::SEXT_MASK;
    }
}

/**
 * @brief Get sign-extended value (bits 63:48 match SET_IOV)
 */
AXP_PURE AXP_FLATTEN
     quint16 getSEXT(quint64 excSum) noexcept
{
    return static_cast<quint16>((excSum & EXC_SUM::SEXT_MASK) >> EXC_SUM::SEXT_SHIFT);
}

// ============================================================================
// EXC_SUM Analysis Helpers
// ============================================================================

/**
 * @brief Check if any exception is pending
 */
AXP_PURE AXP_FLATTEN
    inline bool isAnyExceptionPending(quint64 excSum) noexcept
{
    return (excSum & EXC_SUM::EXCEPTION_MASK) != 0;
}

/**
 * @brief Check if arithmetic exception should trap
 * @param excSum Current EXC_SUM value
 * @return true if exception matches enabled trap
 */
AXP_PURE
     bool shouldTrapOnException(quint64 excSum) noexcept
{
    // Check each exception against its trap enable
    if (getIOV(excSum) && getSET_IOV(excSum)) return true;
    if (getINE(excSum) && getSET_INE(excSum)) return true;
    if (getUNF(excSum) && getSET_UNF(excSum)) return true;
    if (getFOV(excSum) && getSET_OVF(excSum)) return true;
    if (getDZE(excSum) && getSET_DZE(excSum)) return true;
    if (getINV(excSum) && getSET_INV(excSum)) return true;

    return false;
}

/**
 * @brief Count active exception bits
 */
AXP_PURE
     quint8 countExceptions(quint64 excSum) noexcept
{
    quint8 count = 0;
    if (getINT(excSum)) ++count;
    if (getIOV(excSum)) ++count;
    if (getINE(excSum)) ++count;
    if (getUNF(excSum)) ++count;
    if (getFOV(excSum)) ++count;
    if (getDZE(excSum)) ++count;
    if (getINV(excSum)) ++count;
    if (getSWC(excSum)) ++count;
    return count;
}

// ============================================================================
// EXC_SUM Display / Debug Helpers
// ============================================================================

/**
 * @brief Format EXC_SUM for debugging
 */
inline QString formatEXC_SUM(quint64 excSum) noexcept
{
    QStringList parts;

    // Trap enables
    if (getSET_IOV(excSum)) parts << "SET_IOV";
    if (getSET_INE(excSum)) parts << "SET_INE";
    if (getSET_UNF(excSum)) parts << "SET_UNF";
    if (getSET_OVF(excSum)) parts << "SET_OVF";
    if (getSET_DZE(excSum)) parts << "SET_DZE";
    if (getSET_INV(excSum)) parts << "SET_INV";

    // Status
    if (getPC_OVFL(excSum)) parts << "PC_OVFL";
    if (getBAD_IVA(excSum)) parts << "BAD_IVA";

    quint8 reg = getREG(excSum);
    if (reg != 0) parts << QString("REG=%1").arg(reg);

    // Exceptions
    if (getINT(excSum)) parts << "INT";
    if (getIOV(excSum)) parts << "IOV";
    if (getINE(excSum)) parts << "INE";
    if (getUNF(excSum)) parts << "UNF";
    if (getFOV(excSum)) parts << "FOV";
    if (getDZE(excSum)) parts << "DZE";
    if (getINV(excSum)) parts << "INV";
    if (getSWC(excSum)) parts << "SWC";

    return QString("EXC_SUM[%1]").arg(parts.isEmpty() ? "none" : parts.join(" "));
}

/**
 * @brief Format EXC_SUM with detailed breakdown
 */
inline QString formatEXC_SUM_Detailed(quint64 excSum) noexcept
{
    QString result = QString("EXC_SUM=0x%1\n").arg(excSum, 16, 16, QChar('0'));

    result += "  Trap Enables:\n";
    result += QString("    SET_IOV[47] = %1\n").arg(getSET_IOV(excSum) ? 1 : 0);
    result += QString("    SET_INE[46] = %1\n").arg(getSET_INE(excSum) ? 1 : 0);
    result += QString("    SET_UNF[45] = %1\n").arg(getSET_UNF(excSum) ? 1 : 0);
    result += QString("    SET_OVF[44] = %1\n").arg(getSET_OVF(excSum) ? 1 : 0);
    result += QString("    SET_DZE[43] = %1\n").arg(getSET_DZE(excSum) ? 1 : 0);
    result += QString("    SET_INV[42] = %1\n").arg(getSET_INV(excSum) ? 1 : 0);

    result += "  Status:\n";
    result += QString("    PC_OVFL[41] = %1\n").arg(getPC_OVFL(excSum) ? 1 : 0);
    result += QString("    BAD_IVA[13] = %1\n").arg(getBAD_IVA(excSum) ? 1 : 0);
    result += QString("    REG[12:8]   = %1\n").arg(getREG(excSum));

    result += "  Exceptions:\n";
    result += QString("    INT[7] = %1\n").arg(getINT(excSum) ? 1 : 0);
    result += QString("    IOV[6] = %1\n").arg(getIOV(excSum) ? 1 : 0);
    result += QString("    INE[5] = %1\n").arg(getINE(excSum) ? 1 : 0);
    result += QString("    UNF[4] = %1\n").arg(getUNF(excSum) ? 1 : 0);
    result += QString("    FOV[3] = %1\n").arg(getFOV(excSum) ? 1 : 0);
    result += QString("    DZE[2] = %1\n").arg(getDZE(excSum) ? 1 : 0);
    result += QString("    INV[1] = %1\n").arg(getINV(excSum) ? 1 : 0);
    result += QString("    SWC[0] = %1\n").arg(getSWC(excSum) ? 1 : 0);

    return result;
}

/**
 * @brief Build EXC_SUM from arithmetic exception
 */
inline quint64 buildEXC_SUM_FromException(quint8 regNum,
                                          bool isInteger,
                                          bool overflow,
                                          bool inexact,
                                          bool underflow,
                                          bool divZero,
                                          bool invalid,
                                          bool needsSWC) noexcept
{
    quint64 excSum = 0;

    setREG(excSum, regNum);
    setINT(excSum, isInteger);
    setIOV(excSum, overflow && isInteger);
    setFOV(excSum, overflow && !isInteger);
    setINE(excSum, inexact);
    setUNF(excSum, underflow);
    setDZE(excSum, divZero);
    setINV(excSum, invalid);
    setSWC(excSum, needsSWC);

    return excSum;
}

#endif // MASKED_EXC_SUM_INL_H
