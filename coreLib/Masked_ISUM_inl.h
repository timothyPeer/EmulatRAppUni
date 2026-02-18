#ifndef MASKED_ISUM_INL_H
#define MASKED_ISUM_INL_H

// ============================================================================
// Masked_ISUM_inl.h
// ============================================================================
// Interrupt Summary Register (ISUM) inline helpers
//
// ISUM Register Layout (EV6):
// Bits [38:33] - EI     External Interrupt Summary (6 external interrupt lines)
// Bit  [32]    - SL     System Level Interrupt Summary
// Bit  [31]    - CR     Corrected Read Error Summary
// Bits [30:29] - PC     Performance Counter Summary (2 counters: PC0, PC1)
// Bits [28:14] - SI     Software Interrupt Summary (IPL 14-28, maps to IPL 1-15)
// Bit  [10]    - ASTU   AST User mode
// Bit  [9]     - ASTS   AST Supervisor mode
// Bit  [4]     - ASTE   AST Executive mode
// Bit  [3]     - ASTK   AST Kernel mode
//
// ISUM is READ-ONLY and reflects current pending interrupts.
// Each bit indicates a pending interrupt at the corresponding level.
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "Axp_Attributes_core.h"
#include <QString>
#include "coreLib_core.h"
#include "Ast_core_inl.h"


// ============================================================================
// ISUM Getters (Read-Only Register)
// ============================================================================

/**
 * @brief Get External Interrupt Summary (bits 38:33)
 * @return 6-bit mask indicating which external interrupts are pending
 */
AXP_PURE AXP_FLATTEN
     quint8 getEI(quint64 isum) noexcept
{
    return static_cast<quint8>((isum & ISUM::EI_MASK) >> ISUM::EI_SHIFT);
}

/**
 * @brief Check if external interrupt line is pending
 */
AXP_PURE AXP_FLATTEN
     bool isExternalInterruptPending(quint64 isum, quint8 line) noexcept
{
    Q_ASSERT(line < 6);
    return (isum & (1ULL << (ISUM::EI_SHIFT + line))) != 0;
}

/**
 * @brief Get System Level Interrupt status (bit 32)
 */
AXP_PURE AXP_FLATTEN
     bool getSL(quint64 isum) noexcept
{
    return (isum & ISUM::SL_MASK) != 0;
}

/**
 * @brief Get Corrected Read Error status (bit 31)
 */
AXP_PURE AXP_FLATTEN
     bool getCorrectedReadErrorStatusBit(quint64 isum) noexcept
{
    return (isum & ISUM::CR_MASK) != 0;
}

/**
 * @brief Get Performance Counter Summary (bits 30:29)
 * @return 2-bit mask: bit 0 = PC0, bit 1 = PC1
 */
AXP_PURE AXP_FLATTEN
     quint8 getPerformanceCounter(quint64 isum) noexcept
{
    return static_cast<quint8>((isum & ISUM::PC_MASK) >> ISUM::PC_SHIFT);
}

/**
 * @brief Check if Performance Counter 0 interrupt is pending
 */
AXP_PURE AXP_FLATTEN
     bool isPC0Pending(quint64 isum) noexcept
{
    return (isum & ISUM::PC0_MASK) != 0;
}

/**
 * @brief Check if Performance Counter 1 interrupt is pending
 */
AXP_PURE AXP_FLATTEN
     bool isPC1Pending(quint64 isum) noexcept
{
    return (isum & ISUM::PC1_MASK) != 0;
}

/**
 * @brief Get Software Interrupt Summary (bits 28:14)
 * Maps to IPL 1-15 (bit 14 = IPL 1, bit 28 = IPL 15)
 * @return 15-bit mask indicating pending software interrupts
 */
AXP_PURE AXP_FLATTEN
     quint16 getSI(quint64 isum) noexcept
{
    return static_cast<quint16>((isum & ISUM::SI_MASK) >> ISUM::SI_SHIFT);
}

/**
 * @brief Check if software interrupt at IPL is pending
 * @param ipl Interrupt priority level (1-15)
 */
AXP_PURE AXP_FLATTEN
     bool isSoftwareInterruptPending(quint64 isum, quint8 ipl) noexcept
{
    Q_ASSERT(ipl >= 1 && ipl <= 15);
    quint64 bitPos = ISUM::SI_SHIFT + (ipl - 1);
    return (isum & (1ULL << bitPos)) != 0;
}


/**
 * @brief Get all AST bits as 4-bit mask
 * Bit order: [ASTU, ASTS, ASTE, ASTK]
 */



