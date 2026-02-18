#pragma once
#include <QtGlobal>
#include "pendingEvent_struct.h"
#include "types_core.h"
#include "../exceptionLib/Exception_Core.h"
#include "PendingEvent.h"
#include "GlobalFaultDispatcherBank.h"

bool hasPendingTranslationFault(CPUIdType cpuId, VAType va) noexcept
{
	auto& fd = globalFaultDispatcher(cpuId);
	if (fd.eventPendingState().kind != PendingEventKind::TRAP_SYNC) {
		return false;
	}

	// Only treat Memory / AccessViolation as "translation faults".
	switch (fd.eventPendingState().exceptionClass) {
	case ExceptionClass::MEMORY_FAULT:
	case ExceptionClass::ACCESS_VIOLATION:
		break;
	default:
		return false;
	}

	// If caller cares about a specific VA, match it; if va == 0, treat as
	// "any translation fault pending".
	if (va == 0) {
		return true;
	}

	return (fd.eventPendingState().faultVA == va);
}