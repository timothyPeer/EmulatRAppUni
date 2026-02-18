#ifndef SCHEDULEARITHMETICTRAP_INL_H
#define SCHEDULEARITHMETICTRAP_INL_H

#include "../faultLib/fault_core.h"
#include <QtGlobal>
#include <QString>
#include "../coreLib/IPRStorage_HotExt.h"
#include "../coreLib/types_core.h"
#include "coreLib/LoggingMacros.h"
#include "palLib_EV6/Global_PALVectorTable.h"
#include "palLib_EV6/PalVectorTable_final.h"

// ============================================================================
// scheduleArithmeticTrap
// Schedules a synchronous ARITHMETIC trap via the FaultDispatcher.
// Used by FP grains (OVF/UNF/INE/etc.) and integer grains (IOV).
// ============================================================================

inline void scheduleArithmeticTrap_fp(CPUIdType cpuId, quint64 fpcr) noexcept
{
	
	auto& iprs = globalIPRHotExt(cpuId);

	// ------------------------------------------------------------------------
	// TRACE: report all FPCR exception bits + EXC_SUM + PC
	// ------------------------------------------------------------------------
	// TRACE_LOG(
	// 	QString("scheduleArithmeticTrap: cpu=%1 fpcr=0x%2 exc_sum=0x%3 pc=0x%4")
	// 	.arg(cpuId)
	// 	.arg(fpcr)
	// 	.arg(iprs.exc_sum)
	// 	.arg(getPC_Active(cpuId)
	// 	);


	// ------------------------------------------------------------------------
	// Map ARITHMETIC class -> PAL vector @ 0x0100
	// ------------------------------------------------------------------------
	auto& palTable = global_PalVectorTable();
	const PalVectorId palVec = palTable.mapException(ExceptionClass_EV6::Arithmetic);

	// ------------------------------------------------------------------------
	// Build PendingEvent
	// ------------------------------------------------------------------------
	PendingEvent ev{};
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Arithmetic;
	ev.palVectorId = palVec;

	// Arithmetic traps do not have a memory VA
	ev.faultVA = 0;
	ev.extraInfo = iprs.exc_sum;    // EXC_SUM is the standard PAL-visible summary

	// Property info -> synchronous, non-memory trap
	ev.pendingEvent_Info.isWrite = false;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isUnaligned = false;

	// Queue the trap in the per-CPU dispatcher
	globalFaultDispatcher(cpuId).setPendingEvent(ev);

	// ------------------------------------------------------------------------
	// EXC_ADDR = next PC, per Alpha SRM (II-A) 6-13..6-14
	// ------------------------------------------------------------------------
	iprs.exc_addr = getPC_Active(cpuId);
}

#endif // SCHEDULEARITHMETICTRAP_INL_H
