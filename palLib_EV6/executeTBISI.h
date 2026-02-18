#pragma once
#include <QTGlobal>
#include "CPUStateIPRInterface.h"
#include "Global_IPRInterface.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "VA_helpers.h"
#include "Ev6TLBInterface.h"

inline void executeTBISI(CPUStateIPRInterface* cpuState)
{
	const quint8 cpuId = cpuState->cpuId();
	auto& iprs = globalIPRBank()[cpuId];

	auto& tlb = Ev6SiliconTLB_Singleton::interface();
	const quint64 va = iprs.va;
	const quint8  asn = getASN_Active(cpuId); // consistent with TBIS/TBISD

	tlb.tbis(cpuId, Realm::I, 0, va, asn);
}


