#ifndef VIRTUAL_TAPE_DEVICE_H
#define VIRTUAL_TAPE_DEVICE_H
// ============================================================================
// VirtualTapeDevice.H  -  Minimal SCSI Sequential-Access Tape Device
// ============================================================================
// This header defines a basic virtual SCSI tape device compatible with
// SSC-3 style sequential-access commands such as:
//    - TEST UNIT READY
//    - INQUIRY
//    - READ BLOCK LIMITS
//    - READ / WRITE
//    - SPACE
//    - WRITE FILEMARKS
//
// The tape data is represented as a QIODevice (typically a QFile) holding a
// linear byte stream. Filemarks are simulated as special markers.
//
// Design constraints:
//   - Header-only implementation.
//   - Pure ASCII, UTF-8 (no BOM).
//   - Depends only on QtCore and SCSI headers (no coreLib, no controllerLib).
//
// References:
//   - SSC-3 (SCSI Stream Commands)
//   - SPC-3
//
// ============================================================================



#include <QtGlobal>
#include <QFile>
#include <QString>

#include "ScsiTypes.H"
#include "ScsiOpcodes.H"
#include "ScsiCbd.h"
#include "ScsiSenseData.H"
#include "ScsiCommand.H"
#include "VirtualScsiDevice.H"

// ============================================================================
// Filemark Encoding
// ============================================================================
// A very small, simple filemark representation:
//   - We insert a special 4-byte pattern into the data stream where a tape
//     filemark would occur.
//   - This is *not* a real tape container format, but it acts sufficiently
//     for OS behaviors that only expect filemarks to exist.
//
// Pattern:  0xFF 0xFF 0xFF 0xFF
//
// ============================================================================

static const quint32 VIRT_TAPE_FILEMARK_VALUE = 0xFFFFFFFFu;

// ============================================================================
// VirtualTapeDevice
// ============================================================================

class VirtualTapeDevice : public VirtualScsiDevice
{
public:
	VirtualTapeDevice(QIODevice* backend, bool ownBackend) noexcept
		: m_backend(backend)
		, m_ownBackend(ownBackend)
		, m_vendor("ENVSYS  ")
		, m_product("VIRT-TAPE     ")
		, m_revision("0001")
		, m_loaded(true)
	{
	}

	VirtualTapeDevice(const QString& tapPath) noexcept
		: m_backend(nullptr)
		, m_ownBackend(true)
		, m_vendor("ENVSYS  ")
		, m_product("VIRT-TAPE     ")
		, m_revision("0001")
		, m_loaded(false)
	{
		QFile* f = new QFile(tapPath);
		if (f->open(QIODevice::ReadWrite))
		{
			m_backend = f;
			m_loaded = true;
		}
		else
		{
			delete f;
			m_backend = nullptr;
			m_loaded = false;
		}
	}

	virtual ~VirtualTapeDevice() noexcept override
	{
		if (m_ownBackend && m_backend)
		{
			m_backend->close();
			delete m_backend;
		}
	}

	// ------------------------------------------------------------------------
	// Device Type
	// ------------------------------------------------------------------------
	virtual ScsiPeripheralDeviceType deviceType() const noexcept override
	{
		return ScsiPeripheralDeviceType::SequentialAccessDevice;
	}

	// ------------------------------------------------------------------------
	// INQUIRY
	// ------------------------------------------------------------------------
	virtual void buildInquiryData(QByteArray& out) const noexcept override
	{
		out.resize(36);
		std::memset(out.data(), 0, 36);

		// Byte 0: PDT=1 (tape)
		out[0] = static_cast<char>(
			static_cast<quint8>(deviceType()) & 0x1F);

		out[2] = 0x05; // Version: SPC-3
		out[3] = 0x02; // Response format: SPC-3
		out[4] = 31;   // Additional length

		copyField(out.data() + 8, 8, m_vendor);
		copyField(out.data() + 16, 16, m_product);
		copyField(out.data() + 32, 4, m_revision);
	}

	// ------------------------------------------------------------------------
	// Tape devices do NOT report block size / count
	// ------------------------------------------------------------------------
	virtual quint32 logicalBlockSize() const noexcept override
	{
		return 0;
	}
	virtual quint64 logicalBlockCount() const noexcept override
	{
		return 0;
	}

