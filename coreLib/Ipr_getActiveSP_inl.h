#pragma once
#include <QtGlobal>
#include "CPUStateIPRInterface.h"
#include "HWPCB_helpers_inline.h"

inline quint64 getActiveSP(CPUStateIPRInterface* cpuState) noexcept
{
	quint8 cpuId = cpuState->cpuId();

	quint8 cm = getCM_Active(cpuId);
	switch (cm) {
	case 0: return getKSP_Active(cpuId);  // Kernel
	case 1: return getESP_Active(cpuId);  // Executive
	case 2: return getSSP_Active(cpuId);  // Supervisor
	case 3: return getUSP_Active(cpuId);  // User
	default: return getKSP_Active(cpuId);
	}
}