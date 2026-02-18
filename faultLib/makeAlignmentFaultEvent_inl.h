#ifndef MAKEALIGNMENTFAULTEVENT_INL_H
#define MAKEALIGNMENTFAULTEVENT_INL_H

#include "PendingEvent_refined.h"
#include "../coreLib/HWPCB_helpers_inline.h"
#include "../coreLib/memory_enums_structs.h"

/**
 * @brief Create a PendingEvent for an architectural alignment fault
 *
 * Used by naturally aligned memory instructions (LDT, LDQ, STQ, etc).
 * Not used by *_U variants (LDQ_U, STQ_U), which do not alignment-trap.
 *
 * ASA behavior:
 *  - Alignment fault occurs before translation completes
 *  - Fault VA is reported
 *  - PAL vector resolved later by FaultDispatcher
 */
AXP_ALWAYS_INLINE 
PendingEvent makeAlignmentFaultEvent(
    CPUIdType cpuId,
    quint64   faultVA,
    bool      isWrite
) noexcept
{
    PendingEvent ev{};

    // ---------------------------------------------------------------------
    // Event classification
    // ---------------------------------------------------------------------
    ev.kind = PendingEventKind::Exception;
    ev.exceptionClass = ExceptionClass_EV6::Unalign;
    ev.cm = getCM_Active(cpuId);
    // ---------------------------------------------------------------------
    // Address context
    // ---------------------------------------------------------------------
    ev.faultVA = faultVA;
    ev.asn = getASN_Active(cpuId);
    ev.faultPC = getPC_Active(cpuId);
    // ---------------------------------------------------------------------
    // Memory fault properties
    // ---------------------------------------------------------------------
    ev.pendingEvent_Info.faultType = MemoryFaultType::ALIGNMENT_FAULT;
    ev.pendingEvent_Info.isWrite = isWrite;
    ev.pendingEvent_Info.isExecute = false;
    ev.pendingEvent_Info.isInstruction = false;   // DTB-side fault
    ev.pendingEvent_Info.isUnaligned = true;
    ev.pendingEvent_Info.accessType =
        isWrite ? MemoryAccessType::WRITE
        : MemoryAccessType::READ;

    // ---------------------------------------------------------------------
    // PAL vector left INVALID - resolved later by FaultDispatcher
    // ---------------------------------------------------------------------
    ev.palVectorId = PalVectorId::INVALID;

    return ev;
}

#endif // MAKEALIGNMENTFAULTEVENT_INL_H
