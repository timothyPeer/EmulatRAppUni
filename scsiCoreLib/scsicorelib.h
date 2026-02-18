#ifndef SCSICORELIB_H
#define SCSICORELIB_H

#include "scsiCoreLib_global.h"

class SCSICORELIB_EXPORT ScsiCoreLib
{
public:
    ScsiCoreLib();
};


/*
// In SCSI controller
void ScsiController::completeOperation() {
	// ... operation done ...

	// Assert interrupt
	global_IRQController().assertInterrupt(
		m_interruptVector,  // 0x800
		m_interruptIPL,     // 20
		m_targetCpu         // 0
	);
}

*/
#endif // SCSICORELIB_H
