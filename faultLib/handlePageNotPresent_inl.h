#ifndef HANDLEPAGENOTPRESENT_INL_H
#define HANDLEPAGENOTPRESENT_INL_H

#include "MemoryFaultInfo.h"
#include "cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>


/// \brief Handle page not present
void handlePageNotPresent(AlphaCPU* argCpu, const MemoryFaultInfo& faultInfo)
{
	if (!argCpu) return;

	DEBUG_LOG(QString("Page Not Present: VA=0x%1")
		.arg(faultInfo.faultAddress, 16, 16, QChar('0')));

	// Page is swapped out or not yet allocated
}

#endif // HANDLEPAGENOTPRESENT_INL_H
