#ifndef MASKED_I_STAT_INL_H
#define MASKED_I_STAT_INL_H

// ============================================================================
// Masked_I_STAT_inl.h
// ============================================================================
// Ibox Status Register (I_STAT) inline helpers
//
// I_STAT Register Layout (EV6):
// Bit  [30]    - DPE     D-cache Parity Error
// Bit  [29]    - TPE     Tag Parity Error
// Bits [28:0]  - Reserved (implementation-dependent status bits)
//
// I_STAT is primarily READ-ONLY and reflects hardware error conditions.
// Some implementations may allow write-1-to-clear for error bits.
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "../../types_core.h"
#include "Axp_Attributes_core.h"

// ============================================================================
// I_STAT Bit Masks
// ============================================================================

namespace I_STAT {
// D-cache Parity Error (bit 30)
static constexpr quint32 DPE_MASK        = 0x40000000;
static constexpr quint32 DPE_SHIFT       = 30;

// Tag Parity Error (bit 29)
static constexpr quint32 TPE_MASK        = 0x20000000;
static constexpr quint32 TPE_SHIFT       = 29;

// All defined error bits
static constexpr quint32 ERROR_MASK      = DPE_MASK | TPE_MASK;

// Reserved bits (implementation-dependent)
static constexpr quint32 RESERVED_MASK   = 0x1FFFFFFF;  // Bits 28:0

// Valid bits mask (defined architectural bits)
static constexpr quint32 VALID_MASK      = ERROR_MASK;
}

// ============================================================================
// I_STAT Getters
// ============================================================================

/**
 * @brief Get DPE (D-cache Parity Error) status
 */
AXP_PURE AXP_FLATTEN
    inline bool getDPE(quint32 iStat) noexcept
{
    return (iStat & I_STAT::DPE_MASK) != 0;
}

/**
 * @brief Get TPE (Tag Parity Error) status
 */
AXP_PURE AXP_FLATTEN
    inline bool getTPE(quint32 iStat) noexcept
{
    return (iStat & I_STAT::TPE_MASK) != 0;
}

/**
 * @brief Check if any error is present
 */
AXP_PURE AXP_FLATTEN
    inline bool isAnyErrorPresent(quint32 iStat) noexcept
{
    return (iStat & I_STAT::ERROR_MASK) != 0;
}

/**
 * @brief Get all error bits as mask
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getAllErrorBits(quint32 iStat) noexcept
{
    quint8 errors = 0;
    if (getDPE(iStat)) errors |= 0x02;  // Bit 1
    if (getTPE(iStat)) errors |= 0x01;  // Bit 0
    return errors;
}

/**
 * @brief Get reserved/implementation bits (bits 28:0)
 */
AXP_PURE AXP_FLATTEN
    inline quint32 getReservedBits(quint32 iStat) noexcept
{
    return iStat & I_STAT::RESERVED_MASK;
}

// ============================================================================
// I_STAT Setters (for hardware error reporting)
// ============================================================================

/**
 * @brief Set DPE (D-cache Parity Error)
 */
AXP_FLATTEN
    inline void setDPE(quint32& iStat, bool error) noexcept
{
    if (error) {
        iStat |= I_STAT::DPE_MASK;
    } else {
        iStat &= ~I_STAT::DPE_MASK;
    }
}

/**
 * @brief Set TPE (Tag Parity Error)
 */
AXP_FLATTEN
    inline void setTPE(quint32& iStat, bool error) noexcept
{
    if (error) {
        iStat |= I_STAT::TPE_MASK;
    } else {
        iStat &= ~I_STAT::TPE_MASK;
    }
}

/**
 * @brief Set multiple error bits from mask
 */
AXP_FLATTEN
    inline void setErrorBits(quint32& iStat, quint8 errorMask) noexcept
{
    setDPE(iStat, (errorMask & 0x02) != 0);
    setTPE(iStat, (errorMask & 0x01) != 0);
}

// ============================================================================
// I_STAT Clear Operations (Write-1-to-Clear semantics)
// ============================================================================

/**
 * @brief Clear DPE (D-cache Parity Error)
 */
