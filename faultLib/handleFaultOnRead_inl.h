#ifndef HANDLEFAULTONREAD_INL_H
#define HANDLEFAULTONREAD_INL_H
#include "MemoryFaultInfo.h"
#include "cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>

/// \brief Handle fault-on-read (demand paging)
void handleFaultOnRead(AlphaCPU* argCpu, const MemoryFaultInfo& faultInfo)
{
	if (!argCpu) return;

	DEBUG_LOG(QString("Fault-on-Read: VA=0x%1")
		.arg(faultInfo.faultAddress, 16, 16, QChar('0')));

	// Used for demand paging, lazy allocation
}
#endif // HANDLEFAULTONREAD_INL_H
