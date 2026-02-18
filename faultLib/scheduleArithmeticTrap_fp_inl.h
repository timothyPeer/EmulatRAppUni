// ============================================================================
// scheduleArithmeticTrap_fp_inl.h
//
// Schedules a synchronous floating-point ARITHMETIC trap based on the
// local FPCR exception bits (INV, DZE, OVF, UNF, INE, IOV).
//
// This is the FP counterpart to scheduleArithmeticTrap_int_inl.h.
//
// Ref: Alpha AXP Architecture Reference Manual, Version 6, 1994
//      Vol. II-A, Exception Model, Arithmetic Trap, and EXC_SUM.
// ============================================================================

#ifndef SCHEDULEARITHMETICTRAP_FP_INL_H
#define SCHEDULEARITHMETICTRAP_FP_INL_H

#include <format>

#include "FaultDispatcher.h"
#include "../coreLib/types_core.h"

#include "../coreLib/globalIPR_hot_cold_new.h"


#include "../faultLib/fault_core.h"
#include "GlobalFaultDispatcherBank.h"
#include "PendingEvent_Refined.h"
#include "../coreLib/alpha_fpcr_core.h"
#include "coreLib/HWPCB_inline.h"
#include "coreLib/LoggingMacros.h"
#include "palLib_EV6/Global_PALVectorTable.h"
#include "palLib_EV6/PalVectorTable_final.h"

// ============================================================================
// scheduleArithmeticTrap_fp
// ============================================================================

/*
inline void scheduleArithmeticTrap_fp(CPUIdType cpuId, quint64 fpcrLocal) noexcept
{

	auto& iprs = globalIPRHotExt(cpuId);

	// Copy FPCR exception bits into EXC_SUM (PAL-visible summary).
	// You may already be updating EXC_SUM in the FP helpers; in that case
	// this can be reduced to OR-ing in the new bits.
	iprs.exc_sum |= (fpcrLocal & AlphaFPCR::EXC_MASK);

	// TRACE: log trap scheduling for diagnostics
	TRACE_LOG(std::format(
		"scheduleArithmeticTrap_fp: cpu={} fpcrLocal=0x{:016X} exc_sum=0x{:016X} pc=0x{:016X}",
		cpuId,
		fpcrLocal,
		iprs.exc_sum,
		getPC_Active(cpuId)
	));

	// Map ARITHMETIC exception -> PAL vector (ARITH)
	auto& palTable = global_PalVectorTable();
	const PalVectorId palVec = palTable.mapException(ExceptionClass_EV6::Arithmetic);

	PendingEvent ev{};
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Arithmetic;
	ev.palVectorId = palVec;

	ev.faultVA = 0;             // Not a memory fault
	ev.extraInfo = iprs.exc_sum;  // EXC_SUM passed to PAL in argument registers

	ev.pendingEvent_Info.isWrite = false;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isUnaligned = false;

	// Queue trap in dispatcher; CPU run loop will deliver it.
	globalFaultDispatcher(cpuId).setPendingEvent(ev);

	// EXC_ADDR = next PC, per Alpha SRM (II-A) 6-13..6-14.
	iprs.exc_addr = getPC_Active(cpuId);
}
*/

#endif // SCHEDULEARITHMETICTRAP_FP_INL_H
