#ifndef MASKED_I_CTL_INL_H
#define MASKED_I_CTL_INL_H

// ============================================================================
// Masked_I_CTL_inl.h
// ============================================================================
// Ibox Control Register (I_CTL) inline helpers
//
// I_CTL Register Layout (EV6):
// Bit  [22]    - TB_MB_EN        Trap barrier / Memory barrier enable
// Bit  [21]    - MCHK_EN         Machine check enable
// Bit  [20]    - CALL_PAL_R23    CALL_PAL uses R23 for return address
// Bit  [19]    - PCT1_EN         Performance Counter 1 enable
// Bit  [18]    - PCT0_EN         Performance Counter 0 enable
// Bit  [17]    - SINGLE_ISSUE_H  Single issue mode (hardware)
// Bit  [16]    - VA_FORM_32      Virtual address format is 32-bit
// Bit  [15]    - VA_48           Virtual address is 48-bit (vs 43-bit)
// Bit  [14]    - SL_RCV          Serial line receive enable
// Bit  [13]    - SL_XMIT         Serial line transmit enable
// Bit  [12]    - HWE             Hardware error enable
// Bits [11:10] - BP_MODE         Branch prediction mode
// Bits [9:8]   - SBE             Store buffer enable
// Bits [7:6]   - SDE             Store data enable
// Bits [5:3]   - SPE             Speculative execution enable
// Bits [2:1]   - IC_EN           I-cache enable
// Bit  [0]     - SPCE            Speculative cache enable
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include "../../types_core.h"
#include "Axp_Attributes_core.h"
#include <QString>

// ============================================================================
// I_CTL Bit Masks
// ============================================================================

namespace I_CTL {
// Single-bit control flags
static constexpr quint32 TB_MB_EN_MASK      = 0x00400000;  // Bit 22
static constexpr quint32 TB_MB_EN_SHIFT     = 22;

static constexpr quint32 MCHK_EN_MASK       = 0x00200000;  // Bit 21
static constexpr quint32 MCHK_EN_SHIFT      = 21;

static constexpr quint32 CALL_PAL_R23_MASK  = 0x00100000;  // Bit 20
static constexpr quint32 CALL_PAL_R23_SHIFT = 20;

static constexpr quint32 PCT1_EN_MASK       = 0x00080000;  // Bit 19
static constexpr quint32 PCT1_EN_SHIFT      = 19;

static constexpr quint32 PCT0_EN_MASK       = 0x00040000;  // Bit 18
static constexpr quint32 PCT0_EN_SHIFT      = 18;

static constexpr quint32 SINGLE_ISSUE_H_MASK = 0x00020000; // Bit 17
static constexpr quint32 SINGLE_ISSUE_H_SHIFT = 17;

static constexpr quint32 VA_FORM_32_MASK    = 0x00010000;  // Bit 16
static constexpr quint32 VA_FORM_32_SHIFT   = 16;

static constexpr quint32 VA_48_MASK         = 0x00008000;  // Bit 15
static constexpr quint32 VA_48_SHIFT        = 15;

static constexpr quint32 SL_RCV_MASK        = 0x00004000;  // Bit 14
static constexpr quint32 SL_RCV_SHIFT       = 14;

static constexpr quint32 SL_XMIT_MASK       = 0x00002000;  // Bit 13
static constexpr quint32 SL_XMIT_SHIFT      = 13;

static constexpr quint32 HWE_MASK           = 0x00001000;  // Bit 12
static constexpr quint32 HWE_SHIFT          = 12;

static constexpr quint32 SPCE_MASK          = 0x00000001;  // Bit 0
static constexpr quint32 SPCE_SHIFT         = 0;

// Multi-bit fields
static constexpr quint32 BP_MODE_MASK       = 0x00000C00;  // Bits 11:10
static constexpr quint32 BP_MODE_SHIFT      = 10;

static constexpr quint32 SBE_MASK           = 0x00000300;  // Bits 9:8
static constexpr quint32 SBE_SHIFT          = 8;

static constexpr quint32 SDE_MASK           = 0x000000C0;  // Bits 7:6
static constexpr quint32 SDE_SHIFT          = 6;

static constexpr quint32 SPE_MASK           = 0x00000038;  // Bits 5:3
static constexpr quint32 SPE_SHIFT          = 3;

static constexpr quint32 IC_EN_MASK         = 0x00000006;  // Bits 2:1
static constexpr quint32 IC_EN_SHIFT        = 1;

// Valid write mask (all defined bits)
static constexpr quint32 WRITE_MASK         = TB_MB_EN_MASK | MCHK_EN_MASK |
                                      CALL_PAL_R23_MASK | PCT1_EN_MASK |
                                      PCT0_EN_MASK | SINGLE_ISSUE_H_MASK |
                                      VA_FORM_32_MASK | VA_48_MASK |
                                      SL_RCV_MASK | SL_XMIT_MASK | HWE_MASK |
                                      BP_MODE_MASK | SBE_MASK | SDE_MASK |
                                      SPE_MASK | IC_EN_MASK | SPCE_MASK;
}

