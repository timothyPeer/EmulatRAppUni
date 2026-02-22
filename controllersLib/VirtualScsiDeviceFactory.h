// ============================================================================
// VirtualScsiDeviceFactory.h - ============================================================================
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

#ifndef VIRTUAL_SCSI_DEVICE_FACTORY_H
#define VIRTUAL_SCSI_DEVICE_FACTORY_H

// ============================================================================
// VirtualScsiDeviceFactory.H  (Corrected for QVariant-based settings)
// ============================================================================

#include <QtGlobal>
#include <QString>
#include <QByteArray>

#include "settings.h"
#include "VirtualScsiDevice.h"
#include "VirtualScsiDisk.h"
#include "VirtualIsoDevice.h"
#include "VirtualTapeDevice.h"

// ============================================================================
// Factory
// ============================================================================
class VirtualScsiDeviceFactory
{
public:
	static VirtualScsiDevice* createFromConfig(const DeviceConfig& cfg) noexcept
	{
		const QString type = cfg.type.toUpper();

		if (type == "SCSI_DISK")   return createDisk(cfg);
		if (type == "SCSI_TAPE")   return createTape(cfg);

		if (type == "SCSI_ISO" ||
			type == "SCSI_CDROM" ||
			type == "ISO" ||
			type == "CDROM")
		{
			return createIso(cfg);
		}

		return nullptr;
	}

private:
	// =====================================================================
	// DISK
	// =====================================================================
	static VirtualScsiDevice* createDisk(const DeviceConfig& cfg) noexcept
	{
		// Required: image path
		const QString image = cfg.fields.value("image").toString().trimmed();
		if (image.isEmpty())
			return nullptr;

		// Geometry
		quint32 blockSize = 512;
		quint64 qwords = 0;

		const auto geo = cfg.subBlocks.value("geometry");
		if (!geo.isEmpty())
		{
			bool ok = false;
			blockSize = geo.value("blockSize").toUInt(&ok);
			if (!ok || blockSize == 0)
				blockSize = 512;

			qwords = geo.value("qwords").toULongLong(&ok);
			if (!ok)
				qwords = 0;
		}

		// Construct disk
		VirtualScsiDisk* disk = new VirtualScsiDisk(image, blockSize);

		// Identity
		const auto idBlock = cfg.subBlocks.value("identity");
		if (!idBlock.isEmpty())
		{
			disk->setVendorId(idBlock.value("manufacturer").toByteArray());
			disk->setProductId(idBlock.value("model").toByteArray());
			disk->setProductRevision(idBlock.value("serial").toByteArray());
		}

		return disk;
	}

	// =====================================================================
	// TAPE
	// =====================================================================
	static VirtualScsiDevice* createTape(const DeviceConfig& cfg) noexcept
	{
		const QString image = cfg.fields.value("image").toString().trimmed();
		if (image.isEmpty())
			return nullptr;

		VirtualTapeDevice* tape = new VirtualTapeDevice(image);

		// Identity block (not used by tape device currently)
		// but parsed cleanly to avoid warnings
		const auto idBlock = cfg.subBlocks.value("identity");
		Q_UNUSED(idBlock);

		return tape;
	}

	// =====================================================================
	// ISO / CD-ROM
	// =====================================================================
	static VirtualScsiDevice* createIso(const DeviceConfig& cfg) noexcept
	{
		const QString image = cfg.fields.value("image").toString().trimmed();
		if (image.isEmpty())
			return nullptr;

		// Default 2048 for ISO9660
		quint32 blockSize = 2048;

		const auto geo = cfg.subBlocks.value("geometry");
		if (!geo.isEmpty())
		{
			bool ok = false;
			quint32 b = geo.value("blockSize").toUInt(&ok);
			if (ok && b > 0)
				blockSize = b;
		}

		VirtualIsoDevice* iso = new VirtualIsoDevice(image, blockSize);

		const auto idBlock = cfg.subBlocks.value("identity");
		if (!idBlock.isEmpty())
		{
			iso->setVendorId(idBlock.value("manufacturer").toByteArray());
			iso->setProductId(idBlock.value("model").toByteArray());
			iso->setProductRevision(idBlock.value("serial").toByteArray());
		}

		return iso;
	}
};

#endif // VIRTUAL_SCSI_DEVICE_FACTORY_H
