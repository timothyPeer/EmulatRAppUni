#pragma once
#include "CPUStateIPRInterface.h"
#include <QtGlobal>
#include "VA_helpers.h"
#include "Global_IPRInterface.h"
#include "HWPCB_helpers_inline.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "Ev6TLBInterface.h"

inline void executeTBIS(CPUStateIPRInterface* cpuState)
{
	const quint8 cpuId = cpuState->cpuId();
	auto& iprs = globalIPRBank()[cpuId];

	auto& tlb = Ev6SiliconTLB_Singleton::interface();
	const quint64 va = iprs.va;
	const quint8  asn = getASN_Active(cpuId);

	// Invalidate both ITB and DTB entries corresponding to VA (TBIS)
	tlb.tbis(cpuId, Realm::I, 0, va, asn);
	tlb.tbis(cpuId, Realm::D, 0, va, asn);
}