AXP_FLATTEN
    inline void clearDPE(quint32& iStat) noexcept
{
    iStat &= ~I_STAT::DPE_MASK;
}

/**
 * @brief Clear TPE (Tag Parity Error)
 */
AXP_FLATTEN
    inline void clearTPE(quint32& iStat) noexcept
{
    iStat &= ~I_STAT::TPE_MASK;
}

/**
 * @brief Clear all error bits
 */
AXP_FLATTEN
    inline void clearAllErrors(quint32& iStat) noexcept
{
    iStat &= ~I_STAT::ERROR_MASK;
}

/**
 * @brief Apply write-1-to-clear operation
 * @param iStat Current I_STAT value (modified in place)
 * @param clearMask Value written to I_STAT (1s indicate bits to clear)
 */
AXP_FLATTEN
    inline void applyWrite1ToClear(quint32& iStat, quint32 clearMask) noexcept
{
    // Clear bits where clearMask has 1s
    iStat &= ~(clearMask & I_STAT::ERROR_MASK);
}

// ============================================================================
// I_STAT Validation
// ============================================================================

/**
 * @brief Validate I_STAT write value (only error bits can be written)
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidI_STAT_Write(quint32 value) noexcept
{
    // Only error bits should be set in write value
    return (value & ~I_STAT::ERROR_MASK) == 0;
}

/**
 * @brief Sanitize I_STAT write value (mask to valid bits)
 */
AXP_FLATTEN
    inline quint32 sanitizeI_STAT_Write(quint32 value) noexcept
{
    return value & I_STAT::ERROR_MASK;
}

// ============================================================================
// I_STAT Analysis Helpers
// ============================================================================

/**
 * @brief Count number of errors present
 */
AXP_PURE
    inline quint8 countErrors(quint32 iStat) noexcept
{
    quint8 count = 0;
    if (getDPE(iStat)) ++count;
    if (getTPE(iStat)) ++count;
    return count;
}

/**
 * @brief Check if hardware error should trigger machine check
 */
AXP_PURE AXP_FLATTEN
    inline bool shouldTriggerMachineCheck(quint32 iStat) noexcept
{
    // Either D-cache parity error or tag parity error should trigger MCHK
    return isAnyErrorPresent(iStat);
}

/**
 * @brief Get error type name for first error found
 */
AXP_PURE
    inline const char* getFirstErrorType(quint32 iStat) noexcept
{
    if (getDPE(iStat)) return "D-cache Parity Error";
    if (getTPE(iStat)) return "Tag Parity Error";
    return "No Error";
}

// ============================================================================
// I_STAT Display / Debug Helpers
// ============================================================================

/**
 * @brief Format I_STAT for debugging
 */
inline QString formatI_STAT(quint32 iStat) noexcept
{
    if (!isAnyErrorPresent(iStat)) {
        return QString("I_STAT[OK]");
    }

    QStringList errors;
    if (getDPE(iStat)) errors << "DPE";
    if (getTPE(iStat)) errors << "TPE";

    return QString("I_STAT[%1]").arg(errors.join(" "));
}

/**
 * @brief Format I_STAT with detailed breakdown
 */
inline QString formatI_STAT_Detailed(quint32 iStat) noexcept
{
    QString result = QString("I_STAT=0x%1\n").arg(iStat, 8, 16, QChar('0'));

    result += "  Error Status:\n";
    result += QString("    DPE[30] = %1 %2\n")
                  .arg(getDPE(iStat) ? 1 : 0)
                  .arg(getDPE(iStat) ? "(D-cache Parity Error)" : "");
    result += QString("    TPE[29] = %1 %2\n")
                  .arg(getTPE(iStat) ? 1 : 0)
                  .arg(getTPE(iStat) ? "(Tag Parity Error)" : "");

    if (getReservedBits(iStat) != 0) {
        result += QString("  Reserved[28:0] = 0x%1 (implementation-specific)\n")
        .arg(getReservedBits(iStat), 8, 16, QChar('0'));
    }

    result += QString("  Error Count: %1\n").arg(countErrors(iStat));
    result += QString("  Machine Check: %1\n")
                  .arg(shouldTriggerMachineCheck(iStat) ? "Required" : "Not Required");

    return result;
}

