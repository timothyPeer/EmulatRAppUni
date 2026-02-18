#ifndef MASKED_IER_CM_INL_H
#define MASKED_IER_CM_INL_H

// ============================================================================
// Masked_IER_CM_inl.h
// ============================================================================
// Interrupt Enable Register (IER) and Current Mode (CM) inline helpers
//
// IER Register Layout (EV6):
// Bits [5:0]   - EIEN   External Interrupt Enable (6 external interrupt lines)
// Bit  [32]    - SLEN   System Level Enable
// Bit  [31]    - CREN   Corrected Read Error Enable
// Bits [30:29] - PCEN   Performance Counter Enable (2 counters)
// Bits [15:1]  - SIEN   Software Interrupt Enable (IPL 1-15)
// Bit  [13]    - ASTEN  AST Enable
// Bits [1:0]   - CM     Current Mode (00=Kernel, 01=Executive, 10=Supervisor, 11=User)
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "types_core.h"
#include "Axp_Attributes_core.h"

// ============================================================================
// IER Bit Masks
// ============================================================================

namespace IER {
// External Interrupt Enable (bits 5:0)
static constexpr quint64 EIEN_MASK       = 0x000000000000003FULL;
static constexpr quint64 EIEN_SHIFT      = 0;

// Software Interrupt Enable (bits 15:1)
static constexpr quint64 SIEN_MASK       = 0x000000000000FFFEULL;
static constexpr quint64 SIEN_SHIFT      = 1;

// AST Enable (bit 13)
static constexpr quint64 ASTEN_MASK      = 0x0000000000002000ULL;
static constexpr quint64 ASTEN_SHIFT     = 13;

// Performance Counter Enable (bits 30:29)
static constexpr quint64 PCEN_MASK       = 0x0000000060000000ULL;
static constexpr quint64 PCEN_SHIFT      = 29;

// Corrected Read Error Enable (bit 31)
static constexpr quint64 CREN_MASK       = 0x0000000080000000ULL;
static constexpr quint64 CREN_SHIFT      = 31;

// System Level Enable (bit 32)
static constexpr quint64 SLEN_MASK       = 0x0000000100000000ULL;
static constexpr quint64 SLEN_SHIFT      = 32;

// Current Mode (bits 1:0) - stored in PS, not IER
static constexpr quint64 CM_MASK         = 0x0000000000000003ULL;
static constexpr quint64 CM_SHIFT        = 0;

// Combined write mask (all writable bits)
static constexpr quint64 WRITE_MASK      = EIEN_MASK | SIEN_MASK | ASTEN_MASK |
                                      PCEN_MASK | CREN_MASK | SLEN_MASK;
}

// ============================================================================
// Current Mode (CM) Values
// ============================================================================

enum class CurrentMode : quint8 {
    KERNEL      = 0,  // 00
    EXECUTIVE   = 1,  // 01
    SUPERVISOR  = 2,  // 10
    USER        = 3   // 11
};

// ============================================================================
// IER Getters
// ============================================================================

/**
 * @brief Get External Interrupt Enable mask (bits 5:0)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getEIEN(quint64 ier) noexcept
{
    return static_cast<quint8>((ier & IER::EIEN_MASK) >> IER::EIEN_SHIFT);
}

/**
 * @brief Get Software Interrupt Enable mask (bits 15:1)
 */
AXP_PURE AXP_FLATTEN
    inline quint16 getSIEN(quint64 ier) noexcept
{
    return static_cast<quint16>((ier & IER::SIEN_MASK) >> IER::SIEN_SHIFT);
}

/**
 * @brief Get AST Enable (bit 13)
 */
AXP_PURE AXP_FLATTEN
    inline bool getASTEN(quint64 ier) noexcept
{
    return (ier & IER::ASTEN_MASK) != 0;
}

/**
 * @brief Get Performance Counter Enable (bits 30:29)
 */
AXP_PURE AXP_FLATTEN
    inline quint8 getPCEN(quint64 ier) noexcept
{
    return static_cast<quint8>((ier & IER::PCEN_MASK) >> IER::PCEN_SHIFT);
}

/**
 * @brief Get Corrected Read Error Enable (bit 31)
 */
AXP_PURE AXP_FLATTEN
    inline bool getCREN(quint64 ier) noexcept
{
    return (ier & IER::CREN_MASK) != 0;
}

/**
 * @brief Get System Level Enable (bit 32)
 */
AXP_PURE AXP_FLATTEN
    inline bool getSLEN(quint64 ier) noexcept
{
    return (ier & IER::SLEN_MASK) != 0;
}

