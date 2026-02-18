#ifndef SCSI_SENSE_DATA_H
#define SCSI_SENSE_DATA_H
// ============================================================================
// ScsiSenseData.H  -  SCSI Sense Data Structures & Builders for scsiCoreLib
// ============================================================================
// This header defines the SCSI "Sense Data" formats used to report error
// conditions back to the initiator (the guest OS). It includes:
//
//    - Fixed Format Sense Data (SPC-3 4.4.2 / Table 32)
//    - Descriptor Format Sense Data (SPC-3 4.4.3)
//    - Minimal sense-builder helpers for common conditions
//
// Design Rules:
//   - Header-only (no .CPP).
//   - Pure ASCII, UTF-8 (no BOM).
//   - Depends only on QtCore + stdlib.
//   - Absolutely no dependencies on coreLib, controllerLib,
//     AlphaCPU, PAL, IPR, MMIO, or SafeMemory.
//
// References:
//   - SPC-3 Section 4.4   "Sense Data Format"
//   - SPC-3 Section 4.5   "Status Codes"
//   - SBC-3 Section 4.23  Error reporting for block devices
//
// ============================================================================



#include <QtGlobal>
#include <cstring>     // for memset
#include "ScsiTypes.H" // Sense keys, response codes, etc.

// ============================================================================
// Fixed Format Sense Data (Current / Deferred) - SPC-3 Table 32
// ============================================================================
//
// Required minimum sense size is 18 bytes, but many devices return 32 or 96.
// We define the classic 18-byte form because it is sufficient for most use
// cases in block/tape/ISO virtual devices.
//
// Layout:
//   Byte  0 : Response Code (0x70 = current, 0x71 = deferred)
//   Byte  1 : Obsolete
//   Byte  2 : Sense Key (plus flags)
//   Byte  3 : Information[3]
//   Byte  4 : Information[2]
//   Byte  5 : Information[1]
//   Byte  6 : Information[0]
//   Byte  7 : Additional Sense Length
//   Byte  8 : Command Specific Info[3]
//   Byte  9 : Command Specific Info[2]
//   Byte 10 : Command Specific Info[1]
//   Byte 11 : Command Specific Info[0]
//   Byte 12 : Additional Sense Code (ASC)
//   Byte 13 : Additional Sense Code Qualifier (ASCQ)
//   Byte 14 : Field Replaceable Unit Code (FRU)
//   Byte 15 : Sense Key Specific[0]
//   Byte 16 : Sense Key Specific[1]
//   Byte 17 : Sense Key Specific[2]
//
// ============================================================================

struct ScsiFixedSenseData
{
	quint8 data[18];

	ScsiFixedSenseData() noexcept
	{
		std::memset(data, 0, sizeof(data));
	}

	// Initialize core fields.
	void init(ScsiSenseResponseCode response,
		ScsiSenseKey key,
		quint8 asc,
		quint8 ascq) noexcept
	{
		std::memset(data, 0, sizeof(data));

		data[0] = static_cast<quint8>(response); // Response code (0x70 or 0x71)
		data[2] = static_cast<quint8>(key);      // Sense key
		data[7] = 0x0A;                          // Additional sense length for 18-byte sense
		data[12] = asc;                          // Additional Sense Code
		data[13] = ascq;                         // Additional Sense Code Qualifier
	}

	const quint8* bytes() const noexcept { return data; }
	quint8* bytes() noexcept { return data; }

	static constexpr int size() noexcept { return 18; }
};

// ============================================================================
// Simple Sense Builders
// ============================================================================
//
// These inline helpers allow a virtual SCSI device to quickly create correct
// sense data for common error conditions.
//
// Note: ASC/ASCQ codes below use common SBC-3 values. They can be customized
//       later if you implement full ASC/ASCQ tables.
// ============================================================================

// "NO SENSE" (0x00/0x00)
inline ScsiFixedSenseData scsiSense_NoSense() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::NoSense,
		0x00, // ASC: no additional sense information
		0x00);
	return s;
}

// "NOT READY - MEDIUM NOT PRESENT" (ASC 0x3A / ASCQ 0x00)
inline ScsiFixedSenseData scsiSense_NotReadyMediumAbsent() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::NotReady,
		0x3A,  // ASC: Medium not present
		0x00); // ASCQ
	return s;
}

// "ILLEGAL REQUEST - INVALID COMMAND OPERATION CODE" (0x20/0x00)
inline ScsiFixedSenseData scsiSense_IllegalOpcode() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::IllegalRequest,
		0x20,  // ASC: Invalid command operation code
		0x00);
	return s;
}

// "ILLEGAL REQUEST - INVALID FIELD IN CDB" (0x24/0x00)
inline ScsiFixedSenseData scsiSense_InvalidFieldInCdb() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::IllegalRequest,
		0x24,  // ASC: Invalid field in CDB
		0x00);
	return s;
}

// "MEDIUM ERROR - UNRECOVERED READ ERROR" (0x11/0x00)
inline ScsiFixedSenseData scsiSense_UnrecoveredReadError() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::MediumError,
		0x11,  // ASC: Unrecovered read error
		0x00);
	return s;
}

// "HARDWARE ERROR - INTERNAL FAILURE" (0x44/0x00)
inline ScsiFixedSenseData scsiSense_InternalHardwareError() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::HardwareError,
		0x44,  // ASC: Internal target failure
		0x00);
	return s;
}

// "DATA PROTECT - WRITE PROTECTED" (0x27/0x00)
inline ScsiFixedSenseData scsiSense_WriteProtected() noexcept
{
	ScsiFixedSenseData s;
	s.init(ScsiSenseResponseCode::CurrentFixed,
		ScsiSenseKey::DataProtect,
		0x27,  // ASC: Write protected
		0x00);
	return s;
}

#endif // SCSI_SENSE_DATA_H
