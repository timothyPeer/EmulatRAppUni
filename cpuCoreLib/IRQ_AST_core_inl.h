#pragma once
#include <QtGlobal>
#include "AlphaCPU.h"
#include "Global_IRQController.h"
#include "IRQController.h"
#include "CPUStateIPRInterface.h"



// TODO we should integrate with the FaultDispatcher
inline void onAstModeChanged(AlphaCPU* cpuState) {
	// If AST delivery rules depend on privilege mode, evaluate here.
	cpuState->setInterruptEligibilityDirty(true);
}



// TODO integrate with FaultDispatcher -- ASTs are handled there
inline void onAstTakenForMode(CPUIdType cpuId, quint8 cm)
{

	auto& iprs = globalIPRBank()[cpuId];

	quint8 astrr = iprs.astrr;
	astrr &= static_cast<quint8>(~(1u << cm));
	iprs.astrr = astrr;

	if (astrr == 0) {
		/*setPendingAST(cpuId,false);  LAZY update used on HWPCB::ASTER*/ 
	}
}
//TODO we need to check if this is correct
inline bool checkPendingAST(CPUIdType cpuId, bool bIsInPalMode = true)  noexcept
{
	// No ASTs in PAL mode
	if (bIsInPalMode)
		return false;

	auto& iprs = globalIPRBank()[cpuId];
	// Apply ASTEN mask (enable bits)
	quint32 enabled = static_cast<quint32>(iprs.asten); // AST Enabled
	if (!enabled)
		return false;

	
	// AST levels masked by IPL:
	// AST level N is deliverable only if N > IPL
	quint8 currIPL = iprs.ipl;
	quint32 maskIPL = ~((1u << (currIPL + 1)) - 1);

	quint32 eligible = enabled & maskIPL;
	return eligible != 0;
}

inline void evaluatePendingASTs(CPUStateIPRInterface* cpuState) {
}




