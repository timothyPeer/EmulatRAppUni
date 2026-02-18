#ifndef HANDLEACCESSVIOLATION_INL_H
#define HANDLEACCESSVIOLATION_INL_H
#include "MemoryFaultInfo.h"
#include "cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>

/// \brief Handle access violation
inline void handleAccessViolation(AlphaCPU* argCpu, const MemoryFaultInfo& faultInfo)
{
	if (!argCpu) return;

	qWarning() << "Access Violation:"
		<< "VA=" << Qt::hex << faultInfo.faultAddress
		<< "PC=" << Qt::hex << faultInfo.pc
		<< "mode=" << faultInfo.currentMode
		<< (faultInfo.isWrite ? "WRITE" : "READ");

	// ----------------------------------------------------------------
	// Access violations are serious - typically result in SEGV
	// PALcode will save context and call OS handler
	// ----------------------------------------------------------------

#ifdef BREAK_ON_ACCESS_VIOLATION
	argCpu->triggerDebugBreak("Access Violation");
#endif
}

#endif // HANDLEACCESSVIOLATION_INL_H