/**
 * @brief Check if external interrupt line is enabled
 */
AXP_PURE AXP_FLATTEN
    inline bool isExternalInterruptEnabled(quint64 ier, quint8 line) noexcept
{
    Q_ASSERT(line < 6);
    return (ier & (1ULL << line)) != 0;
}

/**
 * @brief Check if software interrupt at IPL is enabled
 */
AXP_PURE AXP_FLATTEN
    inline bool isSoftwareInterruptEnabled(quint64 ier, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= 1 && ipl <= 15);
    return (ier & (1ULL << ipl)) != 0;
}

// ============================================================================
// IER Setters
// ============================================================================

/**
 * @brief Set External Interrupt Enable mask (bits 5:0)
 */
AXP_FLATTEN
    inline void setEIEN(quint64& ier, quint8 value) noexcept
{
    ier = (ier & ~IER::EIEN_MASK) | ((static_cast<quint64>(value & 0x3F) << IER::EIEN_SHIFT) & IER::EIEN_MASK);
}

/**
 * @brief Set Software Interrupt Enable mask (bits 15:1)
 */
AXP_FLATTEN
    inline void setSIEN(quint64& ier, quint16 value) noexcept
{
    ier = (ier & ~IER::SIEN_MASK) | ((static_cast<quint64>(value) << IER::SIEN_SHIFT) & IER::SIEN_MASK);
}

/**
 * @brief Set AST Enable (bit 13)
 */
AXP_FLATTEN
    inline void setASTEN(quint64& ier, bool enable) noexcept
{
    if (enable) {
        ier |= IER::ASTEN_MASK;
    } else {
        ier &= ~IER::ASTEN_MASK;
    }
}

/**
 * @brief Set Performance Counter Enable (bits 30:29)
 */
AXP_FLATTEN
    inline void setPCEN(quint64& ier, quint8 value) noexcept
{
    ier = (ier & ~IER::PCEN_MASK) | ((static_cast<quint64>(value & 0x3) << IER::PCEN_SHIFT) & IER::PCEN_MASK);
}

/**
 * @brief Set Corrected Read Error Enable (bit 31)
 */
AXP_FLATTEN
    inline void setCREN(quint64& ier, bool enable) noexcept
{
    if (enable) {
        ier |= IER::CREN_MASK;
    } else {
        ier &= ~IER::CREN_MASK;
    }
}

/**
 * @brief Set System Level Enable (bit 32)
 */
AXP_FLATTEN
    inline void setSLEN(quint64& ier, bool enable) noexcept
{
    if (enable) {
        ier |= IER::SLEN_MASK;
    } else {
        ier &= ~IER::SLEN_MASK;
    }
}

/**
 * @brief Enable external interrupt line
 */
AXP_FLATTEN
    inline void enableExternalInterrupt(quint64& ier, quint8 line) noexcept
{
    Q_ASSERT(line < 6);
    ier |= (1ULL << line);
}

/**
 * @brief Disable external interrupt line
 */
AXP_FLATTEN
    inline void disableExternalInterrupt(quint64& ier, quint8 line) noexcept
{
    Q_ASSERT(line < 6);
    ier &= ~(1ULL << line);
}

/**
 * @brief Enable software interrupt at IPL
 */
AXP_FLATTEN
    inline void enableSoftwareInterrupt(quint64& ier, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= 1 && ipl <= 15);
    ier |= (1ULL << ipl);
}

/**
 * @brief Disable software interrupt at IPL
 */
AXP_FLATTEN
    inline void disableSoftwareInterrupt(quint64& ier, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= 1 && ipl <= 15);
    ier &= ~(1ULL << ipl);
}

/**
 * @brief Write full IER value (masked)
 */
AXP_FLATTEN
    inline void writeIER(quint64& ier, quint64 value) noexcept
{
    ier = value & IER::WRITE_MASK;
}

// ============================================================================
// Current Mode (CM) Helpers
// ============================================================================

/**
 * @brief Get Current Mode from PS register
 */
AXP_PURE AXP_FLATTEN
    inline CurrentMode getCM(quint64 ps) noexcept
{
    return static_cast<CurrentMode>(ps & IER::CM_MASK);
}

/**
 * @brief Set Current Mode in PS register
 */
AXP_FLATTEN
    inline void setCM(quint64& ps, CurrentMode mode) noexcept
{
    ps = (ps & ~IER::CM_MASK) | static_cast<quint64>(mode);
}

/**
 * @brief Check if in Kernel mode
 */
AXP_PURE AXP_FLATTEN
    inline bool isKernelMode(quint64 ps) noexcept
{
    return (ps & IER::CM_MASK) == static_cast<quint64>(CurrentMode::KERNEL);
}

