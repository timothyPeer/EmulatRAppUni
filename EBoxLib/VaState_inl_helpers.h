#pragma once
#include <QtGlobal>
#include "EboxVAState.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"

// Standard way to prepare VA state before any translation
inline void loadVAState(EBoxVAState& s, AlphaProcessorContext& ipr, VAType va)
{
	s.setVA(va);
	s.setVA_CTL(ipr.getVA_CTL());
	s.setVPTB(ipr.getVPTB());
}