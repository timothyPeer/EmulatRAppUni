#ifndef MASKED_HW_INT_CLR_INL_H
#define MASKED_HW_INT_CLR_INL_H

// ============================================================================
// Masked_HW_INT_CLR_inl.h
// ============================================================================
// Hardware Interrupt Clear Register (HW_INT_CLR) inline helpers
//
// HW_INT_CLR Register Layout (EV6):
// Bit  [32]    - SL       System Level Interrupt Clear
// Bit  [31]    - CR       Corrected Read Error Clear
// Bits [30:29] - PC       Performance Counter Clear (PC0, PC1)
// Bit  [28]    - MCHK_D   Machine Check Disable/Clear
// Bit  [26]    - FBTP     Force Bad Target Prediction Clear
//
// HW_INT_CLR is WRITE-ONLY. Writing 1 to a bit clears the corresponding
// interrupt source. Writing 0 has no effect.
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "Axp_Attributes_core.h"

// ============================================================================
// HW_INT_CLR Bit Masks
// ============================================================================

namespace HW_INT_CLR {
// System Level Interrupt Clear (bit 32)
static constexpr quint64 SL_MASK         = 0x0000000100000000ULL;
static constexpr quint64 SL_SHIFT        = 32;

// Corrected Read Error Clear (bit 31)
static constexpr quint64 CR_MASK         = 0x0000000080000000ULL;
static constexpr quint64 CR_SHIFT        = 31;

// Performance Counter Clear (bits 30:29)
static constexpr quint64 PC_MASK         = 0x0000000060000000ULL;
static constexpr quint64 PC_SHIFT        = 29;
static constexpr quint64 PC0_MASK        = 0x0000000020000000ULL;  // Bit 29
static constexpr quint64 PC1_MASK        = 0x0000000040000000ULL;  // Bit 30

// Machine Check Disable/Clear (bit 28)
static constexpr quint64 MCHK_D_MASK     = 0x0000000010000000ULL;
static constexpr quint64 MCHK_D_SHIFT    = 28;

// Force Bad Target Prediction Clear (bit 26)
static constexpr quint64 FBTP_MASK       = 0x0000000004000000ULL;
static constexpr quint64 FBTP_SHIFT      = 26;

// Valid write mask (all writable bits)
static constexpr quint64 WRITE_MASK      = SL_MASK | CR_MASK | PC_MASK |
                                      MCHK_D_MASK | FBTP_MASK;
}

// ============================================================================
// HW_INT_CLR Write-1-to-Clear Setters
// ============================================================================

/**
 * @brief Clear System Level Interrupt
 */
AXP_FLATTEN
    inline void clearSL(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::SL_MASK;
}

/**
 * @brief Clear Corrected Read Error
 */
AXP_FLATTEN
    inline void clearCR(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::CR_MASK;
}

/**
 * @brief Clear Performance Counter 0 interrupt
 */
AXP_FLATTEN
    inline void clearPC0(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::PC0_MASK;
}

/**
 * @brief Clear Performance Counter 1 interrupt
 */
AXP_FLATTEN
    inline void clearPC1(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::PC1_MASK;
}

/**
 * @brief Clear both Performance Counter interrupts
 */
AXP_FLATTEN
    inline void clearAllPC(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::PC_MASK;
}

/**
 * @brief Clear specific Performance Counter interrupt
 * @param counter Performance counter number (0 or 1)
 */
AXP_FLATTEN
    inline void clearPC(quint64& hwIntClr, quint8 counter) noexcept
{
    Q_ASSERT(counter <= 1);
    if (counter == 0) {
        clearPC0(hwIntClr);
    } else {
        clearPC1(hwIntClr);
    }
}

/**
 * @brief Clear/Disable Machine Check
 */
AXP_FLATTEN
    inline void clearMCHK_D(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::MCHK_D_MASK;
}

/**
 * @brief Clear Force Bad Target Prediction
 */
AXP_FLATTEN
    inline void clearFBTP(quint64& hwIntClr) noexcept
{
    hwIntClr |= HW_INT_CLR::FBTP_MASK;
}

// ============================================================================
// HW_INT_CLR Bulk Operations
// ============================================================================

/**
 * @brief Clear multiple interrupts from bitmask
 * @param hwIntClr Value to write to HW_INT_CLR register
 * @param mask Bitmask of interrupts to clear
 */
AXP_FLATTEN
    inline void clearInterrupts(quint64& hwIntClr, quint64 mask) noexcept
{
    hwIntClr |= (mask & HW_INT_CLR::WRITE_MASK);
}

