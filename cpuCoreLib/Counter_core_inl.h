#pragma once
#include <QtGlobal>
#include "CPUStateIPRInterface.h"

inline void setProcessorPerformanceCounterEnable(CPUStateIPRInterface* cpuState, quint8 value) noexcept{

	quint8 cpuId = cpuState->cpuId();
	auto& iprs = globalIPRBank()[cpuId];
	//iprs.ppce = value; // we need to set the enable switch?
}