#ifndef HANDLEFAULTONEXECUTE_INL_H
#define HANDLEFAULTONEXECUTE_INL_H
#include "cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>

/// \brief Handle fault-on-execute
void handleFaultOnExecute(AlphaCPU* argCpu, const MemoryFaultInfo& faultInfo)
{
	if (!argCpu) return;

	DEBUG_LOG(QString("Fault-on-Execute: VA=0x%1")
		.arg(faultInfo.faultAddress, 16, 16, QChar('0')));

	// Used for NX (no-execute) page protection
}
#endif // HANDLEFAULTONEXECUTE_INL_H
