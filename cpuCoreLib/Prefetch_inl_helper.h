#pragma once
#include <QtGlobal>
#include "AlphaCPU.h"

inline void setSpeculativeLoads(AlphaCPU& cpuState, bool bSpecLoads) {

	throw std::logic_error("The method or operation is not implemented.");
}

inline void flushIPrefetchQueue_inl(AlphaCPU& cpuState) noexcept {

}


void flushIPrefetchQueue(CPUIdType cpuId)
{
	//TODO
}


inline void flushPrefetchLine(AlphaCPU& cpuState, quint64 va) noexcept {

}

void flushIPrefetchLine(CPUIdType cpuId, const VAType va)
{
	//TODO
}

inline void flushICache() {

}

inline void flushICacheByASN(ASNType asn) {

}
inline void flushBranchPredictorByASN(ASNType asn) {

}