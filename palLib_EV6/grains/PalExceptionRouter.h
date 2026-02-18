#ifndef PALEXCEPTIONROUTER_H
#define PALEXCEPTIONROUTER_H
#include "../../faultLib/PendingEvent_Refined.h"
#include "../../exceptionLib/Exception_Core_Refined.h"
#include "../PALVectorId_Refined.h"

// ============================================================================
// Exception Class to PAL Vector Mapping
// ============================================================================
// SINGLE SOURCE OF TRUTH for ExceptionClass -> PalVectorId resolution.
//
// This function implements the canonical Alpha AXP exception vector mapping
// from Table 5-8 of the Architecture Reference Manual.
//
// Pipeline code should NEVER compute palVectorId directly - always use this.
//

/**
 * @brief Map ExceptionClass to PalVectorId
 *
 * This is the single authoritative mapping for exception vector resolution.
 * All exceptions flow through this function during preparation for delivery.
 *
 * Special cases:
 * - CALL_PAL exceptions are marked with INVALID and require calculation
 * - Named CALL_PALs (BPT, BUGCHECK, GENTRAP) are handled here
 *
 * @param ev PendingEvent with exceptionClass set
 * @return PalVectorId for hardware vector lookup
 */
inline PalVectorId mapClassToPalVector(const PendingEvent& ev) noexcept
{
    using EC = ExceptionClass;

    switch (ev.exceptionClass)
    {
    // ========================================================================
    // Memory Management - ITB Domain
    // ========================================================================
    case EC::ITB_MISS:
        return PalVectorId::ITB_MISS;

    case EC::ITB_ACV:
        return PalVectorId::IACCVIO;

    // ========================================================================
    // Memory Management - DTB Domain
    // ========================================================================
    case EC::DTB_MISS_SINGLE:
        return PalVectorId::DTB_MISS_SINGLE;

    case EC::DTB_MISS_DOUBLE:
        return PalVectorId::DTB_MISS_DOUBLE;

    case EC::DTB_FAULT:
        // Both map to DFAULT vector (0x0380)
        // PAL handler distinguishes via MM_STAT
        return PalVectorId::DFAULT;

    // ========================================================================
    // Alignment & Opcode Faults
    // ========================================================================
    case EC::UNALIGN:
        return PalVectorId::UNALIGN;

    case EC::ILLEGAL_OPCODE:
        return PalVectorId::OPCDEC;

    case EC::FEN:
        return PalVectorId::FEN;

    // ========================================================================
    // Arithmetic Traps
    // ========================================================================
    case EC::ARITH:
        return PalVectorId::ARITH;

    // ========================================================================
    // System - CALL_PAL Instructions
    // ========================================================================
    case EC::CALL_PAL:
        // Generic CALL_PAL - PC must be calculated
        // preparePendingEventForDelivery() will call calculateCallPalEntryPC()
        // Return INVALID to signal calculation required
        return PalVectorId::INVALID;
    case EC::AST:
    {
        const quint8 cm = ev.faultCM;  // captured at fault creation
        switch (cm)
        {
        case CM_KERNEL:
        case CM_EXECUTIVE:
        case CM_SUPERVISOR:
            return PalVectorId::AST_SYS;

        case CM_USER:
            return PalVectorId::AST_USER;

        default:
            return PalVectorId::INVALID;
        }
    }
    case ExceptionClass::AST_USER:
        return PalVectorId::AST_USER;


    // ========================================================================
    // Asynchronous Events
    // ========================================================================
    case EC::INTERRUPT:
        return PalVectorId::INTERRUPT;

    case EC::MACHINE_CHECK:
        return PalVectorId::MCHK;

    case EC::RESET:
        return PalVectorId::RESET;

    // ========================================================================
    // Invalid/Unknown
    // ========================================================================
    case EC::None:
    case EC::INVALID:
    default:
        return PalVectorId::INVALID;
    }
}

/**
 * @brief Check if exception class requires CALL_PAL calculation
 * @param ec ExceptionClass to check
 * @return true if PC calculation required (not a static vector)
 */
inline bool requiresCallPalCalculation(ExceptionClass ec) noexcept
{
    return ec == ExceptionClass::CALL_PAL;
}

/**
 * @brief Check if exception is memory-related
 * @param ec ExceptionClass to check
 * @return true if ITB/DTB exception
 */
inline bool isMemoryException(ExceptionClass ec) noexcept
{
    using EC = ExceptionClass;
    return ec == EC::ITB_MISS || ec == EC::ITB_ACV ||
           ec == EC::DTB_MISS_SINGLE || ec == EC::DTB_MISS_DOUBLE ||
           ec == EC::DFAULT;
}

/**
 * @brief Check if exception is synchronous (fault/trap)
 * @param ec ExceptionClass to check
 * @return true if synchronous, false if asynchronous
 */
inline bool isSynchronousException(ExceptionClass ec) noexcept
{
    using EC = ExceptionClass;
    return ec != EC::INTERRUPT && ec != EC::MCHK && ec != EC::RESET;
}
#endif // PALEXCEPTIONROUTER_H
