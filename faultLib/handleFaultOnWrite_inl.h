#ifndef HANDLEFAULTONWRITE_INL_H
#define HANDLEFAULTONWRITE_INL_H
#include "MemoryFaultInfo.h"
#include "cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>

/// \brief Handle fault-on-write (copy-on-write, dirty tracking)
void handleFaultOnWrite(AlphaCPU* argCpu, const MemoryFaultInfo& faultInfo)
{
	if (!argCpu) return;

	DEBUG_LOG(QString("Fault-on-Write: VA=0x%1")
		.arg(faultInfo.faultAddress, 16, 16, QChar('0')));

	// ----------------------------------------------------------------
	// FOW is used by OS for:
	// - Copy-on-write pages
	// - Dirty bit tracking
	// - Memory-mapped file synchronization
	// ----------------------------------------------------------------
}
#endif // HANDLEFAULTONWRITE_INL_H
