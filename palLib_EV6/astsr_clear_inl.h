#pragma once

#include <QtGlobal>
#include "Global_IPRInterface.h"


inline void clearASTSR(CPUIdType cpuId AlphaInstructionGrain* grain, quint8 deliveredLevel) noexcept {
	auto& iprs = globalIPRBank()[cpuId];
	iprs.astsr &= ~(1u << deliveredLevel);
}
