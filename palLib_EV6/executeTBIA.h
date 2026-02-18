#pragma once
#include "CPUStateIPRInterface.h"
#include <QtGlobal>
#include "VA_helpers.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "Global_IPRInterface.h"
#include "Ev6TLBInterface.h"

inline void executeTBIA(CPUStateIPRInterface* cpuState)
{
	const quint8 cpuId = cpuState->cpuId();
	auto& tlb = Ev6SiliconTLB_Singleton::interface();
	tlb.tbia(cpuId);
}

