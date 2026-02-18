#pragma once
#include <QtGlobal>
#include "../pipelineLib/adjustPipeline_inl.h"
#include "../coreLib/types_core.h"
#include "../coreLib/LoggingMacros.h"
#include "../pteLib/alpha_pte_core.h"
#include "../EboxLib/prefetchVA_inl.h"

/// \brief Wrapper to prepare for virtual address translation in a microarchitectural simulator.
/// \param va The virtual address to prepare for translation.
/// \details
/// - Calls prewarmTLB and adjustPipeline in sequence. 
/// - No side effects in functional simulation.
/// \see Alpha AXP System Reference Manual, Microarchitecture Appendices
inline void prepareForVATranslation(CPUIdType cpuId,
	Realm   realm,
	SC_Type  sizeClass,
	VAType va)
{
	// 1) Warm the SPAM/TLB for this VA
	//prewarmTLB(cpuId, realm, sizeClass, va);

	prefetchVA(cpuId, va, realm == Realm::I ? true : false);
	// 2) Optionally do micro-architectural pipeline modeling
	//    For a functional emulator this can stay a no-op.
	adjustPipeline(va);

	DEBUG_LOG(QString("PREPARE_VA_XLATE VA: 0x%1").arg(16,16,va));

}