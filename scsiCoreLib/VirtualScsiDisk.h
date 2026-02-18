#ifndef VIRTUAL_SCSI_DISK_H
#define VIRTUAL_SCSI_DISK_H
// ============================================================================
// VirtualScsiDisk.H  -  Virtual Block Device (Disk) for scsiCoreLib
// ============================================================================
// This header defines a concrete implementation of VirtualScsiDevice that
// models a simple SBC-3 style block device (disk). It supports a core set
// of SCSI commands typically needed by operating systems such as OpenVMS,
// Windows NT, and UNIX-like systems:
//
//   - TEST UNIT READY
//   - INQUIRY
//   - READ CAPACITY (10)
//   - READ (6), READ (10)
//   - WRITE (6), WRITE (10)
//   - SYNCHRONIZE CACHE (10)               (implemented as a no-op or flush)
//   - MODE SENSE (6) (very minimal stub)
//
// The device is backed by a QIODevice instance (for example, QFile) which
// represents the underlying disk image or raw disk. The caller can either
// provide an already-open QIODevice or let this class create a QFile from
// a file path.
//
// Design Constraints:
//   - Header-only, no .CPP file is required.
//   - Depends only on QtCore and the SCSI core headers in scsiCoreLib.
//   - Does not depend on coreLib, controllerLib, AlphaCPU, PAL, MMIO, PTE,
//     or any other CPU-related facilities.
//   - Pure ASCII, UTF-8 (no BOM).
//
// SCSI References:
//   - SPC-3 Section 6.1  "INQUIRY"
//   - SPC-3 Section 4.4  "Sense Data Format"
//   - SPC-3 Section 4.5  "Status codes"
//   - SBC-3 Section 5.2  "READ (6), READ (10), READ (12), READ (16)"
//   - SBC-3 Section 5.3  "WRITE (6), WRITE (10), WRITE (12), WRITE (16)"
//   - SBC-3 Section 5.10 "READ CAPACITY (10)"
//   - SBC-3 Section 5.32 "SYNCHRONIZE CACHE (10)"
//
// Notes:
//   - This implementation is intentionally modest and correct rather than
//     exhaustive. It can be extended later with more mode pages, caching
//     behavior, defect management, and protection information.
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
// VirtualScsiDisk - basic SBC-3 block device
// ============================================================================
//
// Key design choices:
//   - Backend is any QIODevice. This allows:
//       * QFile for disk images
//       * Custom QIODevice subclasses for container-backed disks
//   - Block size and geometry are supplied by the caller or derived from
//     the backend size.
//   - Minimal but correct INQUIRY, READ CAPACITY, and basic READ/WRITE.
//
// ============================================================================

class VirtualScsiDisk : public VirtualScsiDevice
{
public:
	// Constructor taking an existing QIODevice.
	//
	// Parameters:
	//   backend       - Pointer to an already allocated QIODevice.
	//   logicalSize   - Logical block size in bytes (for example, 512 or 4096).
	//   takeOwnership - If true, VirtualScsiDisk will delete backend in its
	//                   destructor. If false, the caller retains ownership.
	//
	VirtualScsiDisk(QIODevice* backend,
		quint32    logicalSize,
		bool       takeOwnership) noexcept
		: m_backend(backend)
		, m_ownBackend(takeOwnership)
		, m_blockSize(logicalSize)
		, m_blockCount(0)
		, m_vendorId("ENVSYS  ")
		, m_productId("VIRT-DISK      ")
		, m_productRev("0001")
	{
		updateCapacityFromBackend();
	}

	// Convenience constructor that opens a QFile as the backend.
	//
	// Parameters:
	//   imagePath  - Path to the disk image file.
	//   logicalSize- Logical block size in bytes (for example, 512).
	//
	VirtualScsiDisk(const QString& imagePath,
		quint32        logicalSize) noexcept
		: m_backend(nullptr)
		, m_ownBackend(true)
		, m_blockSize(logicalSize)
		, m_blockCount(0)
		, m_vendorId("ENVSYS  ")
		, m_productId("VIRT-DISK      ")
		, m_productRev("0001")
	{
		QFile* file = new QFile(imagePath);
		if (file->open(QIODevice::ReadWrite))
		{
			m_backend = file;
			updateCapacityFromBackend();
		}
		else
		{
			delete file;
			m_backend = nullptr;
			m_blockCount = 0;
		}
	}

	virtual ~VirtualScsiDisk() noexcept override
	{
		if (m_ownBackend && m_backend)
		{
			m_backend->close();
			delete m_backend;
			m_backend = nullptr;
		}
	}

	// ------------------------------------------------------------------------
	// VirtualScsiDevice interface implementation
	// ------------------------------------------------------------------------