// ============================================================================
// ISUM Setters (for building ISUM value)
// ============================================================================

/**
 * @brief Set External Interrupt Summary bits
 */
AXP_FLATTEN
     void setEI(quint64& isum, quint8 value) noexcept
{
    isum = (isum & ~ISUM::EI_MASK) |
           ((static_cast<quint64>(value & 0x3F) << ISUM::EI_SHIFT) & ISUM::EI_MASK);
}

/**
 * @brief Set external interrupt line pending
 */
AXP_FLATTEN
     void setExternalInterruptPending(quint64& isum, quint8 line, bool pending) noexcept
{
    Q_ASSERT(line < 6);
    quint64 mask = 1ULL << (ISUM::EI_SHIFT + line);
    if (pending) {
        isum |= mask;
    } else {
        isum &= ~mask;
    }
}

/**
 * @brief Set System Level Interrupt
 */
AXP_FLATTEN
     void setSL(quint64& isum, bool pending) noexcept
{
    if (pending) {
        isum |= ISUM::SL_MASK;
    } else {
        isum &= ~ISUM::SL_MASK;
    }
}

/**
 * @brief Set Corrected Read Error
 */
AXP_FLATTEN
     void setCR(quint64& isum, bool pending) noexcept
{
    if (pending) {
        isum |= ISUM::CR_MASK;
    } else {
        isum &= ~ISUM::CR_MASK;
    }
}

/**
 * @brief Set Performance Counter Summary
 */
AXP_FLATTEN
     void setPC(quint64& isum, quint8 value) noexcept
{
    isum = (isum & ~ISUM::PC_MASK) |
           ((static_cast<quint64>(value & 0x3) << ISUM::PC_SHIFT) & ISUM::PC_MASK);
}

/**
 * @brief Set Performance Counter 0 pending
 */
AXP_FLATTEN
     void setPC0(quint64& isum, bool pending) noexcept
{
    if (pending) {
        isum |= ISUM::PC0_MASK;
    } else {
        isum &= ~ISUM::PC0_MASK;
    }
}

/**
 * @brief Set Performance Counter 1 pending
 */
AXP_FLATTEN
     void setPC1(quint64& isum, bool pending) noexcept
{
    if (pending) {
        isum |= ISUM::PC1_MASK;
    } else {
        isum &= ~ISUM::PC1_MASK;
    }
}

/**
 * @brief Set Software Interrupt Summary from SIRR/SISR
 * @param si Software interrupt bitmap (bits correspond to IPL 1-15)
 */
AXP_FLATTEN
     void setSI(quint64& isum, quint16 si) noexcept
{
    isum = (isum & ~ISUM::SI_MASK) |
           ((static_cast<quint64>(si & 0x7FFF) << ISUM::SI_SHIFT) & ISUM::SI_MASK);
}

/**
 * @brief Set software interrupt pending for specific IPL
 */
AXP_FLATTEN
     void setSoftwareInterruptPending(quint64& isum, quint8 ipl, bool pending) noexcept
{
    Q_ASSERT(ipl >= 1 && ipl <= 15);
    quint64 bitPos = ISUM::SI_SHIFT + (ipl - 1);
    quint64 mask = 1ULL << bitPos;
    if (pending) {
        isum |= mask;
    } else {
        isum &= ~mask;
    }
}




// ============================================================================
// ISUM Analysis Helpers
// ============================================================================

/**
 * @brief Check if any interrupt is pending
 */
AXP_PURE AXP_FLATTEN
     bool isAnyInterruptPending(quint64 isum) noexcept
{
    return (isum & ISUM::VALID_MASK) != 0;
}

/**
 * @brief Count total pending interrupts
 */
AXP_PURE AXP_FLATTEN
     quint8 countPendingInterrupts(quint64 isum) noexcept
{
    quint8 count = 0;

    // Count external interrupts
    quint8 ei = getEI(isum);
    for (int i = 0; i < 6; ++i) {
        if (ei & (1 << i)) ++count;
    }

    // Count software interrupts
    quint16 si = getSI(isum);
    for (int i = 0; i < 15; ++i) {
        if (si & (1 << i)) ++count;
    }

    // Count other interrupts
    if (getSL(isum)) ++count;
    if (getCorrectedReadErrorStatusBit(isum)) ++count;
    if (isPC0Pending(isum)) ++count;
    if (isPC1Pending(isum)) ++count;
    if (isAnyASTPending(isum)) ++count;

    return count;
}

// ============================================================================
// ISUM Display / Debug Helpers
// ============================================================================