// ============================================================================
// Branch Prediction Modes
// ============================================================================

enum class BranchPredictionMode : quint8 {
    DISABLED    = 0,  // 00 - No branch prediction
    STATIC      = 1,  // 01 - Static prediction
    DYNAMIC_2BIT = 2, // 10 - 2-bit dynamic prediction
    DYNAMIC_3BIT = 3  // 11 - 3-bit dynamic prediction
};

// ============================================================================
// I_CTL Getters - Single Bit Flags
// ============================================================================

/**
 * @brief Get TB_MB_EN (Trap barrier / Memory barrier enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getTB_MB_EN(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::TB_MB_EN_MASK) != 0;
}

/**
 * @brief Get MCHK_EN (Machine check enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getMCHK_EN(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::MCHK_EN_MASK) != 0;
}

/**
 * @brief Get CALL_PAL_R23 (CALL_PAL uses R23 for return address)
 */
AXP_PURE AXP_FLATTEN
    inline bool getCALL_PAL_R23(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::CALL_PAL_R23_MASK) != 0;
}

/**
 * @brief Get PCT1_EN (Performance Counter 1 enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getPCT1_EN(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::PCT1_EN_MASK) != 0;
}

/**
 * @brief Get PCT0_EN (Performance Counter 0 enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getPCT0_EN(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::PCT0_EN_MASK) != 0;
}

/**
 * @brief Get SINGLE_ISSUE_H (Single issue mode hardware)
 */
AXP_PURE AXP_FLATTEN
    inline bool getSINGLE_ISSUE_H(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::SINGLE_ISSUE_H_MASK) != 0;
}

/**
 * @brief Get VA_FORM_32 (Virtual address format is 32-bit)
 */
AXP_PURE AXP_FLATTEN
    inline bool getVA_FORM_32(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::VA_FORM_32_MASK) != 0;
}

/**
 * @brief Get VA_48 (Virtual address is 48-bit)
 */
AXP_PURE AXP_FLATTEN
    inline bool getVA_48(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::VA_48_MASK) != 0;
}

/**
 * @brief Get SL_RCV (Serial line receive enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getSL_RCV(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::SL_RCV_MASK) != 0;
}

/**
 * @brief Get SL_XMIT (Serial line transmit enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getSL_XMIT(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::SL_XMIT_MASK) != 0;
}

/**
 * @brief Get HWE (Hardware error enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getHWE(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::HWE_MASK) != 0;
}

/**
 * @brief Get SPCE (Speculative cache enable)
 */
AXP_PURE AXP_FLATTEN
    inline bool getSPCE(quint32 iCtl) noexcept
{
    return (iCtl & I_CTL::SPCE_MASK) != 0;
}

// ============================================================================
// I_CTL Getters - Multi-bit Fields
// ============================================================================

/**
 * @brief Get BP_MODE (Branch prediction mode)
 */
AXP_PURE AXP_FLATTEN
    inline BranchPredictionMode getBP_MODE(quint32 iCtl) noexcept
{
    return static_cast<BranchPredictionMode>((iCtl & I_CTL::BP_MODE_MASK) >> I_CTL::BP_MODE_SHIFT);
}

/**
 * @brief Get BP_MODE as raw value
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getBP_MODE_Raw(quint32 iCtl) noexcept
{
    return static_cast<quint8>((iCtl & I_CTL::BP_MODE_MASK) >> I_CTL::BP_MODE_SHIFT);
}

/**
 * @brief Get SBE (Store buffer enable)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getSBE(quint32 iCtl) noexcept
{
    return static_cast<quint8>((iCtl & I_CTL::SBE_MASK) >> I_CTL::SBE_SHIFT);
}

/**
 * @brief Get SDE (Store data enable)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getSDE(quint32 iCtl) noexcept
{
    return static_cast<quint8>((iCtl & I_CTL::SDE_MASK) >> I_CTL::SDE_SHIFT);
}

/**
 * @brief Get SPE (Speculative execution enable)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getSPE(quint32 iCtl) noexcept
{
    return static_cast<quint8>((iCtl & I_CTL::SPE_MASK) >> I_CTL::SPE_SHIFT);
}

/**
 * @brief Get IC_EN (I-cache enable)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getIC_EN(quint32 iCtl) noexcept
{
    return static_cast<quint8>((iCtl & I_CTL::IC_EN_MASK) >> I_CTL::IC_EN_SHIFT);
}

// ============================================================================
// I_CTL Setters - Single Bit Flags
// ============================================================================

/**
 * @brief Set TB_MB_EN (Trap barrier / Memory barrier enable)
 */
