#pragma once
#include <QtGlobal>
#include "../coreLib/types_core.h"
#include "../PteLib/alpha_pte_core.h"
#include "../pteLib/Ev6SiliconTLB_Singleton.h"


// AlphaCPU::prefetchVA helper (software-visible hint only)
inline void prefetchVA(CPUIdType cpuId, VAType va, bool isInstrStream)
{
	//auto& silicon = Ev6SiliconTLB_Singleton().instance();
	auto& silicon = Ev6SiliconTLB_Singleton::silicon();
	auto& mgr = silicon.spam();
	const Realm r = isInstrStream ? Realm::I : Realm::D;
	const quint8 sc = 0; // or map from granularity hint bits

	mgr.prepareForVATranslation(cpuId, r, sc, va);
}
