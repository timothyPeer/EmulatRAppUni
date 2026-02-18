#ifndef MASKED_SIRR_INL_H
#define MASKED_SIRR_INL_H

// ============================================================================
// Masked_SIRR_inl.h
// ============================================================================
// Software Interrupt Request Register (SIRR) inline helpers
//
// SIRR Register Layout (EV6):
// Bits [15:1]  - SIR    Software Interrupt Request (IPL 1-15)
// Bit  [0]     - Reserved (always 0)
//
// Writing 1 to SIR[N] requests a software interrupt at IPL N.
// Reading SISR returns the pending software interrupt bitmap.
//
// Note: SIRR is write-only, SISR is read-only (same bits, different access)
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "Axp_Attributes_core.h"

// ============================================================================
// SIRR Bit Masks
// ============================================================================

namespace SIRR {
// Software Interrupt Request (bits 15:1)
static constexpr quint16 SIR_MASK        = 0xFFFE;  // Bits 15:1
static constexpr quint16 SIR_SHIFT       = 1;
static constexpr quint16 RESERVED_MASK   = 0x0001;  // Bit 0 (reserved)

// Valid write mask (only bits 15:1)
static constexpr quint16 WRITE_MASK      = SIR_MASK;

// IPL range for software interrupts
static constexpr quint8 MIN_IPL          = 1;
static constexpr quint8 MAX_IPL          = 15;
}

// ============================================================================
// SIRR Getters (for SISR read operations)
// ============================================================================

/**
 * @brief Get full Software Interrupt Request bitmap (bits 15:1)
 */
AXP_PURE AXP_FLATTEN
    inline quint16 getSIR(quint16 sirr) noexcept
{
    return sirr & SIRR::SIR_MASK;
}

/**
 * @brief Check if software interrupt at IPL is requested
 */
AXP_PURE AXP_FLATTEN
    inline bool isSoftwareInterruptRequested(quint16 sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= SIRR::MIN_IPL && ipl <= SIRR::MAX_IPL);
    return (sirr & (1u << ipl)) != 0;
}

/**
 * @brief Get highest priority pending software interrupt
 * @param sirr Current SIRR/SISR value
 * @param[out] ipl Highest priority IPL with pending interrupt
 * @return true if any software interrupt is pending
 */
AXP_PURE AXP_FLATTEN
    inline bool getHighestPendingSoftwareInterrupt(quint16 sirr, quint8& ipl) noexcept
{
    // Find highest set bit (highest priority)
    for (int i = SIRR::MAX_IPL; i >= SIRR::MIN_IPL; --i) {
        if (sirr & (1u << i)) {
            ipl = static_cast<quint8>(i);
            return true;
        }
    }
    return false;
}

/**
 * @brief Count pending software interrupts
 */
AXP_PURE AXP_FLATTEN
    inline quint8 countPendingSoftwareInterrupts(quint16 sirr) noexcept
{
    quint8 count = 0;
    for (int i = SIRR::MIN_IPL; i <= SIRR::MAX_IPL; ++i) {
        if (sirr & (1u << i)) {
            ++count;
        }
    }
    return count;
}

/**
 * @brief Get pending software interrupts above current IPL
 * @param sirr Current SIRR/SISR value
 * @param currentIPL Current processor IPL
 * @return Bitmap of pending interrupts above current IPL
 */
AXP_PURE AXP_FLATTEN
    inline quint16 getPendingAboveIPL(quint16 sirr, quint8 currentIPL) noexcept
{
    // Mask off interrupts at or below current IPL
    quint16 mask = (currentIPL < 15) ? ~((1u << (currentIPL + 1)) - 1) : 0;
    return sirr & mask;
}

// ============================================================================
// SIRR Setters (for SIRR write operations)
// ============================================================================

/**
 * @brief Set full Software Interrupt Request bitmap
 */
AXP_FLATTEN
    inline void setSIR(quint16& sirr, quint16 value) noexcept
{
    sirr = value & SIRR::WRITE_MASK;
}

/**
 * @brief Request software interrupt at IPL
 */