	// ------------------------------------------------------------------------
	// Core SCSI handling
	// ------------------------------------------------------------------------
	virtual bool handleCommand(ScsiCommand& cmd) noexcept override
	{
		if (!m_loaded || !m_backend)
		{
			cmd.setCheckCondition(scsiSense_NotReadyMediumAbsent());
			return true;
		}

		const quint8 op = scsiCdbGetOpcode(cmd.cdb);

		switch (op)
		{
		case ScsiOpcode6::TEST_UNIT_READY:
			return handleTestUnitReady(cmd);

		case ScsiOpcode6::INQUIRY:
			return handleInquiry(cmd);

		case ScsiOpcode6::READ_BLOCK_LIMITS:
			return handleReadBlockLimits(cmd);

		case ScsiOpcode6::READ6:
			return handleRead(cmd);

		case ScsiOpcode6::WRITE6:
			return handleWrite(cmd);

		case ScsiOpcode6::SPACE:
			return handleSpace(cmd);

		case ScsiOpcode6::WRITE_FILEMARKS:
			return handleWriteFilemarks(cmd);

		default:
			cmd.setCheckCondition(scsiSense_IllegalOpcode());
			return true;
		}
	}

	virtual bool supportsTaggedQueueing() const noexcept override
	{
		return false;
	}

	virtual void reset() noexcept override
	{
		// For tape, reset = rewind + no unit attention.
		if (m_backend)
		{
			m_backend->seek(0);
		}
	}

private:

	// =====================================================================
	// TEST UNIT READY
	// =====================================================================
	inline bool handleTestUnitReady(ScsiCommand& cmd) noexcept
	{
		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	// =====================================================================
	// INQUIRY
	// =====================================================================
	inline bool handleInquiry(ScsiCommand& cmd) noexcept
	{
		QByteArray inq;
		buildInquiryData(inq);

		const quint8 alloc = cmd.cdb[4];
		const quint32 copyBytes =
			qMin(static_cast<int>(alloc),
				inq.size());

		if (cmd.dataBuffer && cmd.dataTransferLength >= copyBytes)
		{
			std::memcpy(cmd.dataBuffer, inq.constData(), copyBytes);
			cmd.dataTransferred = copyBytes;
			cmd.status = ScsiStatus::Good;
		}
		else
		{
			cmd.dataTransferred = 0;
			cmd.status = ScsiStatus::Good;
		}
		return true;
	}

	// =====================================================================
	// READ BLOCK LIMITS
	// =====================================================================
	// SSC-3 requires this to return:
	//   - Max block length (3 bytes)
	//   - Min block length (2 bytes)
	// We return a simple fixed block limit.
	//
	inline bool handleReadBlockLimits(ScsiCommand& cmd) noexcept
	{
		quint8 resp[6] = {};

		// Max block length = 0x00 0x40 0x00  (16384 bytes)
		resp[1] = 0x00;
		resp[2] = 0x40;
		resp[3] = 0x00;

		// Min block length = 0x0001
		resp[4] = 0x00;
		resp[5] = 0x01;

		const quint8 alloc = cmd.cdb[4];
		const quint32 toCopy = qMin<quint32>(alloc, sizeof(resp));

		if (cmd.dataBuffer && cmd.dataTransferLength >= toCopy)
		{
			std::memcpy(cmd.dataBuffer, resp, toCopy);
			cmd.dataTransferred = toCopy;
		}
		cmd.status = ScsiStatus::Good;
		return true;
	}

	// =====================================================================
	// READ (sequential)
	// =====================================================================
	inline bool handleRead(ScsiCommand& cmd) noexcept
	{
		const quint32 req = scsiCdbGetTransferLengthFromCdb6(cmd.cdb);
		if (req == 0)
		{
			cmd.status = ScsiStatus::Good;
			cmd.dataTransferred = 0;
			return true;
		}

		const quint32 bytes = req;  // treat blocks = bytes for tape model

		if (!cmd.dataBuffer || cmd.dataTransferLength < bytes)
		{
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			return true;
		}

		const qint64 r =
			m_backend->read(static_cast<char*>(cmd.dataBuffer),
				static_cast<qint64>(bytes));

		if (r <= 0)
		{
			// End-of-tape or error -> report BLANK CHECK
			cmd.setCheckCondition(scsiSense_UnrecoveredReadError());
			cmd.dataTransferred = 0;
			return true;
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = static_cast<quint32>(r);
		return true;
	}

	// =====================================================================
	// WRITE (sequential)
	// =====================================================================
	inline bool handleWrite(ScsiCommand& cmd) noexcept
	{
		const quint32 req = scsiCdbGetTransferLengthFromCdb6(cmd.cdb);
		if (req == 0)
		{
			cmd.status = ScsiStatus::Good;
			cmd.dataTransferred = 0;
			return true;
		}

		const quint32 bytes = req;

		if (!cmd.dataBuffer || cmd.dataTransferLength < bytes)
		{
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			return true;
		}

		const qint64 w =
			m_backend->write(static_cast<const char*>(cmd.dataBuffer),
				static_cast<qint64>(bytes));

		if (w != static_cast<qint64>(bytes))
		{
			cmd.setCheckCondition(scsiSense_InternalHardwareError());
			cmd.dataTransferred = (w > 0) ? w : 0;
			return true;
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = bytes;
		return true;
	}

	// =====================================================================
	// SPACE (filemarks)
	// =====================================================================
	inline bool handleSpace(ScsiCommand& cmd) noexcept
	{
		// CDB6 SPACE format:
		//   Byte 1: Code (0=filemarks, 1=blocks, etc)
		//   Byte 2-4: Count
		//
		const quint8 code = cmd.cdb[1] & 0x07;
		const quint32 count =
			(static_cast<quint32>(cmd.cdb[2]) << 16) |
			(static_cast<quint32>(cmd.cdb[3]) << 8) |
			(static_cast<quint32>(cmd.cdb[4]));

		if (code == 0)
		{
			// SPACE FILEMARKS
			// Very naive implementation: simply searches filemark patterns.
			for (quint32 i = 0; i < count; ++i)
			{
				if (!skipToNextFilemark())
				{
					cmd.setCheckCondition(scsiSense_UnrecoveredReadError());
					return true;
				}
			}
			cmd.status = ScsiStatus::Good;
			cmd.dataTransferred = 0;
			return true;
		}

		// Other SPACE codes (block, end-of-data) not implemented.
		cmd.setCheckCondition(scsiSense_IllegalOpcode());
		return true;
	}

	// =====================================================================
	// WRITE FILEMARKS
	// =====================================================================
	inline bool handleWriteFilemarks(ScsiCommand& cmd) noexcept
	{
		const quint32 count =
			(static_cast<quint32>(cmd.cdb[2]) << 16) |
			(static_cast<quint32>(cmd.cdb[3]) << 8) |
			(static_cast<quint32>(cmd.cdb[4]));

		for (quint32 i = 0; i < count; ++i)
		{
			writeFilemark();
		}

		cmd.status = ScsiStatus::Good;
		cmd.dataTransferred = 0;
		return true;
	}

	// =====================================================================
	// Helpers
	// =====================================================================
	inline void copyField(char* dst, int len, const QByteArray& src) const noexcept
	{
		std::memset(dst, ' ', len);
		const int n = qMin(len, src.size());
		if (n > 0)
		{
			std::memcpy(dst, src.constData(), n);
		}
	}

	// Inserts the filemark marker into the tape byte stream.
	inline void writeFilemark() noexcept
	{
		const quint32 v = VIRT_TAPE_FILEMARK_VALUE;
		m_backend->write(reinterpret_cast<const char*>(&v), 4);
	}

	// Skip forward to next filemark pattern. Returns true if found.
	inline bool skipToNextFilemark() noexcept
	{
		quint32 buf;
		while (m_backend->read(reinterpret_cast<char*>(&buf), 4) == 4)
		{
			if (buf == VIRT_TAPE_FILEMARK_VALUE)
			{
				return true;
			}
		}
		return false;
	}

private:
	QIODevice* m_backend;
	bool       m_ownBackend;

	QByteArray m_vendor;
	QByteArray m_product;
	QByteArray m_revision;

	bool       m_loaded;
};

#endif // VIRTUAL_TAPE_DEVICE_H
