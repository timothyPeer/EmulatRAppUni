// ============================================================================
// ScsiControllerKzpba.h - HBA-specific: bind IRQ, DMA, SafeMemory, DeviceNode, etc.
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

#pragma once
#include <QtGlobal>
#include "../scsiCoreLib/IscsiDevice.h"

class ScsiControllerKzpba
{
public:

	bool attachSCSIDevice(quint8 target, quint8 lun, ISCSIDevice* dev)
	{
		if (!m_bus.attachDevice(target, lun, dev))
			return false;

		// HBA-specific: bind IRQ, DMA, SafeMemory, DeviceNode, etc.
		// Example:
		// dev->setController(this);
		// dev->setTargetId(target);
		// dev->setLun(lun);
		// dev->setDMACoherencyManager(m_dmaMgr);
		// dev->setSafeMemory(m_safeMem);
		// gIRQController()->registerIRQVector(...);
		// Set address fields for the device
		dev->setTarget(target);
		dev->setLun(lun);

		return true;
	}

	ISCSIDevice* resolve(quint8 target, quint8 lun) const
	{
		return m_bus.device(target, lun);
	}



private:
	SCSIBus m_bus;
};
