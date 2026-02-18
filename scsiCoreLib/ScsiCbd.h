// ============================================================================
// ScsiCdb.H  -  SCSI Command Descriptor Block (CDB) helpers for scsiCoreLib
// ============================================================================
// This header defines lightweight helper functions and small utility types
// for decoding SCSI Command Descriptor Blocks (CDBs) in 6-, 10-, 12-, and
// 16-byte formats.
//
// Design constraints:
//   - Header only, no .CPP file.
//   - Depends only on QtCore (QtGlobal) and the C++ standard library.
//   - No dependency on coreLib, controllerLib, AlphaCPU, MMIO, PAL, or PTE.
//   - All helpers operate on raw CDB byte arrays (quint8*).
//   - All code is pure ASCII and suitable for UTF-8 (no BOM).
//
// References:
//   - SCSI Primary Commands - 3 (SPC-3)
//       * Section 4.3.1  Command descriptor block (CDB)
//       * Section 4.3.3  Control byte
//   - SCSI Block Commands - 3 (SBC-3)
//       * READ(6), WRITE(6) CDB formats
//       * READ(10), WRITE(10) CDB formats
//       * READ(12), WRITE(12) CDB formats
//       * READ(16), WRITE(16) CDB formats
//
// Note:
//   These helpers intentionally keep the interface simple. They do not
//   perform bounds checking on the CDB pointer or length, so the caller
//   is responsible for ensuring the CDB buffer is valid and large enough
//   for the requested format.
//
//   Typical usage in a virtual device or controller might look like:
//
//       quint8 opcode = scsiCdbGetOpcode(cdb);
//       if (opcode == ScsiOpcode10::READ10)
//       {
//           quint32 lba = scsiCdbGetLbaFromCdb10(cdb);
//           quint32 xfer = scsiCdbGetTransferLengthFromCdb10(cdb);
//           quint8 control = scsiCdbGetControlByteFromCdb10(cdb);
//           // Process READ(10) using lba and xfer.
//       }
//
// ============================================================================

#ifndef SCSI_CDB_H
#define SCSI_CDB_H

#include <QtGlobal>
#include <cstdint>  // for uint32_t, uint64_t

#include "ScsiTypes.H"    // For ScsiLun and basic types.
#include "ScsiOpcodes.H"  // For opcode constants (optional but convenient).

// ============================================================================
// Basic opcode and group helpers
// ============================================================================

// Extracts the opcode (byte 0) from any CDB format.
//
// SPC-3 Section 4.3.1 defines the CDB as a sequence of bytes where the
// first byte is always the operation code.
//
inline quint8 scsiCdbGetOpcode(const quint8* cdb) noexcept
{
	return cdb ? cdb[0] : 0;
}

// Returns the CDB group code (bits 7..5 of the opcode).
// This is sometimes useful for generic decode paths.
//
// SPC-3 Section 4.3.1: Upper three bits of the operation code identify
// the CDB group (e.g., 6 byte, 10 byte, 12 byte, 16 byte).
//
inline quint8 scsiCdbGetGroupCode(const quint8* cdb) noexcept
{
	const quint8 op = scsiCdbGetOpcode(cdb);
	return static_cast<quint8>((op >> 5) & 0x07);
}

// ============================================================================
// LUN extraction for 6-byte CDBs
// ============================================================================
//
// For 6-byte CDBs, the Logical Unit Number is often encoded in bits 7..5
// of byte 1. Many modern devices do not rely on this encoding, since LUN
// is generally conveyed by the transport layer, but emulation may still
// need to decode it.
//
// SBC-3 legacy note: For commands like READ(6) and WRITE(6), byte 1 is:
//   bits 7..5: LUN
//   bits 4..0: MSB of Logical Block Address.
//
inline ScsiLun scsiCdbGetLunFromCdb6(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return ScsiLun(0);
	}
	const quint8 b1 = cdb[1];
	const quint16 lunVal = static_cast<quint16>((b1 >> 5) & 0x07);
	return ScsiLun(lunVal);
}