AXP_FLATTEN
    inline void requestSoftwareInterrupt(quint16& sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= SIRR::MIN_IPL && ipl <= SIRR::MAX_IPL);
    sirr |= (1u << ipl);
}

/**
 * @brief Clear software interrupt at IPL
 */
AXP_FLATTEN
    inline void clearSoftwareInterrupt(quint16& sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= SIRR::MIN_IPL && ipl <= SIRR::MAX_IPL);
    sirr &= ~(1u << ipl);
}

/**
 * @brief Clear all software interrupts
 */
AXP_FLATTEN
    inline void clearAllSoftwareInterrupts(quint16& sirr) noexcept
{
    sirr = 0;
}

/**
 * @brief Clear software interrupts at or below IPL
 */
AXP_FLATTEN
    inline void clearSoftwareInterruptsBelowIPL(quint16& sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl <= SIRR::MAX_IPL);
    quint16 mask = ~((1u << (ipl + 1)) - 1);
    sirr &= mask;
}

/**
 * @brief Request multiple software interrupts from bitmap
 */
AXP_FLATTEN
    inline void requestSoftwareInterruptsBitmap(quint16& sirr, quint16 bitmap) noexcept
{
    sirr |= (bitmap & SIRR::WRITE_MASK);
}

/**
 * @brief Clear multiple software interrupts from bitmap
 */
AXP_FLATTEN
    inline void clearSoftwareInterruptsBitmap(quint16& sirr, quint16 bitmap) noexcept
{
    sirr &= ~(bitmap & SIRR::WRITE_MASK);
}

// ============================================================================
// SIRR Validation
// ============================================================================

/**
 * @brief Validate SIRR value (check reserved bits)
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidSIRR(quint16 sirr) noexcept
{
    // Bit 0 must be 0 (reserved)
    return (sirr & SIRR::RESERVED_MASK) == 0;
}

/**
 * @brief Sanitize SIRR value (clear reserved bits)
 */
AXP_FLATTEN
    inline quint16 sanitizeSIRR(quint16 sirr) noexcept
{
    return sirr & SIRR::WRITE_MASK;
}

// ============================================================================
// SIRR/SISR Iteration Helpers
// ============================================================================

/**
 * @brief Iterate over all pending software interrupts
 * @param sirr Current SIRR/SISR value
 * @param callback Function called for each pending IPL: void(quint8 ipl)
 */
template<typename Callback>
inline void forEachPendingSoftwareInterrupt(quint16 sirr, Callback&& callback) noexcept
{
    for (int ipl = SIRR::MIN_IPL; ipl <= SIRR::MAX_IPL; ++ipl) {
        if (sirr & (1u << ipl)) {
            callback(static_cast<quint8>(ipl));
        }
    }
}

/**
 * @brief Iterate over pending software interrupts in priority order (high to low)
 * @param sirr Current SIRR/SISR value
 * @param callback Function called for each pending IPL: void(quint8 ipl)
 */
template<typename Callback>
inline void forEachPendingSoftwareInterruptByPriority(quint16 sirr, Callback&& callback) noexcept
{
    for (int ipl = SIRR::MAX_IPL; ipl >= SIRR::MIN_IPL; --ipl) {
        if (sirr & (1u << ipl)) {
            callback(static_cast<quint8>(ipl));
        }
    }
}

// ============================================================================
// SIRR Display / Debug Helpers
// ============================================================================

/**
 * @brief Format SIRR for debugging
 */
inline QString formatSIRR(quint16 sirr) noexcept
{
    QString result = QString("SIRR[0x%1:").arg(sirr, 4, 16, QChar('0'));

    bool first = true;
    for (int ipl = SIRR::MAX_IPL; ipl >= SIRR::MIN_IPL; --ipl) {
        if (sirr & (1u << ipl)) {
            if (!first) result += ",";
            result += QString(" IPL%1").arg(ipl);
            first = false;
        }
    }

    if (first) {
        result += " none";
    }

    result += "]";
    return result;
}

/**
 * @brief Format SISR for debugging (same format as SIRR)
 */