/**
 * @brief Clear all clearable interrupts
 */
AXP_FLATTEN
    inline void clearAllInterrupts(quint64& hwIntClr) noexcept
{
    hwIntClr = HW_INT_CLR::WRITE_MASK;
}

/**
 * @brief Build HW_INT_CLR value from individual flags
 */
AXP_FLATTEN
    inline quint64 buildHW_INT_CLR(bool clearSL,
                    bool clearCR,
                    bool clearPC0,
                    bool clearPC1,
                    bool clearMCHK_D,
                    bool clearFBTP) noexcept
{
    quint64 value = 0;

    if (clearSL)     value |= HW_INT_CLR::SL_MASK;
    if (clearCR)     value |= HW_INT_CLR::CR_MASK;
    if (clearPC0)    value |= HW_INT_CLR::PC0_MASK;
    if (clearPC1)    value |= HW_INT_CLR::PC1_MASK;
    if (clearMCHK_D) value |= HW_INT_CLR::MCHK_D_MASK;
    if (clearFBTP)   value |= HW_INT_CLR::FBTP_MASK;

    return value;
}

// ============================================================================
// HW_INT_CLR Query Helpers (for write value validation)
// ============================================================================

/**
 * @brief Check if SL clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isSLClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::SL_MASK) != 0;
}

/**
 * @brief Check if CR clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isCRClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::CR_MASK) != 0;
}

/**
 * @brief Check if PC0 clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isPC0ClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::PC0_MASK) != 0;
}

/**
 * @brief Check if PC1 clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isPC1ClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::PC1_MASK) != 0;
}

/**
 * @brief Check if MCHK_D clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isMCHK_DClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::MCHK_D_MASK) != 0;
}

/**
 * @brief Check if FBTP clear is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isFBTPClearRequested(quint64 hwIntClr) noexcept
{
    return (hwIntClr & HW_INT_CLR::FBTP_MASK) != 0;
}

/**
 * @brief Get Performance Counter clear mask
 * @return 2-bit mask: bit 0 = PC0, bit 1 = PC1
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getPCClearMask(quint64 hwIntClr) noexcept
{
    return static_cast<quint8>((hwIntClr & HW_INT_CLR::PC_MASK) >> HW_INT_CLR::PC_SHIFT);
}

// ============================================================================
// HW_INT_CLR Validation
// ============================================================================

/**
 * @brief Validate HW_INT_CLR write value
 * @return true if value only sets valid bits
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidHW_INT_CLR(quint64 hwIntClr) noexcept
{
    return (hwIntClr & ~HW_INT_CLR::WRITE_MASK) == 0;
}

/**
 * @brief Sanitize HW_INT_CLR value (mask invalid bits)
 */
AXP_FLATTEN
    inline quint64 sanitizeHW_INT_CLR(quint64 hwIntClr) noexcept
{
    return hwIntClr & HW_INT_CLR::WRITE_MASK;
}

// ============================================================================
// HW_INT_CLR Application (Clear interrupts from ISUM)
// ============================================================================

/**
 * @brief Apply HW_INT_CLR to ISUM (clear requested interrupts)
 * @param isum Current ISUM value (modified in place)
 * @param hwIntClr HW_INT_CLR write value
 */
inline void applyHW_INT_CLR_ToISUM(quint64& isum, quint64 hwIntClr) noexcept
{
    // Write-1-to-clear semantics: clear bits in ISUM that are set in hwIntClr

    if (isSLClearRequested(hwIntClr)) {
        // Clear System Level interrupt in ISUM (bit 32)
        isum &= ~0x0000000100000000ULL;
    }

    if (isCRClearRequested(hwIntClr)) {
        // Clear Corrected Read Error in ISUM (bit 31)
        isum &= ~0x0000000080000000ULL;
    }

    if (isPC0ClearRequested(hwIntClr)) {
        // Clear PC0 in ISUM (bit 29)
        isum &= ~0x0000000020000000ULL;
    }

    if (isPC1ClearRequested(hwIntClr)) {
        // Clear PC1 in ISUM (bit 30)
        isum &= ~0x0000000040000000ULL;
    }

    // MCHK_D and FBTP don't directly clear ISUM bits
    // They control machine check behavior and branch prediction
}

// ============================================================================
// HW_INT_CLR Display / Debug Helpers
// ============================================================================

/**
 * @brief Format HW_INT_CLR for debugging
 */