// ============================================================================
// LBA extraction helpers
// ============================================================================
//
// The following helpers decode Logical Block Address fields from CDBs.
// All values are returned in host-endian form as 32-bit or 64-bit values.
//
// The caller is responsible for ensuring that the CDB actually conforms
// to the expected command format (for example, that the opcode is READ(10)
// before calling scsiCdbGetLbaFromCdb10).
//
// References for field layouts:
//   - SBC-3, READ(6) and WRITE(6) CDBs
//   - SBC-3, READ(10) and WRITE(10) CDBs
//   - SBC-3, READ(12) and WRITE(12) CDBs
//   - SBC-3, READ(16) and WRITE(16) CDBs
//

// 6-byte CDB LBA (READ(6), WRITE(6)).
//
// Layout (bytes):
//   Byte 1: bits 7..5 = LUN, bits 4..0 = LBA[20..16]
//   Byte 2: LBA[15..8]
//   Byte 3: LBA[7..0]
//   Byte 4: Transfer length
//   Byte 5: Control
//
inline quint32 scsiCdbGetLbaFromCdb6(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b1 = cdb[1];
	const quint8 b2 = cdb[2];
	const quint8 b3 = cdb[3];

	const quint32 high = static_cast<quint32>(b1 & 0x1F); // lower 5 bits
	const quint32 lba =
		(high << 16) |
		(static_cast<quint32>(b2) << 8) |
		static_cast<quint32>(b3);

	return lba;
}

// 10-byte CDB LBA (READ(10), WRITE(10), VERIFY(10), etc.).
//
// Layout (bytes):
//   Byte 2: LBA[31..24]
//   Byte 3: LBA[23..16]
//   Byte 4: LBA[15..8]
//   Byte 5: LBA[7..0]
//
inline quint32 scsiCdbGetLbaFromCdb10(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b2 = cdb[2];
	const quint8 b3 = cdb[3];
	const quint8 b4 = cdb[4];
	const quint8 b5 = cdb[5];

	const quint32 lba =
		(static_cast<quint32>(b2) << 24) |
		(static_cast<quint32>(b3) << 16) |
		(static_cast<quint32>(b4) << 8) |
		static_cast<quint32>(b5);

	return lba;
}

// 12-byte CDB LBA (READ(12), WRITE(12), VERIFY(12), etc.).
//
// Layout (bytes):
//   Byte 2: LBA[31..24]
//   Byte 3: LBA[23..16]
//   Byte 4: LBA[15..8]
//   Byte 5: LBA[7..0]
//
inline quint32 scsiCdbGetLbaFromCdb12(const quint8* cdb) noexcept
{
	// For LBA field, CDB12 uses the same four-byte layout as CDB10.
	return scsiCdbGetLbaFromCdb10(cdb);
}

// 16-byte CDB LBA (READ(16), WRITE(16), VERIFY(16), etc.).
//
// Layout (bytes):
//   Byte 2:  LBA[63..56]
//   Byte 3:  LBA[55..48]
//   Byte 4:  LBA[47..40]
//   Byte 5:  LBA[39..32]
//   Byte 6:  LBA[31..24]
//   Byte 7:  LBA[23..16]
//   Byte 8:  LBA[15..8]
//   Byte 9:  LBA[7..0]
//
inline quint64 scsiCdbGetLbaFromCdb16(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b2 = cdb[2];
	const quint8 b3 = cdb[3];
	const quint8 b4 = cdb[4];
	const quint8 b5 = cdb[5];
	const quint8 b6 = cdb[6];
	const quint8 b7 = cdb[7];
	const quint8 b8 = cdb[8];
	const quint8 b9 = cdb[9];

	const quint64 lba =
		(static_cast<quint64>(b2) << 56) |
		(static_cast<quint64>(b3) << 48) |
		(static_cast<quint64>(b4) << 40) |
		(static_cast<quint64>(b5) << 32) |
		(static_cast<quint64>(b6) << 24) |
		(static_cast<quint64>(b7) << 16) |
		(static_cast<quint64>(b8) << 8) |
		static_cast<quint64>(b9);

	return lba;
}