AXP_FLATTEN
    inline void setTB_MB_EN(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::TB_MB_EN_MASK;
    } else {
        iCtl &= ~I_CTL::TB_MB_EN_MASK;
    }
}

/**
 * @brief Set MCHK_EN (Machine check enable)
 */
AXP_FLATTEN
    inline void setMCHK_EN(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::MCHK_EN_MASK;
    } else {
        iCtl &= ~I_CTL::MCHK_EN_MASK;
    }
}

/**
 * @brief Set CALL_PAL_R23
 */
AXP_FLATTEN
    inline void setCALL_PAL_R23(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::CALL_PAL_R23_MASK;
    } else {
        iCtl &= ~I_CTL::CALL_PAL_R23_MASK;
    }
}

/**
 * @brief Set PCT1_EN (Performance Counter 1 enable)
 */
AXP_FLATTEN
    inline void setPCT1_EN(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::PCT1_EN_MASK;
    } else {
        iCtl &= ~I_CTL::PCT1_EN_MASK;
    }
}

/**
 * @brief Set PCT0_EN (Performance Counter 0 enable)
 */
AXP_FLATTEN
    inline void setPCT0_EN(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::PCT0_EN_MASK;
    } else {
        iCtl &= ~I_CTL::PCT0_EN_MASK;
    }
}

/**
 * @brief Set SINGLE_ISSUE_H
 */
AXP_FLATTEN
    inline void setSINGLE_ISSUE_H(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::SINGLE_ISSUE_H_MASK;
    } else {
        iCtl &= ~I_CTL::SINGLE_ISSUE_H_MASK;
    }
}

/**
 * @brief Set VA_FORM_32
 */
AXP_FLATTEN
    inline void setVA_FORM_32(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::VA_FORM_32_MASK;
    } else {
        iCtl &= ~I_CTL::VA_FORM_32_MASK;
    }
}

/**
 * @brief Set VA_48
 */
AXP_FLATTEN
    inline void setVA_48(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::VA_48_MASK;
    } else {
        iCtl &= ~I_CTL::VA_48_MASK;
    }
}

/**
 * @brief Set SL_RCV (Serial line receive enable)
 */
AXP_FLATTEN
    inline void setSL_RCV(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::SL_RCV_MASK;
    } else {
        iCtl &= ~I_CTL::SL_RCV_MASK;
    }
}

/**
 * @brief Set SL_XMIT (Serial line transmit enable)
 */
AXP_FLATTEN
    inline void setSL_XMIT(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::SL_XMIT_MASK;
    } else {
        iCtl &= ~I_CTL::SL_XMIT_MASK;
    }
}

/**
 * @brief Set HWE (Hardware error enable)
 */
AXP_FLATTEN
    inline void setHWE(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::HWE_MASK;
    } else {
        iCtl &= ~I_CTL::HWE_MASK;
    }
}

/**
 * @brief Set SPCE (Speculative cache enable)
 */
AXP_FLATTEN
    inline void setSPCE(quint32& iCtl, bool enable) noexcept
{
    if (enable) {
        iCtl |= I_CTL::SPCE_MASK;
    } else {
        iCtl &= ~I_CTL::SPCE_MASK;
    }
}

// ============================================================================
// I_CTL Setters - Multi-bit Fields
// ============================================================================

/**
 * @brief Set BP_MODE (Branch prediction mode)
 */
AXP_FLATTEN
    inline void setBP_MODE(quint32& iCtl, BranchPredictionMode mode) noexcept
{
    iCtl = (iCtl & ~I_CTL::BP_MODE_MASK) |
           ((static_cast<quint32>(mode) << I_CTL::BP_MODE_SHIFT) & I_CTL::BP_MODE_MASK);
}

/**
 * @brief Set BP_MODE from raw value
 */