/**
 * @brief Format ISUM for debugging
 */
AXP_PURE  AXP_FLATTEN  QString formatISUM(quint64 isum) noexcept
{
    QStringList parts;

    if (quint8 ei = getEI(isum)) {
        parts << QString("EI=0x%1").arg(ei, 2, 16, QChar('0'));
    }
    if (getSL(isum)) parts << "SL";
    if (getCorrectedReadErrorStatusBit(isum)) parts << "CR";
    if (quint8 pc = (isum)) {
        parts << QString("PC=%1").arg(pc, 2, 2, QChar('0'));
    }
    if (quint16 si = getSI(isum)) {
        parts << QString("SI=0x%1").arg(si, 4, 16, QChar('0'));
    }
    if (getASTU(isum)) parts << "ASTU";
    if (getASTS(isum)) parts << "ASTS";
    if (getASTE(isum)) parts << "ASTE";
    if (getASTK(isum)) parts << "ASTK";

    return QString("ISUM[%1]").arg(parts.isEmpty() ? "none" : parts.join(" "));
}

/**
 * @brief Format ISUM with detailed breakdown
 */
AXP_PURE  AXP_FLATTEN  QString formatISUMDetailed(quint64 isum) noexcept
{
    QString result = QString("ISUM=0x%1\n").arg(isum, 16, 16, QChar('0'));

    result += QString("  EI[38:33] = 0x%1 (").arg(getEI(isum), 2, 16, QChar('0'));
    for (int i = 0; i < 6; ++i) {
        if (isExternalInterruptPending(isum, i)) {
            result += QString(" %1").arg(i);
        }
    }
    result += " )\n";

    result += QString("  SL[32]    = %1\n").arg(getSL(isum) ? 1 : 0);
    result += QString("  CR[31]    = %1\n").arg(getCorrectedReadErrorStatusBit(isum) ? 1 : 0);
    result += QString("  PC[30:29] = %1 (PC0=%2 PC1=%3)\n")
                  .arg(getPerformanceCounter(isum), 2, 2, QChar('0'))
                  .arg(isPC0Pending(isum) ? 1 : 0)
                  .arg(isPC1Pending(isum) ? 1 : 0);

    result += QString("  SI[28:14] = 0x%1 (IPL").arg(getSI(isum), 4, 16, QChar('0'));
    for (int ipl = 1; ipl <= 15; ++ipl) {
        if (isSoftwareInterruptPending(isum, ipl)) {
            result += QString(" %1").arg(ipl);
        }
    }
    result += " )\n";

    result += QString("  AST       = K=%1 E=%2 S=%3 U=%4\n")
                  .arg(getASTK(isum) ? 1 : 0)
                  .arg(getASTE(isum) ? 1 : 0)
                  .arg(getASTS(isum) ? 1 : 0)
                  .arg(getASTU(isum) ? 1 : 0);

    return result;
}

/**
 * @brief Build ISUM from IRQ controller state
 */
AXP_PURE  AXP_FLATTEN  quint64 buildISUMFromIRQState(quint8 externalMask,
                                     quint16 softwareMask,
                                     bool systemLevel,
                                     bool correctedRead,
                                     quint8 perfCounter,
                                     quint8 astMask) noexcept
{
    quint64 isum = 0;

    setEI(isum, externalMask);
    setSL(isum, systemLevel);
    setCR(isum, correctedRead);
    setPC(isum, perfCounter);
    setSI(isum, softwareMask);

    if (astMask & 0x1) setASTK(isum, true);
    if (astMask & 0x2) setASTE(isum, true);
    if (astMask & 0x4) setASTS(isum, true);
    if (astMask & 0x8) setASTU(isum, true);

    return isum;
}

#endif // MASKED_ISUM_INL_H

/*
 * Usage:
 *
 * // Build ISUM from interrupt state
quint64 isum = 0;
setEI(isum, 0x04);           // External interrupt line 2 pending
setSI(isum, 0x0020);         // Software interrupt at IPL 6 pending
setASTU(isum, true);         // AST pending for user mode

// Check if any interrupt is pending
if (isAnyInterruptPending(isum)) {
    // Process interrupts
}

// Check if AST is pending for current mode
quint8 currentMode = getCM(ps);
if (isASTPendingForMode(isum, currentMode)) {
    deliverAST(cpuId);
}

// Debug output
DEBUG_LOG(formatISUM(isum));
// Output: "ISUM[EI=0x04 SI=0x0020 ASTU]"
 *
 *
 */