// ============================================================================
// Transfer length helpers
// ============================================================================
//
// These helpers decode the requested transfer length as expressed in the
// CDB. The units are command-specific:
//
//   - For most block commands (e.g., READ, WRITE), the value is in blocks.
//   - For some other commands (e.g., READ BUFFER), the value is in bytes.
//
// SBC-3 should be consulted per command for the exact semantics.
//
// References:
//   - SBC-3 READ(6), READ(10), READ(12), READ(16).
//   - SPC-3 for commands that use length as bytes (e.g., REQUEST SENSE).
//

// 6-byte CDB transfer length.
// Layout (bytes):
//   Byte 4: Transfer length (0 means 256 blocks for READ(6)/WRITE(6)).
//
inline quint32 scsiCdbGetTransferLengthFromCdb6(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 len = cdb[4];
	// For READ(6) and WRITE(6), a length of 0 means 256 logical blocks.
	return (len == 0) ? 256u : static_cast<quint32>(len);
}

// 10-byte CDB transfer length.
//
// Layout (bytes):
//   Byte 7: Transfer length[15..8]
//   Byte 8: Transfer length[7..0]
//
inline quint32 scsiCdbGetTransferLengthFromCdb10(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b7 = cdb[7];
	const quint8 b8 = cdb[8];

	const quint32 len =
		(static_cast<quint32>(b7) << 8) |
		static_cast<quint32>(b8);

	return len;
}

// 12-byte CDB transfer length.
//
// Layout (bytes):
//   Byte 6: Transfer length[31..24]
//   Byte 7: Transfer length[23..16]
//   Byte 8: Transfer length[15..8]
//   Byte 9: Transfer length[7..0]
//
inline quint32 scsiCdbGetTransferLengthFromCdb12(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b6 = cdb[6];
	const quint8 b7 = cdb[7];
	const quint8 b8 = cdb[8];
	const quint8 b9 = cdb[9];

	const quint32 len =
		(static_cast<quint32>(b6) << 24) |
		(static_cast<quint32>(b7) << 16) |
		(static_cast<quint32>(b8) << 8) |
		static_cast<quint32>(b9);

	return len;
}

// 16-byte CDB transfer length.
//
// Layout (bytes) for block commands (e.g., READ(16), WRITE(16)):
//   Byte 10: Transfer length[31..24]
//   Byte 11: Transfer length[23..16]
//   Byte 12: Transfer length[15..8]
//   Byte 13: Transfer length[7..0]
//
inline quint32 scsiCdbGetTransferLengthFromCdb16(const quint8* cdb) noexcept
{
	if (!cdb)
	{
		return 0;
	}

	const quint8 b10 = cdb[10];
	const quint8 b11 = cdb[11];
	const quint8 b12 = cdb[12];
	const quint8 b13 = cdb[13];

	const quint32 len =
		(static_cast<quint32>(b10) << 24) |
		(static_cast<quint32>(b11) << 16) |
		(static_cast<quint32>(b12) << 8) |
		static_cast<quint32>(b13);

	return len;
}

// ============================================================================
// Control byte helpers
// ============================================================================
//
// SPC-3 Section 4.3.3 describes the control byte at the end of CDBs.
// It contains fields such as:
//   - NACA (Normal ACA)
//   - Link
//   - Vendor specific bits
//
// For many virtual device implementations, the control byte is either
// ignored or used only for simple purposes (for example, link bit).
//

inline quint8 scsiCdbGetControlByteFromCdb6(const quint8* cdb) noexcept
{
	return cdb ? cdb[5] : 0;
}

inline quint8 scsiCdbGetControlByteFromCdb10(const quint8* cdb) noexcept
{
	return cdb ? cdb[9] : 0;
}

inline quint8 scsiCdbGetControlByteFromCdb12(const quint8* cdb) noexcept
{
	return cdb ? cdb[11] : 0;
}

inline quint8 scsiCdbGetControlByteFromCdb16(const quint8* cdb) noexcept
{
	return cdb ? cdb[15] : 0;
}

#endif // SCSI_CDB_H