inline QString formatHW_INT_CLR(quint64 hwIntClr) noexcept
{
    QStringList clears;

    if (isSLClearRequested(hwIntClr))     clears << "SL";
    if (isCRClearRequested(hwIntClr))     clears << "CR";
    if (isPC0ClearRequested(hwIntClr))    clears << "PC0";
    if (isPC1ClearRequested(hwIntClr))    clears << "PC1";
    if (isMCHK_DClearRequested(hwIntClr)) clears << "MCHK_D";
    if (isFBTPClearRequested(hwIntClr))   clears << "FBTP";

    return QString("HW_INT_CLR[%1]").arg(clears.isEmpty() ? "none" : clears.join(" "));
}

/**
 * @brief Format HW_INT_CLR with bit positions
 */
inline QString formatHW_INT_CLR_Detailed(quint64 hwIntClr) noexcept
{
    QString result = QString("HW_INT_CLR=0x%1\n").arg(hwIntClr, 16, 16, QChar('0'));

    result += QString("  SL[32]     = %1\n").arg(isSLClearRequested(hwIntClr) ? 1 : 0);
    result += QString("  CR[31]     = %1\n").arg(isCRClearRequested(hwIntClr) ? 1 : 0);
    result += QString("  PC0[29]    = %1\n").arg(isPC0ClearRequested(hwIntClr) ? 1 : 0);
    result += QString("  PC1[30]    = %1\n").arg(isPC1ClearRequested(hwIntClr) ? 1 : 0);
    result += QString("  MCHK_D[28] = %1\n").arg(isMCHK_DClearRequested(hwIntClr) ? 1 : 0);
    result += QString("  FBTP[26]   = %1\n").arg(isFBTPClearRequested(hwIntClr) ? 1 : 0);

    return result;
}

/**
 * @brief Get count of clear requests
 */
AXP_PURE
    inline quint8 countClearRequests(quint64 hwIntClr) noexcept
{
    quint8 count = 0;

    if (isSLClearRequested(hwIntClr))     ++count;
    if (isCRClearRequested(hwIntClr))     ++count;
    if (isPC0ClearRequested(hwIntClr))    ++count;
    if (isPC1ClearRequested(hwIntClr))    ++count;
    if (isMCHK_DClearRequested(hwIntClr)) ++count;
    if (isFBTPClearRequested(hwIntClr))   ++count;

    return count;
}

// ============================================================================
// MTPR_HW_INT_CLR Helper
// ============================================================================

/**
 * @brief Process HW_INT_CLR write (clear interrupts, update state)
 * @param hwIntClr Value written to HW_INT_CLR register
 * @param isum Current ISUM value (will be updated)
 * @param mchkDisabled Machine check disabled flag (will be updated)
 */
inline void processHW_INT_CLR_Write(quint64 hwIntClr,
                                    quint64& isum,
                                    bool& mchkDisabled) noexcept
{
    // Sanitize input
    hwIntClr = sanitizeHW_INT_CLR(hwIntClr);

    // Apply clears to ISUM
    applyHW_INT_CLR_ToISUM(isum, hwIntClr);

    // Handle MCHK_D (machine check disable/clear)
    if (isMCHK_DClearRequested(hwIntClr)) {
        mchkDisabled = true;  // Disable machine checks
    }

    // Handle FBTP (force bad target prediction clear)
    if (isFBTPClearRequested(hwIntClr)) {
        // Clear branch predictor state (implementation-specific)
        // In functional emulator, this is typically a no-op
    }
}

#endif // MASKED_HW_INT_CLR_INL_H


/*
 * USAGE:
 *
 * // Clear specific interrupts
quint64 hwIntClr = 0;
clearCR(hwIntClr);           // Clear corrected read error
clearPC0(hwIntClr);          // Clear performance counter 0
writeHW_INT_CLR_Active(cpuId, hwIntClr);

// Clear all interrupts
quint64 clearAll = 0;
clearAllInterrupts(clearAll);
writeHW_INT_CLR_Active(cpuId, clearAll);

// Build from flags
quint64 value = buildHW_INT_CLR(
    true,   // Clear SL
    true,   // Clear CR
    false,  // Don't clear PC0
    true,   // Clear PC1
    false,  // Don't clear MCHK_D
    false   // Don't clear FBTP
);

// Apply to ISUM
quint64 isum = getISUM_Active(cpuId);
bool mchkDisabled = false;
processHW_INT_CLR_Write(hwIntClr, isum, mchkDisabled);
setISUM_Active(cpuId, isum);

// Debug output
DEBUG_LOG(formatHW_INT_CLR(hwIntClr));
// Output: "HW_INT_CLR[CR PC0]"
 *
 *
 */
