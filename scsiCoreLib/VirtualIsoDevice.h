// ============================================================================
// VirtualIsoDevice.h - ============================================================================
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

#ifndef VIRTUAL_ISO_DEVICE_H
#define VIRTUAL_ISO_DEVICE_H
// ============================================================================
// VirtualIsoDevice.H  -  Read-Only SCSI CD/DVD (ISO) Device for scsiCoreLib
// ============================================================================
// This header defines a concrete VirtualScsiDevice that emulates a simple
// read-only SCSI-3 CD/DVD style logical unit backed by an ISO image
// (or any other read-only QIODevice providing 2048-byte sectors).
//
// Supported commands (minimal, but sufficient for many OSes):
//   - TEST UNIT READY
//   - INQUIRY
//   - READ CAPACITY (10)
//   - READ (10)
//   - READ (12)            (optional path, implemented here)
//   - MODE SENSE (6)       (minimal "no pages" response)
//   - PREVENT / ALLOW      (tracks logical "load/eject" state only)
//
// Not supported (return ILLEGAL REQUEST):
//   - WRITE family
//   - FORMAT UNIT
//   - SYNCHRONIZE CACHE
//   - Other MMC-5 / CD audio features
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and SCSI core headers (ScsiTypes, ScsiCdb, etc.).
//   - No dependency on coreLib, controllerLib, AlphaCPU, PAL, MMIO, PTE, etc.
//   - Pure ASCII, UTF-8 (no BOM).
//
// SCSI / MMC References:
//   - SPC-3 Section 6.1      "INQUIRY" (standard data)
//   - SBC-3 Section 5.10     "READ CAPACITY (10)"
//   - SBC-3 Section 5.2      "READ (10), READ (12)"
//   - MMC-5 Section 6.1.1    "Logical unit model" (CD-ROM / DVD)
//
// ============================================================================



#include <QtGlobal>
#include <QByteArray>
#include <QIODevice>
#include <QFile>
#include <QString>
#include <cstring>      // std::memcpy

#include "ScsiTypes.H"
#include "ScsiOpcodes.H"
#include "ScsiCbd.h"
#include "ScsiSenseData.H"
#include "ScsiCommand.H"
#include "VirtualScsiDevice.H"

// ============================================================================
// VirtualIsoDevice - read-only CD/DVD logical unit
// ============================================================================
//
// Notes:
//   - Backed by a QIODevice (typically QFile) opened read-only.
//   - Logical block size is typically 2048 bytes for ISO-9660 / UDF.
//   - All write attempts result in DATA PROTECT / WRITE PROTECTED sense.
//
// ============================================================================

class VirtualIsoDevice : public VirtualScsiDevice
{
public:
	// Construct from an existing backend device.
	//
	// Parameters:
	//   backend        - QIODevice that provides the ISO data. Should be
	//                    opened in read-only mode; writes are never issued.
	//   logicalSize    - Logical block size in bytes (usually 2048).
	//   takeOwnership  - If true, this class will delete backend in dtor.
	//
	VirtualIsoDevice(QIODevice* backend,
		quint32    logicalSize,
		bool       takeOwnership) noexcept
		: m_backend(backend)
		, m_ownBackend(takeOwnership)
		, m_blockSize(logicalSize)
		, m_blockCount(0)
		, m_vendorId("ENVSYS  ")
		, m_productId("VIRT-CDROM     ")
		, m_productRev("0001")
		, m_loaded(true)
		, m_preventRemoval(false)
	{
		updateCapacityFromBackend();
	}

	// Construct from a file path to an ISO image.
	//
	// Parameters:
	//   imagePath   - path to ISO image file
	//   logicalSize - logical block size (usually 2048)
	//
	VirtualIsoDevice(const QString& imagePath,
		quint32        logicalSize = 2048u) noexcept
		: m_backend(nullptr)
		, m_ownBackend(true)
		, m_blockSize(logicalSize)
		, m_blockCount(0)
		, m_vendorId("ENVSYS  ")
		, m_productId("VIRT-CDROM     ")
		, m_productRev("0001")
		, m_loaded(false)
		, m_preventRemoval(false)
	{
		QFile* f = new QFile(imagePath);
		if (f->open(QIODevice::ReadOnly))
		{
			m_backend = f;
			m_loaded = true;
			updateCapacityFromBackend();
		}
		else
		{
			delete f;
			m_backend = nullptr;
			m_loaded = false;
			m_blockCount = 0;
		}
	}