inline QString formatSISR(quint16 sisr) noexcept
{
    return formatSIRR(sisr).replace("SIRR", "SISR");
}

/**
 * @brief Get binary representation of SIRR
 */
inline QString formatSIRRBinary(quint16 sirr) noexcept
{
    QString result;
    for (int i = 15; i >= 0; --i) {
        result += (sirr & (1u << i)) ? '1' : '0';
        if (i > 0 && i % 4 == 0) result += ' ';
    }
    return result;
}

/**
 * @brief Get list of pending IPLs as string
 */
inline QString getPendingIPLsList(quint16 sirr) noexcept
{
    QStringList ipls;
    for (int ipl = SIRR::MIN_IPL; ipl <= SIRR::MAX_IPL; ++ipl) {
        if (sirr & (1u << ipl)) {
            ipls << QString::number(ipl);
        }
    }
    return ipls.isEmpty() ? "none" : ipls.join(", ");
}

// ============================================================================
// Integration with IRQ Controller
// ============================================================================

/**
 * @brief Convert SIRR to IRQ controller pendingIPLMask format
 * SIRR uses bits [15:1] for IPL 1-15
 * pendingIPLMask uses bit N for IPL N
 */
AXP_PURE AXP_FLATTEN
    inline quint32 sirrToPendingIPLMask(quint16 sirr) noexcept
{
    // SIRR already has correct bit positions (IPL N at bit N)
    return static_cast<quint32>(sirr & SIRR::SIR_MASK);
}

/**
 * @brief Convert pendingIPLMask to SIRR format
 */
AXP_PURE AXP_FLATTEN
    inline quint16 pendingIPLMaskToSIRR(quint32 mask) noexcept
{
    // Extract software interrupt range (IPL 1-15)
    return static_cast<quint16>(mask & SIRR::SIR_MASK);
}

// ============================================================================
// Atomic Operations (for concurrent access)
// ============================================================================

/**
 * @brief Atomically request software interrupt
 */
inline void atomicRequestSoftwareInterrupt(std::atomic<quint16>& sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= SIRR::MIN_IPL && ipl <= SIRR::MAX_IPL);
    sirr.fetch_or(1u << ipl, std::memory_order_release);
}

/**
 * @brief Atomically clear software interrupt
 */
inline void atomicClearSoftwareInterrupt(std::atomic<quint16>& sirr, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= SIRR::MIN_IPL && ipl <= SIRR::MAX_IPL);
    sirr.fetch_and(~(1u << ipl), std::memory_order_release);
}

/**
 * @brief Atomically read SIRR/SISR
 */
AXP_PURE AXP_FLATTEN
    inline quint16 atomicReadSIRR(const std::atomic<quint16>& sirr) noexcept
{
    return sirr.load(std::memory_order_acquire);
}

/**
 * @brief Atomically write SIRR
 */
AXP_FLATTEN
    inline void atomicWriteSIRR(std::atomic<quint16>& sirr, quint16 value) noexcept
{
    sirr.store(value & SIRR::WRITE_MASK, std::memory_order_release);
}

#endif // MASKED_SIRR_INL_H

/*
 * Usage:
 *
 * // Request software interrupt at IPL 5
quint16 sirr = 0;
requestSoftwareInterrupt(sirr, 5);

// Check if interrupt at IPL 7 is pending
if (isSoftwareInterruptRequested(sirr, 7)) {
    // Process interrupt
}

// Find highest priority interrupt
quint8 ipl;
if (getHighestPendingSoftwareInterrupt(sirr, ipl)) {
    deliverSoftwareInterrupt(cpuId, ipl);
}

// Clear interrupt after delivery
clearSoftwareInterrupt(sirr, ipl);

// Atomic operations for concurrent access
std::atomic<quint16> atomicSIRR{0};
atomicRequestSoftwareInterrupt(atomicSIRR, 10);
quint16 current = atomicReadSIRR(atomicSIRR);

// Debug output
DEBUG_LOG(formatSIRR(sirr));  // "SIRR[0x0020: IPL5]"
DEBUG_LOG(getPendingIPLsList(sirr));  // "5"
 *
 */
