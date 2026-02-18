#ifndef ONTRIGGERPRIVILEGEVIOLATION_INL_H
#define ONTRIGGERPRIVILEGEVIOLATION_INL_H


#include "AlphaCPU.h"
#include <QtGlobal>

/// \brief Trigger privilege violation for invalid PAL_BASE write
inline void onTriggerPrivilegeViolation(AlphaCPU* argCpu, quint64 invalidAddress)
{
	if (!argCpu) return;

	CPUIdType cpuId = argCpu->cpuId();
	qCritical() << "Privilege violation: Invalid PAL_BASE write:"
		<< Qt::hex << invalidAddress;
	auto iprs = globalIPRBank()[cpuId];

	// ----------------------------------------------------------------
	// 1. Build fault information
	// ----------------------------------------------------------------
	MemoryFaultInfo faultInfo;
	faultInfo.faultType = MemoryFaultType::PRIVILEGE_VIOLATION;
	faultInfo.faultingVA = 0;  // Not a VA fault
	faultInfo.faultAddress = invalidAddress;
	faultInfo.faultingPC = getPC_Active(cpuId); 
	faultInfo.inPALmode = iprs.isInPalCode();
	faultInfo.currentMode = getCM_Active(cpuId);

	// ----------------------------------------------------------------
	// 2. Record in exception registers
	// ----------------------------------------------------------------
	iprs.exc_addr = getPC_Active(cpuId);

	// ----------------------------------------------------------------
	// 3. Trap to PALcode or abort
	// ----------------------------------------------------------------
#ifdef TRAP_ON_INVALID_PAL_BASE
// Trap to PAL privilege violation handler
	argCpu->trapToPAL(PALEntry::PRIVILEGE_VIOLATION, invalidAddress);
#else
// In emulator, this is often fatal
	argCpu->triggerDebugBreak("Invalid PAL_BASE");
	argCpu->halt();
#endif
}

#endif // ONTRIGGERPRIVILEGEVIOLATION_INL_H