/**
 * @brief Check if in User mode
 */
AXP_PURE AXP_FLATTEN
    inline bool isUserMode(quint64 ps) noexcept
{
    return (ps & IER::CM_MASK) == static_cast<quint64>(CurrentMode::USER);
}

/**
 * @brief Get mode name (for debugging)
 */
AXP_PURE AXP_FLATTEN
    inline const char* getModeName(CurrentMode mode) noexcept
{
    switch (mode) {
    case CurrentMode::KERNEL:     return "Kernel";
    case CurrentMode::EXECUTIVE:  return "Executive";
    case CurrentMode::SUPERVISOR: return "Supervisor";
    case CurrentMode::USER:       return "User";
    default:                      return "Unknown";
    }
}

// ============================================================================
// Interrupt Masking Logic
// ============================================================================

/**
 * @brief Check if interrupt is masked by IER
 * @param ier Current IER value
 * @param ipl Interrupt priority level
 * @param isExternal True if external interrupt
 * @param externalLine External interrupt line (0-5), ignored if !isExternal
 * @return true if interrupt is masked (should not be delivered)
 */
AXP_PURE AXP_FLATTEN
    inline bool isInterruptMasked(quint64 ier, quint8 ipl, bool isExternal = false,
                      quint8 externalLine = 0) noexcept
{
    if (isExternal) {
        // External interrupt - check EIEN bit
        Q_ASSERT(externalLine < 6);
        return !isExternalInterruptEnabled(ier, externalLine);
    } else if (ipl >= 1 && ipl <= 15) {
        // Software interrupt - check SIEN bit
        return !isSoftwareInterruptEnabled(ier, ipl);
    } else {
        // Hardware interrupt at IPL > 15 - not maskable by IER
        return false;
    }
}

/**
 * @brief Build interrupt enable mask for IRQ controller
 * @param ier Current IER value
 * @return 64-bit mask where bit N = interrupt at IPL N is enabled
 */
AXP_PURE AXP_FLATTEN
    inline quint64 buildInterruptEnableMask(quint64 ier) noexcept
{
    quint64 mask = 0;

    // Software interrupts (IPL 1-15)
    quint16 sien = getSIEN(ier);
    mask |= (static_cast<quint64>(sien) << 1);  // SIEN covers bits 15:1

    // External interrupts (use IPL 20-25 for external lines 0-5)
    quint8 eien = getEIEN(ier);
    for (int line = 0; line < 6; ++line) {
        if (eien & (1 << line)) {
            mask |= (1ULL << (20 + line));
        }
    }

    // Hardware interrupts (IPL 16-31) - always enabled
    mask |= 0xFFFFFFFF00000000ULL;  // IPL 32-63 (not used but set for safety)

    return mask;
}

/**
 * @brief Update IRQ controller's interrupt enable mask
 * @param cpuId CPU ID
 * @param ier Current IER value
 */
inline void updateInterruptEnableMask(CPUIdType cpuId, quint64 ier) noexcept;

// ============================================================================
// IER Display / Debug Helpers
// ============================================================================

/**
 * @brief Format IER for debugging
 */
inline QString formatIER(quint64 ier) noexcept
{
    return QString("IER[SLEN=%1 CREN=%2 PCEN=%3 ASTEN=%4 SIEN=0x%5 EIEN=0x%6]")
    .arg(getSLEN(ier) ? 1 : 0)
        .arg(getCREN(ier) ? 1 : 0)
        .arg(getPCEN(ier), 1, 2, QChar('0'))
        .arg(getASTEN(ier) ? 1 : 0)
        .arg(getSIEN(ier), 4, 16, QChar('0'))
        .arg(getEIEN(ier), 2, 16, QChar('0'));
}

/**
 * @brief Format CM for debugging
 */
inline QString formatCM(quint64 ps) noexcept
{
    CurrentMode mode = getCM(ps);
    return QString("CM=%1 (%2)")
        .arg(static_cast<int>(mode), 2, 2, QChar('0'))
        .arg(getModeName(mode));
}

#endif // MASKED_IER_CM_INL_H


/*
 * Usage:
 *
 * // Read IER
quint64 ier = getIER_Active(cpuId);

// Check if software interrupt at IPL 5 is enabled
if (isSoftwareInterruptEnabled(ier, 5)) {
    // Deliver interrupt
}

// Enable external interrupt line 2
enableExternalInterrupt(ier, 2);
setIER_Active(cpuId, ier);

// Update IRQ controller mask
updateInterruptEnableMask(cpuId, ier);
 *
 */