	virtual ScsiPeripheralDeviceType deviceType() const noexcept override
	{
		// DirectAccessBlockDevice represents a disk-style block device.
		// Reference: SPC-3 Table 58.
		return ScsiPeripheralDeviceType::DirectAccessBlockDevice;
	}

	virtual void buildInquiryData(QByteArray& outBuffer) const noexcept override
	{
		// Build a standard 36-byte INQUIRY response.
		// Reference: SPC-3 Section 6.1, standard INQUIRY data format.

		outBuffer.resize(36);
		char* d = outBuffer.data();
		std::memset(d, 0, 36);

		// Byte 0: Peripheral Qualifier (bits 7..5) and Peripheral Device Type (bits 4..0).
		d[0] = static_cast<char>(static_cast<quint8>(deviceType()) & 0x1F);

		// Byte 1: Removable Medium bit (bit 7), here set to non-removable (0).
		d[1] = 0x00;

		// Byte 2: Version. 0x05 indicates SPC-3.
		d[2] = 0x05;

		// Byte 3: Response Data Format (bits 7..4). 0x02 indicates SPC-3 format.
		d[3] = 0x02;

		// Byte 4: Additional Length (n), where total length is n + 5.
		// For 36-byte INQUIRY data, n = 31.
		d[4] = 31;

		// Bytes 8..15: Vendor Identification (8 bytes).
		copyPaddedField(d + 8, 8, m_vendorId);

		// Bytes 16..31: Product Identification (16 bytes).
		copyPaddedField(d + 16, 16, m_productId);

		// Bytes 32..35: Product Revision Level (4 bytes).
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

		const quint8 opcode = scsiCdbGetOpcode(cmd.cdb);

		switch (opcode)
		{
		case ScsiOpcode6::TEST_UNIT_READY:
			return handleTestUnitReady(cmd);

		case ScsiOpcode6::INQUIRY:
			return handleInquiry(cmd);

		case ScsiOpcode6::MODE_SENSE6:
			return handleModeSense6(cmd);

		case ScsiOpcode6::READ6:
			return handleRead6(cmd);

		case ScsiOpcode6::WRITE6:
			return handleWrite6(cmd);

		case ScsiOpcode10::READ_CAPACITY10:
			return handleReadCapacity10(cmd);

		case ScsiOpcode10::READ10:
			return handleRead10(cmd);

		case ScsiOpcode10::WRITE10:
			return handleWrite10(cmd);

		case ScsiOpcode10::SYNCHRONIZE_CACHE10:
			return handleSynchronizeCache10(cmd);

		default:
			// Unsupported opcode, return ILLEGAL REQUEST.
			cmd.setCheckCondition(scsiSense_IllegalOpcode());
			return true;
		}
	}

	virtual bool supportsTaggedQueueing() const noexcept override
	{
		// Basic virtual disk does not require tagged queueing support.
		return false;
	}

	virtual bool flushCache() noexcept override
	{
		// If the backend is a QFile, we can attempt a flush by closing
		// and reopening, but that is heavy-handed. For now, assume the
		// operating system will handle cache flush semantics. A more
		// advanced implementation could be added later.
		return true;
	}

	virtual void reset() noexcept override
	{
		// Reset does not need to do anything for a basic virtual disk.
		// More advanced implementations may clear unit attention or
		// revalidate media.
	}

	// ------------------------------------------------------------------------
	// Disk geometry helpers
	// ------------------------------------------------------------------------

	// Recomputes the logical block count from the backend size.
	//
	// If the backend is not available or the block size is zero, block count
	// is set to zero.
	//
	inline void updateCapacityFromBackend() noexcept
	{
		if (!m_backend || !m_backend->isOpen() || m_blockSize == 0)
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

		m_blockCount = static_cast<quint64>(sizeBytes / static_cast<qint64>(m_blockSize));
	}

