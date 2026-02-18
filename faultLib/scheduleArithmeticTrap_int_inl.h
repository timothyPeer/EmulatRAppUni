#ifndef SCHEDULEARITHMETICTRAP_INT_INL_H
#define SCHEDULEARITHMETICTRAP_INT_INL_H


#include "../faultLib/fault_core.h"

#include "../coreLib/types_core.h"







#include "../coreLib/alpha_fpcr_core.h"
#include "../coreLib/HWPCB_helpers_inline.h"
#include "../palLib_EV6/global_PalVectorTable.h"
#include "../faultLib/GlobalFaultDispatcherBank.h"

// ============================================================================
// scheduleArithmeticTrap_int
// Schedules an INTEGER arithmetic overflow trap.
// This is the integer equivalent of scheduleArithmeticTrap(), but specialized
// for the integer-overflow bit (IOV).
//
// Alpha AXP Architecture:
// - Integer overflow is part of the ARITH class.
// - PAL vector: 0x0100 (ARITH).
// - EXC_SUM bit: IOV (bit 54).
// - EXC_ADDR = next PC.
// - Trap is scheduled, not immediately taken.
// ============================================================================

inline void scheduleArithmeticTrap_int(PipelineSlot& slot) noexcept
{
	const CPUIdType cpuId = slot.cpuId;
	auto& iprs = globalIPRHotExt(cpuId);

	// Set EXC_SUM: integer overflow (IOV).
	iprs.exc_sum |= AlphaFPCR::IOV;

	// TRACE: Report integer arithmetic trap scheduling
	// TRACE_LOG(std::format(
	// 	"scheduleArithmeticTrap_int: cpu={} exc_sum=0x{:016X} pc=0x{:016X}",
	// 	cpuId,
	// 	iprs.exc_sum,
	// 	getPC_Active(cpuId)
	// ));

	// Map ARITHMETIC exception -> PAL vector
	auto& palTable = global_PalVectorTable();
	const PalVectorId palVec =
		palTable.mapException(ExceptionClass_EV6::Arithmetic);

	// Build PendingEvent (synchronous arithmetic trap)
	PendingEvent ev{};
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Arithmetic;
	ev.palVectorId = palVec;
	ev.faultVA = 0;
	ev.extraInfo = iprs.exc_sum;

	// Non-memory trap metadata
	ev.pendingEvent_Info.isWrite = false;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isUnaligned = false;

	// Queue trap in dispatcher
	globalFaultDispatcher(cpuId).setPendingEvent(ev);

	// Per Alpha SRM: EXC_ADDR = next PC
	iprs.exc_addr = getPC_Active(cpuId);
}

#endif // SCHEDULEARITHMETICTRAP_INT_INL_H