	virtual ~VirtualIsoDevice() noexcept override
	{
		if (m_ownBackend && m_backend)
		{
			m_backend->close();
			delete m_backend;
			m_backend = nullptr;
		}
	}

	// ------------------------------------------------------------------------
	// VirtualScsiDevice interface
	// ------------------------------------------------------------------------

	virtual ScsiPeripheralDeviceType deviceType() const noexcept override
	{
		// CdDvdDevice = 0x05 per SPC-3 Table "Peripheral device type codes".
		return ScsiPeripheralDeviceType::CdDvdDevice;
	}

	virtual void buildInquiryData(QByteArray& outBuffer) const noexcept override
	{
		// Standard 36-byte INQUIRY response (SPC-3 Section 6.1).
		outBuffer.resize(36);
		char* d = outBuffer.data();
		std::memset(d, 0, 36);

		// Byte 0: PQ (bits 7..5) + PDT (bits 4..0).
		d[0] = static_cast<char>(static_cast<quint8>(deviceType()) & 0x1F);

		// Byte 1: Removable Medium bit (7). CD/DVD is removable.
		d[1] = static_cast<char>(static_cast<quint8>(0x80)); // RMB=1

		// Byte 2: Version (0x05 for SPC-3).
		d[2] = 0x05;

		// Byte 3: Response Data Format (0x02 for SPC-3).
		d[3] = 0x02;

		// Byte 4: Additional Length (n) where total length = n + 5.
		// For 36-byte INQUIRY data, n = 31.
		d[4] = 31;

		// Vendor ID (8), Product ID (16), Product Revision (4).
		copyPaddedField(d + 8, 8, m_vendorId);
		copyPaddedField(d + 16, 16, m_productId);
		copyPaddedField(d + 32, 4, m_productRev);
	}

	virtual quint32 logicalBlockSize() const noexcept override
	{
		return m_blockSize;
	}

	virtual quint64 logicalBlockCount() const noexcept override
	{
		return m_blockCount;
	}

	virtual bool handleCommand(ScsiCommand& cmd) noexcept override
	{
		cmd.serviceResult = ScsiServiceResult::Success;
		cmd.dataTransferred = 0;

		if (!m_loaded || !backendReady())
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint8 opcode = scsiCdbGetOpcode(cmd.cdb);

		switch (opcode)
		{
		case ScsiOpcode6::TEST_UNIT_READY:
			return handleTestUnitReady(cmd);

		case ScsiOpcode6::INQUIRY:
			return handleInquiry(cmd);

		case ScsiOpcode6::MODE_SENSE6:
			return handleModeSense6(cmd);

		case ScsiOpcode6::PREVENT_ALLOW:
			return handlePreventAllow(cmd);

		case ScsiOpcode10::READ_CAPACITY10:
			return handleReadCapacity10(cmd);

		case ScsiOpcode10::READ10:
			return handleRead10(cmd);

		case ScsiOpcode12::READ12:
			return handleRead12(cmd);

			// Any write-related opcodes should be rejected as write-protected.
		case ScsiOpcode6::WRITE6:
		case ScsiOpcode10::WRITE10:
		case ScsiOpcode12::WRITE12:
			cmd.setCheckCondition(scsiSense_WriteProtected());
			return true;

		default:
			cmd.setCheckCondition(scsiSense_IllegalOpcode());
			return true;
		}
	}

	virtual bool supportsTaggedQueueing() const noexcept override
	{
		// Basic CD emulator does not require tagged queueing in this model.
		return false;
	}

	virtual bool flushCache() noexcept override
	{
		// Read-only device; no cache to flush at this layer.
		return true;
	}

	virtual void reset() noexcept override
	{
		// For a CD/DVD device, reset typically returns it to "loaded" and
		// "ready" with no additional unit attention for this simple model.
		if (m_backend)
		{
			m_backend->seek(0);
		}
	}

	// ------------------------------------------------------------------------
	// Configuration helpers
	// ------------------------------------------------------------------------