AXP_FLATTEN
    inline void setBP_MODE_Raw(quint32& iCtl, quint8 value) noexcept
{
    iCtl = (iCtl & ~I_CTL::BP_MODE_MASK) |
           (((value & 0x3) << I_CTL::BP_MODE_SHIFT) & I_CTL::BP_MODE_MASK);
}

/**
 * @brief Set SBE (Store buffer enable)
 */
AXP_FLATTEN
    inline void setSBE(quint32& iCtl, quint8 value) noexcept
{
    iCtl = (iCtl & ~I_CTL::SBE_MASK) |
           (((value & 0x3) << I_CTL::SBE_SHIFT) & I_CTL::SBE_MASK);
}

/**
 * @brief Set SDE (Store data enable)
 */
AXP_FLATTEN
    inline void setSDE(quint32& iCtl, quint8 value) noexcept
{
    iCtl = (iCtl & ~I_CTL::SDE_MASK) |
           (((value & 0x3) << I_CTL::SDE_SHIFT) & I_CTL::SDE_MASK);
}

/**
 * @brief Set SPE (Speculative execution enable)
 */
AXP_FLATTEN
    inline void setSPE(quint32& iCtl, quint8 value) noexcept
{
    iCtl = (iCtl & ~I_CTL::SPE_MASK) |
           (((value & 0x7) << I_CTL::SPE_SHIFT) & I_CTL::SPE_MASK);
}

/**
 * @brief Set IC_EN (I-cache enable)
 */
AXP_FLATTEN
    inline void setIC_EN(quint32& iCtl, quint8 value) noexcept
{
    iCtl = (iCtl & ~I_CTL::IC_EN_MASK) |
           (((value & 0x3) << I_CTL::IC_EN_SHIFT) & I_CTL::IC_EN_MASK);
}

// ============================================================================
// I_CTL Validation
// ============================================================================

/**
 * @brief Validate I_CTL value (check no reserved bits set)
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidI_CTL(quint32 iCtl) noexcept
{
    return (iCtl & ~I_CTL::WRITE_MASK) == 0;
}

/**
 * @brief Sanitize I_CTL value (clear reserved bits)
 */
AXP_FLATTEN
    inline quint32 sanitizeI_CTL(quint32 iCtl) noexcept
{
    return iCtl & I_CTL::WRITE_MASK;
}

// ============================================================================
// I_CTL Query Helpers
// ============================================================================

/**
 * @brief Check if branch prediction is enabled
 */
AXP_PURE AXP_FLATTEN
    inline bool isBranchPredictionEnabled(quint32 iCtl) noexcept
{
    return getBP_MODE(iCtl) != BranchPredictionMode::DISABLED;
}

/**
 * @brief Check if any performance counter is enabled
 */
AXP_PURE AXP_FLATTEN
    inline bool isAnyPerfCounterEnabled(quint32 iCtl) noexcept
{
    return getPCT0_EN(iCtl) || getPCT1_EN(iCtl);
}

/**
 * @brief Check if I-cache is enabled
 */
AXP_PURE AXP_FLATTEN
    inline bool isICacheEnabled(quint32 iCtl) noexcept
{
    return getIC_EN(iCtl) != 0;
}

/**
 * @brief Get virtual address width (32, 43, or 48 bits)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getVirtualAddressWidth(quint32 iCtl) noexcept
{
    if (getVA_FORM_32(iCtl)) return 32;
    if (getVA_48(iCtl)) return 48;
    return 43;  // Default
}

// ============================================================================
// I_CTL Display / Debug Helpers
// ============================================================================

/**
 * @brief Get branch prediction mode name
 */
AXP_PURE
    inline const char* getBranchPredictionModeName(BranchPredictionMode mode) noexcept
{
    switch (mode) {
    case BranchPredictionMode::DISABLED:    return "Disabled";
    case BranchPredictionMode::STATIC:      return "Static";
    case BranchPredictionMode::DYNAMIC_2BIT: return "Dynamic-2bit";
    case BranchPredictionMode::DYNAMIC_3BIT: return "Dynamic-3bit";
    default:                                 return "Unknown";
    }
}

/**
 * @brief Format I_CTL for debugging
 */