	// Simple setters for vendor and product strings.
	// Strings are automatically padded or truncated to the correct length.
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

protected:
	// ------------------------------------------------------------------------
	// Internal backend helpers
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
		if (!backendReady())
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	inline bool handleInquiry(ScsiCommand& cmd) noexcept
	{
		QByteArray inquiry;
		buildInquiryData(inquiry);

		const quint8 allocLen = cmd.cdb ? cmd.cdb[4] : 0;
		const quint32 maxToCopy =
			static_cast<quint32>(qMin(static_cast<int>(allocLen), inquiry.size()));

		if (cmd.dataBuffer && maxToCopy > 0 && cmd.dataTransferLength >= maxToCopy)
		{
			std::memcpy(cmd.dataBuffer, inquiry.constData(), maxToCopy);
			cmd.dataTransferred = maxToCopy;
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
		// Minimal MODE SENSE (6) implementation that returns an empty
		// mode page set with proper header. This is often sufficient for
		// simple guest OS use until more detailed mode pages are required.
		//
		// Reference: SPC-3 Section 6.8 "MODE SENSE (6) command".

		quint8 response[4] = {};
		// Byte 0: Mode Data Length (n). For this minimal response, 3.
		response[0] = 3;
		// Byte 1: Medium Type (0 for direct-access devices).
		response[1] = 0;
		// Byte 2: Device-specific parameter. Set to 0 for now.
		response[2] = 0;
		// Byte 3: Block Descriptor Length (0 means no block descriptors).
		response[3] = 0;

		const quint8 allocLen = cmd.cdb ? cmd.cdb[4] : 0;
		const quint32 toCopy = qMin<quint32>(allocLen, sizeof(response));

		if (cmd.dataBuffer && toCopy > 0 && cmd.dataTransferLength >= toCopy)
		{
			std::memcpy(cmd.dataBuffer, response, toCopy);
			cmd.dataTransferred = toCopy;
		}
		else
		{
			cmd.dataTransferred = 0;
		}

		cmd.status = ScsiStatus::Good;
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

		// Last Logical Block Address = blockCount - 1.
		const quint32 lastLba =
			(m_blockCount > 0)
			? static_cast<quint32>(m_blockCount - 1)
			: 0;

		// Big-endian encode lastLba and block size.
		buffer[0] = static_cast<quint8>((lastLba >> 24) & 0xFF);
		buffer[1] = static_cast<quint8>((lastLba >> 16) & 0xFF);
		buffer[2] = static_cast<quint8>((lastLba >> 8) & 0xFF);
		buffer[3] = static_cast<quint8>((lastLba) & 0xFF);

		buffer[4] = static_cast<quint8>((m_blockSize >> 24) & 0xFF);
		buffer[5] = static_cast<quint8>((m_blockSize >> 16) & 0xFF);
		buffer[6] = static_cast<quint8>((m_blockSize >> 8) & 0xFF);
		buffer[7] = static_cast<quint8>((m_blockSize) & 0xFF);

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

	inline bool handleRead6(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint32 lba = scsiCdbGetLbaFromCdb6(cmd.cdb);
		const quint32 blocks = scsiCdbGetTransferLengthFromCdb6(cmd.cdb);
		return performReadBlocks(cmd, lba, blocks);
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

	inline bool handleWrite6(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint32 lba = scsiCdbGetLbaFromCdb6(cmd.cdb);
		const quint32 blocks = scsiCdbGetTransferLengthFromCdb6(cmd.cdb);
		return performWriteBlocks(cmd, lba, blocks);
	}

	inline bool handleWrite10(ScsiCommand& cmd) noexcept
	{
		if (!backendReady() || m_blockSize == 0)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint32 lba = scsiCdbGetLbaFromCdb10(cmd.cdb);
		const quint32 blocks = scsiCdbGetTransferLengthFromCdb10(cmd.cdb);
		return performWriteBlocks(cmd, lba, blocks);
	}

	inline bool handleSynchronizeCache10(ScsiCommand& cmd) noexcept
	{
		Q_UNUSED(cmd);
		// Basic implementation: assume data is always written through.
		// A more complex cache layer could be integrated later.
		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	// ------------------------------------------------------------------------
	// Block I/O helpers
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
			static_cast<quint64>(lba) * static_cast<quint64>(m_blockSize);
		const quint64 byteCount =
			static_cast<quint64>(blocks) * static_cast<quint64>(m_blockSize);

		// Check bounds.
		if (lba >= m_blockCount || (lba + blocks) > m_blockCount)
		{
			cmd.setCheckCondition(scsiSense_UnrecoveredReadError());
			return true;
		}

		if (!cmd.dataBuffer || cmd.dataTransferLength < byteCount)
		{
			// Buffer too small; treat as internal error.
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

	inline bool performWriteBlocks(ScsiCommand& cmd,
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
			static_cast<quint64>(lba) * static_cast<quint64>(m_blockSize);
		const quint64 byteCount =
			static_cast<quint64>(blocks) * static_cast<quint64>(m_blockSize);

		// Check bounds. For writes, it is typical to reject writes that
		// extend past the current media size. Extension could be allowed
		// by growing the file, but that is not implemented here.
		if (lba >= m_blockCount || (lba + blocks) > m_blockCount)
		{
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

		const qint64 writtenBytes =
			m_backend->write(static_cast<const char*>(cmd.dataBuffer),
				static_cast<qint64>(byteCount));

		if (writtenBytes != static_cast<qint64>(byteCount))
		{
			cmd.serviceResult = ScsiServiceResult::HostAdapterError;
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			cmd.dataTransferred =
				(writtenBytes > 0) ? static_cast<quint32>(writtenBytes) : 0;
			return true;
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = static_cast<quint32>(writtenBytes);
		return true;
	}

	// ------------------------------------------------------------------------
	// Utility for copying and padding INQUIRY fields
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
};

#endif // VIRTUAL_SCSI_DISK_H