	inline void setVendorId(const QByteArray& vendor) noexcept
	{
		m_vendorId = vendor.left(8);
		while (m_vendorId.size() < 8)
		{
			m_vendorId.append(' ');
		}
	}

	inline void setProductId(const QByteArray& product) noexcept
	{
		m_productId = product.left(16);
		while (m_productId.size() < 16)
		{
			m_productId.append(' ');
		}
	}

	inline void setProductRevision(const QByteArray& rev) noexcept
	{
		m_productRev = rev.left(4);
		while (m_productRev.size() < 4)
		{
			m_productRev.append(' ');
		}
	}

	// Recompute capacity from backend size and block size.
	inline void updateCapacityFromBackend() noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			m_blockCount = 0;
			return;
		}

		const qint64 sizeBytes = m_backend->size();
		if (sizeBytes <= 0)
		{
			m_blockCount = 0;
			return;
		}

		m_blockCount = static_cast<quint64>(
			sizeBytes / static_cast<qint64>(m_blockSize));
	}

protected:
	// ------------------------------------------------------------------------
	// Backend helper
	// ------------------------------------------------------------------------
	inline bool backendReady() const noexcept
	{
		return (m_backend != nullptr) && m_backend->isOpen();
	}

	// ------------------------------------------------------------------------
	// Command-specific handlers
	// ------------------------------------------------------------------------

	inline bool handleTestUnitReady(ScsiCommand& cmd) noexcept
	{
		Q_UNUSED(cmd);
		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	inline bool handleInquiry(ScsiCommand& cmd) noexcept
	{
		QByteArray inquiry;
		buildInquiryData(inquiry);

		const quint8 allocLen = cmd.cdb ? cmd.cdb[4] : 0;
		const quint32 toCopy =
			static_cast<quint32>(
				qMin(static_cast<int>(allocLen), inquiry.size()));

		if (cmd.dataBuffer && toCopy > 0 && cmd.dataTransferLength >= toCopy)
		{
			std::memcpy(cmd.dataBuffer, inquiry.constData(), toCopy);
			cmd.dataTransferred = toCopy;
		}
		else
		{
			cmd.dataTransferred = 0;
		}

		cmd.status = ScsiStatus::Good;
		return true;
	}

	inline bool handleModeSense6(ScsiCommand& cmd) noexcept
	{
		// Minimal MODE SENSE (6) that returns only a header with no pages.
		// This is typically enough to keep guests happy until you add
		// detailed CD-ROM mode pages (e.g., for caching).
		//
		// Reference: SPC-3 Section 6.8 "MODE SENSE (6)".

		quint8 resp[4] = {};
		resp[0] = 3;   // Mode data length
		resp[1] = 0;   // Medium type
		resp[2] = 0;   // Device-specific parameter
		resp[3] = 0;   // Block descriptor length

		const quint8 allocLen = cmd.cdb ? cmd.cdb[4] : 0;
		const quint32 toCopy =
			qMin<quint32>(allocLen, sizeof(resp));

		if (cmd.dataBuffer && toCopy > 0 && cmd.dataTransferLength >= toCopy)
		{
			std::memcpy(cmd.dataBuffer, resp, toCopy);
			cmd.dataTransferred = toCopy;
		}
		else
		{
			cmd.dataTransferred = 0;
		}

		cmd.status = ScsiStatus::Good;
		return true;
	}

	inline bool handlePreventAllow(ScsiCommand& cmd) noexcept
	{
		// SPC-3 PREVENT/ALLOW removal: we only track a flag, and do not
		// emulate actual media ejection. Bit 0 of byte 4 is the Prevent bit.
		//
		// If you later add media-change support, this flag can be used to
		// deny eject operations when set.

		const quint8 prevent = cmd.cdb ? (cmd.cdb[4] & 0x01) : 0;
		m_preventRemoval = (prevent != 0);

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	inline bool handleReadCapacity10(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0 || m_blockCount == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		quint8 buffer[8] = {};

		// Last logical block address = blockCount - 1 (32-bit truncated).
		const quint32 lastLba =
			(m_blockCount > 0)
			? static_cast<quint32>(m_blockCount - 1)
			: 0;

		buffer[0] = static_cast<quint8>((lastLba >> 24) & 0xFF);
		buffer[1] = static_cast<quint8>((lastLba >> 16) & 0xFF);
		buffer[2] = static_cast<quint8>((lastLba >> 8) & 0xFF);
		buffer[3] = static_cast<quint8>(lastLba & 0xFF);

		buffer[4] = static_cast<quint8>((m_blockSize >> 24) & 0xFF);
		buffer[5] = static_cast<quint8>((m_blockSize >> 16) & 0xFF);
		buffer[6] = static_cast<quint8>((m_blockSize >> 8) & 0xFF);
		buffer[7] = static_cast<quint8>(m_blockSize & 0xFF);

		const quint32 toCopy = qMin<quint32>(8, cmd.dataTransferLength);

		if (cmd.dataBuffer && toCopy >= 8)
		{
			std::memcpy(cmd.dataBuffer, buffer, 8);
			cmd.dataTransferred = 8;
		}
		else
		{
			cmd.dataTransferred = 0;
		}

		cmd.status = ScsiStatus::Good;
		return true;
	}

	inline bool handleRead10(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint32 lba = scsiCdbGetLbaFromCdb10(cmd.cdb);
		const quint32 blocks = scsiCdbGetTransferLengthFromCdb10(cmd.cdb);

		return performReadBlocks(cmd, lba, blocks);
	}

	inline bool handleRead12(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint32 lba = scsiCdbGetLbaFromCdb12(cmd.cdb);
		const quint32 blocks = scsiCdbGetTransferLengthFromCdb12(cmd.cdb);

		return performReadBlocks(cmd, lba, blocks);
	}

	// ------------------------------------------------------------------------
	// Block read helper (read-only)
	// ------------------------------------------------------------------------
	inline bool performReadBlocks(ScsiCommand& cmd,
		quint32      lba,
		quint32      blocks) noexcept
	{
		if (blocks == 0)
		{
			cmd.status = ScsiStatus::Good;
			cmd.dataTransferred = 0;
			return true;
		}

		const quint64 byteOffset =
			static_cast<quint64>(lba) *
			static_cast<quint64>(m_blockSize);
		const quint64 byteCount =
			static_cast<quint64>(blocks) *
			static_cast<quint64>(m_blockSize);

		// Range check: do not allow reading past end-of-image.
		if (lba >= m_blockCount || (static_cast<unsigned long long>(lba) + blocks) > m_blockCount)
		{
			// Report unrecovered read error as the simplest mapping.
			cmd.setCheckCondition(scsiSense_UnrecoveredReadError());
			return true;
		}

		if (!cmd.dataBuffer || cmd.dataTransferLength < byteCount)
		{
			cmd.serviceResult = ScsiServiceResult::InternalError;
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			return true;
		}

		if (!m_backend->seek(static_cast<qint64>(byteOffset)))
		{
			cmd.serviceResult = ScsiServiceResult::HostAdapterError;
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			return true;
		}

		const qint64 readBytes =
			m_backend->read(static_cast<char*>(cmd.dataBuffer),
				static_cast<qint64>(byteCount));

		if (readBytes != static_cast<qint64>(byteCount))
		{
			cmd.serviceResult = ScsiServiceResult::HostAdapterError;
			cmd.setCheckCondition(scsiSense_UnrecoveredReadError());
			cmd.dataTransferred =
				(readBytes > 0) ? static_cast<quint32>(readBytes) : 0;
			return true;
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = static_cast<quint32>(readBytes);
		return true;
	}

	// ------------------------------------------------------------------------
	// INQUIRY field helper
	// ------------------------------------------------------------------------
	inline static void copyPaddedField(char* dest,
		int   destLen,
		const QByteArray& src) noexcept
	{
		std::memset(dest, ' ', static_cast<size_t>(destLen));
		const int n = qMin(destLen, src.size());
		if (n > 0)
		{
			std::memcpy(dest, src.constData(), static_cast<size_t>(n));
		}
	}

private:
	QIODevice* m_backend;
	bool        m_ownBackend;
	quint32     m_blockSize;
	quint64     m_blockCount;

	QByteArray  m_vendorId;
	QByteArray  m_productId;
	QByteArray  m_productRev;

	bool        m_loaded;
	bool        m_preventRemoval;
};

#endif // VIRTUAL_ISO_DEVICE_H
