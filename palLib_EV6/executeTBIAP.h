#pragma once
#include "CPUStateIPRInterface.h"
#include <QtGlobal>
#include "VA_helpers.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "Global_IPRInterface.h"
#include "Ev6TLBInterface.h"

inline void executeTBIAP(CPUStateIPRInterface* cpuState)
{
	Q_UNUSED(cpuState); // or keep cpuId if you want, but it's unused
	auto& tlb = Ev6SiliconTLB_Singleton::interface();
	tlb.tbiAll();
}