inline QString formatI_CTL(quint32 iCtl) noexcept
{
    QStringList parts;

    if (getTB_MB_EN(iCtl))      parts << "TB_MB";
    if (getMCHK_EN(iCtl))       parts << "MCHK";
    if (getCALL_PAL_R23(iCtl))  parts << "PAL_R23";
    if (getPCT1_EN(iCtl))       parts << "PCT1";
    if (getPCT0_EN(iCtl))       parts << "PCT0";
    if (getSINGLE_ISSUE_H(iCtl)) parts << "SINGLE";
    if (getVA_FORM_32(iCtl))    parts << "VA32";
    if (getVA_48(iCtl))         parts << "VA48";
    if (getSL_RCV(iCtl))        parts << "SL_RCV";
    if (getSL_XMIT(iCtl))       parts << "SL_XMIT";
    if (getHWE(iCtl))           parts << "HWE";
    if (getSPCE(iCtl))          parts << "SPCE";

    parts << QString("BP=%1").arg(getBranchPredictionModeName(getBP_MODE(iCtl)));
    parts << QString("SBE=%1").arg(getSBE(iCtl));
    parts << QString("IC=%1").arg(getIC_EN(iCtl));

    return QString("I_CTL[%1]").arg(parts.join(" "));
}

/**
 * @brief Format I_CTL with detailed breakdown
 */
inline QString formatI_CTL_Detailed(quint32 iCtl) noexcept
{
    QString result = QString("I_CTL=0x%1\n").arg(iCtl, 8, 16, QChar('0'));

    result += "  Control Flags:\n";
    result += QString("    TB_MB_EN[22]      = %1\n").arg(getTB_MB_EN(iCtl) ? 1 : 0);
    result += QString("    MCHK_EN[21]       = %1\n").arg(getMCHK_EN(iCtl) ? 1 : 0);
    result += QString("    CALL_PAL_R23[20]  = %1\n").arg(getCALL_PAL_R23(iCtl) ? 1 : 0);
    result += QString("    PCT1_EN[19]       = %1\n").arg(getPCT1_EN(iCtl) ? 1 : 0);
    result += QString("    PCT0_EN[18]       = %1\n").arg(getPCT0_EN(iCtl) ? 1 : 0);
    result += QString("    SINGLE_ISSUE_H[17] = %1\n").arg(getSINGLE_ISSUE_H(iCtl) ? 1 : 0);
    result += QString("    VA_FORM_32[16]    = %1\n").arg(getVA_FORM_32(iCtl) ? 1 : 0);
    result += QString("    VA_48[15]         = %1 (VA width=%2 bits)\n")
                  .arg(getVA_48(iCtl) ? 1 : 0)
                  .arg(getVirtualAddressWidth(iCtl));
    result += QString("    SL_RCV[14]        = %1\n").arg(getSL_RCV(iCtl) ? 1 : 0);
    result += QString("    SL_XMIT[13]       = %1\n").arg(getSL_XMIT(iCtl) ? 1 : 0);
    result += QString("    HWE[12]           = %1\n").arg(getHWE(iCtl) ? 1 : 0);
    result += QString("    SPCE[0]           = %1\n").arg(getSPCE(iCtl) ? 1 : 0);

    result += "  Multi-bit Fields:\n";
    result += QString("    BP_MODE[11:10]    = %1 (%2)\n")
                  .arg(getBP_MODE_Raw(iCtl), 2, 2, QChar('0'))
                  .arg(getBranchPredictionModeName(getBP_MODE(iCtl)));
    result += QString("    SBE[9:8]          = %1\n").arg(getSBE(iCtl), 2, 2, QChar('0'));
    result += QString("    SDE[7:6]          = %1\n").arg(getSDE(iCtl), 2, 2, QChar('0'));
    result += QString("    SPE[5:3]          = %1\n").arg(getSPE(iCtl), 3, 2, QChar('0'));
    result += QString("    IC_EN[2:1]        = %1\n").arg(getIC_EN(iCtl), 2, 2, QChar('0'));

    return result;
}

#endif // MASKED_I_CTL_INL_H


/*
 * USAGE:
 * // Enable branch prediction
quint32 iCtl = 0;
setBP_MODE(iCtl, BranchPredictionMode::DYNAMIC_2BIT);
setIC_EN(iCtl, 3);  // Full I-cache enable

// Enable performance counters
setPCT0_EN(iCtl, true);
setPCT1_EN(iCtl, true);

// Enable machine checks
setMCHK_EN(iCtl, true);

// Check features
if (isBranchPredictionEnabled(iCtl)) {
    // Branch prediction active
}

if (isAnyPerfCounterEnabled(iCtl)) {
    // Collect performance data
}

// Get VA width
quint8 vaWidth = getVirtualAddressWidth(iCtl);  // 32, 43, or 48

// Debug output
DEBUG_LOG(formatI_CTL(iCtl));
// Output: "I_CTL[PCT1 PCT0 MCHK BP=Dynamic-2bit SBE=0 IC=3]"
 *
 */
