// ============================================================================
// mmio_deviceTemplate.h - ============================================================================
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

#ifndef _EMULATRAPPUNI_MMIOLIB_MMIO_DEVICETEMPLATE_H
#define _EMULATRAPPUNI_MMIOLIB_MMIO_DEVICETEMPLATE_H

#pragma once

#include <QString>
#include <QVector>
#include "CoreLib/mmio_core.h"
#include "CoreLib/DMA_core.h"
#include "CoreLib/mmio_structs.h"
#include "coreLib/IRQ_SourceId_core.h"
#include "coreLib/InterruptRouter.h"

struct IrqTemplate
{
	quint8          ipl{ IrqIPL::DEVICE_20 };
	ScbVectorIndex  vector{ 0 };
	IrqTriggerMode  trigger{ IrqTriggerMode::Level };
	IrqRoutingPolicy routing{ IrqRoutingPolicy::FixedCPU };
	qint32          affinityCpu{ 0 };
};

// ============================================================================
// DEVICE TEMPLATE (COMPLETE DEVICE SPECIFICATION)
// ============================================================================

struct DeviceTemplate {
	// ========================================================================
	// IDENTITY & METADATA
	// ========================================================================

	QString displayName;          // 
	QString vendorName;           // Vendor name (optional, for documentation)
	QString description;          // Longer description (optional)
	mmio_DeviceClass deviceClass;      // Device class (SCSI_CONTROLLER, NIC, etc.)
	// PCI identity (if PCI device)
	quint16 vendorId;
	quint32 deviceId;
	QString templateId;         // Stable key: "qlogic_isp1020"
	quint8 revision;
	quint32 classCode;
	quint32 size;
	quint16 minAlignment;
	bool is64Bit;
	quint16 barIndex;
	int	allowedWidths;

	// ========================================================================
	// RESOURCE REQUIREMENTS
	// ========================================================================

	QVector<BarTemplate> bars;    // BAR (MMIO window) requirements
	QVector<IrqTemplate> irqs;    // IRQ requirements (typically 1, MSI-X can be more)
	// BAR requirements
/*	QVector<BARDescriptor> bars;*/

	// ========================================================================
	// DEFAULTS 
	// ========================================================================
	quint8 defaultDeviceTmpIPL = 20;
	bool supportsMSI = false;   // Future: MSI/MSI-X
	quint8 msiVectors = 0;
	// ========================================================================
	// DMA CAPABILITIES
	// ========================================================================

	DMACapabilities dmaCaps;      // DMA addressing, coherency, fencing

	// DMA requirements
	quint64 dmaMask = 0xFFFFFFFF; // 32-bit default
	bool dmaCoherent = false;
	bool needsDoorbellFence = false;

	// Defaults for device-specific config
	quint32 queueDepthDefault = 32; // HBA/NIC


	// ========================================================================
	// DEGRADATION POLICY
	// ========================================================================

	bool exposeWhenDegraded;      // Show to guest even if allocation fails?
	// true: register stub handlers (console UARTs)
	// false: hide completely (default)

// ========================================================================
// BACKEND HINTS (OPTIONAL)
// ========================================================================

// Maximum queue depths, timeouts, etc. (device-specific)
	struct BackendHints {
		int maxCommandQueueDepth;  // Max outstanding commands (0 = unlimited)
		int commandTimeoutMs;      // Command timeout in milliseconds
		int maxSGListEntries;      // Max scatter-gather entries
		int maxTransferSize;       // Max single transfer size in bytes

		BackendHints()
			: maxCommandQueueDepth(256)
			, commandTimeoutMs(30000)
			, maxSGListEntries(128)
			, maxTransferSize(1024 * 1024)
		{
		}
	};

	BackendHints backendHints;    // Backend implementation hints

	// ========================================================================
	// CONSTRUCTOR WITH DEFAULTS
	// ========================================================================

	DeviceTemplate()
		: deviceClass(mmio_DeviceClass::UNKNOWN)
		, exposeWhenDegraded(false)
	{
	}

	// ========================================================================
	// VALIDATION
	// ========================================================================

	bool isValid(QString* whyNot = nullptr) const {

		// Child-only devices (no MMIO/IRQ): allow empty bars/irqs
		if (deviceClass == mmio_DeviceClass::SCSI_DISK || deviceClass == mmio_DeviceClass::SCSI_TAPE) {
			return true;
		}

		// Must have at least one BAR
		if (bars.isEmpty()) {
			if (whyNot) *whyNot = "Template has no BARs defined";
			return false;
		}

		// Must have at least one IRQ (unless explicitly DMA-only device)
		if (irqs.isEmpty()) {
			if (whyNot) *whyNot = "Template has no IRQs defined";
			return false;
		}

		// Validate BAR indices are unique
		QSet<quint8> barIndices;
		for (const BarTemplate& bar : bars) {
			if (barIndices.contains(bar.barIndex)) {
				if (whyNot) *whyNot = QString("Duplicate BAR index: %1").arg(bar.barIndex);
				return false;
			}
			barIndices.insert(bar.barIndex);

			// Validate BAR size and alignment
			if (bar.size == 0) {
				if (whyNot) *whyNot = QString("BAR%1 has zero size").arg(bar.barIndex);
				return false;
			}

			if (bar.minAlignment == 0 || (bar.minAlignment & (bar.minAlignment - 1)) != 0) {
				if (whyNot) *whyNot = QString("BAR%1 alignment not power of 2").arg(bar.barIndex);
				return false;
			}
		}

		// Validate IRQ IPLs
		// for (auto& irqDef : deviceTemplate.irqs) {
		// 	IrqSourceId id = m_router->registerDevice(
		// 		irqDef.ipl,
		// 		irqDef.vector,
		// 		irqDef.trigger,
		// 		irqDef.routing,
		// 		irqDef.affinityCpu);
		// 	device->bindIrqSourceId(id);  // device stores its assigned source ID
		// }

		return true;
	}

	// ========================================================================
	// STATISTICS
	// ========================================================================

	quint64 totalMMIOSize() const {
		quint64 total = 0;
		for (const BarTemplate& bar : bars) {
			total += bar.size;
		}
		return total;
	}

	int totalIRQCount() const {
		return irqs.size();
	}
};

#endif
