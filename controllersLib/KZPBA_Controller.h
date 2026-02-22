// ============================================================================
// KZPBA_Controller.h - ============================================================================
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

// ============================================================================
// KZPBA_Controller.H
// ============================================================================
// DEC KZPBA PCI SCSI Controller (ISP1020-based)
// -----------------------------------------------------------------------------
// Purpose:
//   The DEC KZPBA is an OEM-branded QLogic ISP1020 SCSI controller.
//   This header provides a thin adapter layer over the existing
//   ISP1020_Controller, adding:
//
//     � DEC-specific PCI IDs (vendor 0x1011, device 0x000F)
//     � DEC naming (�KZPBA�)
//     � Same BAR configuration model
//     � Same SCSI host adapter behavior
//
//   All SCSI, DMA, mailbox, and script-engine behavior is inherited from
//   ISP1020_Controller.
//
// Dependencies:
//   - QtCore
//   - ISP1020_Controller.H
//   - No AlphaCPU / SafeMemory / MMIOManager
//   - Pure ASCII, header-only
// ============================================================================

#ifndef KZPBA_CONTROLLER_H
#define KZPBA_CONTROLLER_H

#include <QtGlobal>
#include <QString>

#include "ISP1020_Controller.H"

// ============================================================================
// KZPBA_Controller
// ============================================================================
class KZPBA_Controller : public ISP1020_Controller
{
public:
	explicit KZPBA_Controller(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn,
		bool           threadSafe = false) noexcept
		: ISP1020_Controller(bus, initiatorName, initiatorWwn, threadSafe)
	{
		setName("KZPBA_Controller");
		setMmioRegionName("KZPBA_MMIO");

		// DEC PCI IDs for KZPBA adapters:
		//
		//   Vendor : 0x1011  (DEC)
		//   Device : 0x000F  (KZPBA)
		//
		// NOTE: Some KZPBA variants used QLogic vendor ID. You may override
		//       these in EmulatorSettings if a specific machine requires it.
		//
		PciConfigSpace& cfg = config();
		cfg.vendorId = 0x1011;   // DEC
		cfg.deviceId = 0x000F;   // KZPBA device ID
	}

	virtual ~KZPBA_Controller() noexcept override = default;
};

#endif // KZPBA_CONTROLLER_H
