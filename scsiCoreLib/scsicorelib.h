// ============================================================================
// scsicorelib.h - In SCSI controller
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

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