/**
 * @brief Format error summary for logging
 */
inline QString formatErrorSummary(quint32 iStat) noexcept
{
    if (!isAnyErrorPresent(iStat)) {
        return "No hardware errors detected";
    }

    QStringList details;

    if (getDPE(iStat)) {
        details << "D-cache parity error detected - data corruption possible";
    }

    if (getTPE(iStat)) {
        details << "Tag parity error detected - cache integrity compromised";
    }

    return QString("Hardware errors: %1").arg(details.join("; "));
}

// ============================================================================
// I_STAT Hardware Error Reporting
// ============================================================================

/**
 * @brief Report D-cache parity error
 */
inline void reportDCacheParityError(quint32& iStat) noexcept
{
    setDPE(iStat, true);
}

/**
 * @brief Report tag parity error
 */
inline void reportTagParityError(quint32& iStat) noexcept
{
    setTPE(iStat, true);
}

/**
 * @brief Report multiple hardware errors
 */
inline void reportHardwareErrors(quint32& iStat, bool dpe, bool tpe) noexcept
{
    if (dpe) setDPE(iStat, true);
    if (tpe) setTPE(iStat, true);
}

// ============================================================================
// I_STAT Machine Check Integration
// ============================================================================

/**
 * @brief Build machine check syndrome from I_STAT
 * @param iStat Current I_STAT value
 * @return Machine check syndrome value encoding the errors
 */
inline quint64 buildMachineCheckSyndrome(quint32 iStat) noexcept
{
    quint64 syndrome = 0;

    // Encode error types in syndrome
    if (getDPE(iStat)) {
        syndrome |= (1ULL << 0);  // D-cache parity error bit
    }

    if (getTPE(iStat)) {
        syndrome |= (1ULL << 1);  // Tag parity error bit
    }

    // Include raw I_STAT value for diagnosis
    syndrome |= (static_cast<quint64>(iStat) << 32);

    return syndrome;
}

/**
 * @brief Check if I_STAT errors require machine check handling
 * @param iStat Current I_STAT value
 * @param iCtl Current I_CTL value (for MCHK_EN check)
 * @return true if machine check should be delivered
 */
inline bool shouldDeliverMachineCheck(quint32 iStat, quint32 iCtl) noexcept
{
    // Check if errors present
    if (!isAnyErrorPresent(iStat)) {
        return false;
    }

    // Check if machine checks are enabled (I_CTL.MCHK_EN)
    // Assuming getMCHK_EN is available from I_CTL header
    // bool mchkEnabled = getMCHK_EN(iCtl);

    // For now, always deliver if errors present
    return true;
}

// ============================================================================
// I_STAT Atomic Operations (for SMP safety)
// ============================================================================

/**
 * @brief Atomically read I_STAT
 */
AXP_PURE AXP_FLATTEN
    inline quint32 atomicReadI_STAT(const std::atomic<quint32>& iStat) noexcept
{
    return iStat.load(std::memory_order_acquire);
}

/**
 * @brief Atomically set error bit
 */
AXP_FLATTEN
    inline void atomicSetError(std::atomic<quint32>& iStat, quint32 errorMask) noexcept
{
    iStat.fetch_or(errorMask & I_STAT::ERROR_MASK, std::memory_order_release);
}

/**
 * @brief Atomically clear error bit
 */
AXP_FLATTEN
    inline void atomicClearError(std::atomic<quint32>& iStat, quint32 clearMask) noexcept
{
    iStat.fetch_and(~(clearMask & I_STAT::ERROR_MASK), std::memory_order_release);
}

/**
 * @brief Atomically set DPE
 */
AXP_FLATTEN
    inline void atomicSetDPE(std::atomic<quint32>& iStat) noexcept
{
    atomicSetError(iStat, I_STAT::DPE_MASK);
}

/**
 * @brief Atomically set TPE
 */
AXP_FLATTEN
    inline void atomicSetTPE(std::atomic<quint32>& iStat) noexcept
{
    atomicSetError(iStat, I_STAT::TPE_MASK);
}

#endif // MASKED_I_STAT_INL_H
