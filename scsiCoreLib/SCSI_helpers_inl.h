#pragma once
#include <QtGlobal>
#include <QByteArray>
#include "SCSI_helpers_inl.h"


// ============================================================================
// SCSI OPCODE CONSTANTS
// ============================================================================

// ============================================================================
// SCSI Opcodes (Enhanced for OpenVMS Tape Support)
// ============================================================================
namespace SCSIOpcodes {
	// Common commands
	constexpr quint8 TEST_UNIT_READY = 0x00;
	constexpr quint8 REQUEST_SENSE = 0x03;
	constexpr quint8 INQUIRY = 0x12;
	constexpr quint8 MODE_SELECT_6 = 0x15;
	constexpr quint8 MODE_SENSE_6 = 0x1A;
	constexpr quint8 PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E;
	constexpr quint8 READ_CAPACITY_10 = 0x25;
	constexpr quint8 MODE_SELECT_10 = 0x55;
	constexpr quint8 MODE_SENSE_10 = 0x5A;

	// Disk commands
	constexpr quint8 READ_6 = 0x08;
	constexpr quint8 WRITE_6 = 0x0A;
	constexpr quint8 READ_10 = 0x28;
	constexpr quint8 WRITE_10 = 0x2A;
	constexpr quint8 READ_16 = 0x88;
	constexpr quint8 WRITE_16 = 0x8A;

	// Tape commands (Sequential Access)
	constexpr quint8 REWIND = 0x01;
	constexpr quint8 READ_BLOCK_LIMITS = 0x05;
	constexpr quint8 WRITE_FILEMARKS = 0x10;
	constexpr quint8 SPACE = 0x11;
	constexpr quint8 ERASE = 0x19;
	constexpr quint8 LOAD_UNLOAD = 0x1B;
	constexpr quint8 LOCATE_10 = 0x2B;
	constexpr quint8 READ_POSITION = 0x34;
	constexpr quint8 REPORT_DENSITY_SUPPORT = 0x44;

	// Additional tape commands (for completeness)
	constexpr quint8 VERIFY_6 = 0x13;
	constexpr quint8 RECOVER_BUFFERED_DATA = 0x14;
	constexpr quint8 RESERVE_6 = 0x16;
	constexpr quint8 RELEASE_6 = 0x17;
	constexpr quint8 LOCATE_16 = 0x92;
	constexpr quint8 VERIFY_16 = 0x8F;
}

// ============================================================================
// SCSI Sense Keys (Enhanced for Tape Operations)
// ============================================================================
// namespace SCSISenseKey {
// 	constexpr quint8 NO_SENSE = 0x00;
// 	constexpr quint8 RECOVERED_ERROR = 0x01;
// 	constexpr quint8 NOT_READY = 0x02;
// 	constexpr quint8 MEDIUM_ERROR = 0x03;
// 	constexpr quint8 HARDWARE_ERROR = 0x04;
// 	constexpr quint8 ILLEGAL_REQUEST = 0x05;
// 	constexpr quint8 UNIT_ATTENTION = 0x06;
// 	constexpr quint8 DATA_PROTECT = 0x07;
// 	constexpr quint8 BLANK_CHECK = 0x08;
// 	constexpr quint8 VENDOR_SPECIFIC = 0x09;
// 	constexpr quint8 COPY_ABORTED = 0x0A;
// 	constexpr quint8 ABORTED_COMMAND = 0x0B;
// 	constexpr quint8 VOLUME_OVERFLOW = 0x0D;  // Tape: write past end of partition
// 	constexpr quint8 MISCOMPARE = 0x0E;
// 	constexpr quint8 COMPLETED = 0x0F;
// }
// ---------------------------------------------------------------------------
//  SCSI Sense Key enumerations
// ---------------------------------------------------------------------------
enum class SCSISenseKey : quint8
{
	NO_SENSE = 0x00,
	RECOVERED_ERROR = 0x01,
	NOT_READY = 0x02,
	MEDIUM_ERROR = 0x03,
	HARDWARE_ERROR = 0x04,
	ILLEGAL_REQUEST = 0x05,
	UNIT_ATTENTION = 0x06,
	DATA_PROTECT = 0x07,
	BLANK_CHECK = 0x08,
	VENDOR_SPECIFIC = 0x09,
	COPY_ABORTED = 0x0A,
	ABORTED_COMMAND = 0x0B,
	VOLUME_OVERFLOW = 0x0D,
	MISCOMPARE = 0x0E,
	COMPLETED = 0x0F
};
// ============================================================================
// Common ASC/ASCQ Codes (for reference)
// ============================================================================
namespace SCSIAdditionalSense {
	// ASC values
	constexpr quint8 NO_ADDITIONAL_SENSE = 0x00;
	constexpr quint8 FILEMARK_DETECTED = 0x00;  // ASC=0x00, ASCQ=0x01
	constexpr quint8 END_OF_PARTITION = 0x00;   // ASC=0x00, ASCQ=0x02
	constexpr quint8 SETMARK_DETECTED = 0x00;   // ASC=0x00, ASCQ=0x03
	constexpr quint8 END_OF_DATA = 0x00;        // ASC=0x00, ASCQ=0x05

	constexpr quint8 INVALID_COMMAND_OPCODE = 0x20;
	constexpr quint8 LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE = 0x21;
	constexpr quint8 INVALID_FIELD_IN_CDB = 0x24;
	constexpr quint8 LOGICAL_UNIT_NOT_SUPPORTED = 0x25;
	constexpr quint8 INVALID_FIELD_IN_PARAMETER_LIST = 0x26;
	constexpr quint8 WRITE_PROTECTED = 0x27;
	constexpr quint8 NOT_READY_TO_READY_CHANGE = 0x28;
	constexpr quint8 POWER_ON_RESET = 0x29;
	constexpr quint8 PARAMETERS_CHANGED = 0x2A;
	constexpr quint8 MEDIUM_NOT_PRESENT = 0x3A;
	constexpr quint8 SEQUENTIAL_POSITIONING_ERROR = 0x3B;
	constexpr quint8 SAVING_PARAMETERS_NOT_SUPPORTED = 0x39;
	constexpr quint8 MEDIUM_REMOVAL_PREVENTED = 0x53;

	// ASCQ values (for common ASC codes above)
	constexpr quint8 ASCQ_FILEMARK_DETECTED = 0x01;
	constexpr quint8 ASCQ_END_OF_PARTITION = 0x02;
	constexpr quint8 ASCQ_SETMARK_DETECTED = 0x03;
	constexpr quint8 ASCQ_END_OF_DATA = 0x05;
	constexpr quint8 ASCQ_MEDIUM_REMOVAL_PREVENTED = 0x02;
}

// ============================================================================
// Mode Page Codes
// ============================================================================
namespace SCSIModePage {
	constexpr quint8 VENDOR_SPECIFIC = 0x00;
	constexpr quint8 READ_WRITE_ERROR_RECOVERY = 0x01;
	constexpr quint8 DISCONNECT_RECONNECT = 0x02;
	constexpr quint8 FORMAT_DEVICE = 0x03;
	constexpr quint8 RIGID_DISK_GEOMETRY = 0x04;
	constexpr quint8 FLEXIBLE_DISK = 0x05;
	constexpr quint8 CACHING = 0x08;
	constexpr quint8 CONTROL_MODE = 0x0A;
	constexpr quint8 MEDIUM_TYPES_SUPPORTED = 0x0B;
	constexpr quint8 NOTCH_PARTITION = 0x0C;
	constexpr quint8 POWER_CONDITION = 0x0D;
	constexpr quint8 DATA_COMPRESSION = 0x0F;      // Tape: Data compression
	constexpr quint8 DEVICE_CONFIGURATION = 0x10;  // Tape: Device configuration
	constexpr quint8 MEDIUM_PARTITION = 0x11;      // Tape: Medium partition (compression)
	constexpr quint8 INFORMATIONAL_EXCEPTIONS = 0x1C;
	constexpr quint8 ALL_PAGES = 0x3F;
}

// ============================================================================
// SCSI Status Codes
// ============================================================================
namespace SCSIStatus {
	constexpr quint8 GOOD = 0x00;
	constexpr quint8 CHECK_CONDITION = 0x02;
	constexpr quint8 CONDITION_MET = 0x04;
	constexpr quint8 BUSY = 0x08;
	constexpr quint8 INTERMEDIATE = 0x10;
	constexpr quint8 INTERMEDIATE_CONDITION_MET = 0x14;
	constexpr quint8 RESERVATION_CONFLICT = 0x18;
	constexpr quint8 COMMAND_TERMINATED = 0x22;
	constexpr quint8 TASK_SET_FULL = 0x28;
	constexpr quint8 ACA_ACTIVE = 0x30;
	constexpr quint8 TASK_ABORTED = 0x40;
}

// ============================================================================
// SCSI Device Types
// ============================================================================
namespace SCSIDeviceType {
	constexpr quint8 DIRECT_ACCESS = 0x00;      // Disk
	constexpr quint8 SEQUENTIAL_ACCESS = 0x01;  // Tape
	constexpr quint8 PRINTER = 0x02;
	constexpr quint8 PROCESSOR = 0x03;
	constexpr quint8 WRITE_ONCE = 0x04;
	constexpr quint8 CD_ROM = 0x05;
	constexpr quint8 SCANNER = 0x06;
	constexpr quint8 OPTICAL_MEMORY = 0x07;
	constexpr quint8 MEDIUM_CHANGER = 0x08;
	constexpr quint8 COMMUNICATIONS = 0x09;
	constexpr quint8 STORAGE_ARRAY = 0x0C;
	constexpr quint8 ENCLOSURE_SERVICES = 0x0D;
	constexpr quint8 RBC = 0x0E;                // Reduced block commands
	constexpr quint8 OPTICAL_CARD_READER = 0x0F;
	constexpr quint8 BRIDGE_CONTROLLER = 0x10;
	constexpr quint8 OBJECT_BASED_STORAGE = 0x11;
	constexpr quint8 AUTOMATION_DRIVE = 0x12;
	constexpr quint8 WELL_KNOWN_LU = 0x1E;
	constexpr quint8 UNKNOWN = 0x1F;
}

// ============================================================================
// Density Codes (for Tape Devices)
// ============================================================================
namespace SCSIDensityCode {
	constexpr quint8 DEFAULT = 0x00;
	constexpr quint8 QIC_11 = 0x04;
	constexpr quint8 QIC_24 = 0x05;
	constexpr quint8 QIC_120 = 0x0F;
	constexpr quint8 QIC_150 = 0x10;
	constexpr quint8 QIC_320 = 0x11;
	constexpr quint8 QIC_525 = 0x13;
	constexpr quint8 QIC_1350 = 0x14;
	constexpr quint8 QIC_3080 = 0x29;

	constexpr quint8 DDS = 0x13;
	constexpr quint8 DDS2 = 0x24;
	constexpr quint8 DDS3 = 0x25;
	constexpr quint8 DDS4 = 0x26;

	constexpr quint8 DLT_TYPE_III = 0x19;
	constexpr quint8 DLT_TYPE_IV = 0x1A;
	constexpr quint8 SDLT_220 = 0x48;
	constexpr quint8 SDLT_320 = 0x49;
	constexpr quint8 SDLT_600 = 0x4A;

	constexpr quint8 LTO_1 = 0x40;
	constexpr quint8 LTO_2 = 0x42;
	constexpr quint8 LTO_3 = 0x44;
	constexpr quint8 LTO_4 = 0x46;
	constexpr quint8 LTO_5 = 0x58;
	constexpr quint8 LTO_6 = 0x5A;
	constexpr quint8 LTO_7 = 0x5C;
	constexpr quint8 LTO_8 = 0x5D;
	constexpr quint8 LTO_9 = 0x60;

	constexpr quint8 AIT_1 = 0x30;
	constexpr quint8 AIT_2 = 0x31;
	constexpr quint8 AIT_3 = 0x32;
	constexpr quint8 AIT_4 = 0x33;

	constexpr quint8 TRAVAN = 0x27;
}

// ============================================================================
// Service Actions (for multi-function opcodes)
// ============================================================================
namespace SCSIServiceAction {
	// READ POSITION service actions
	constexpr quint8 READ_POSITION_SHORT = 0x00;
	constexpr quint8 READ_POSITION_LONG = 0x06;
	constexpr quint8 READ_POSITION_EXTENDED = 0x08;

	// REPORT DENSITY SUPPORT service actions
	constexpr quint8 REPORT_DENSITY_MEDIA = 0x00;
	constexpr quint8 REPORT_DENSITY_MEDIUM = 0x01;
}

// ============================================================================
// SCSI COMMAND STRUCTURES
// ============================================================================

/**
 * @brief SCSI Command Descriptor Block (CDB).
 *
 * Variable length: 6, 10, 12, or 16 bytes.
 */
struct SCSICommand {
	quint8 opcode;           // SCSI opcode (0x00-0xFF)
	quint8 cdb[16];          // Command bytes (including opcode)
	quint8 cdbLength;        // Actual CDB length (6, 10, 12, 16)

	quint8 targetId;         // SCSI target ID (0-15)
	quint8 lun;              // Logical Unit Number (0-7)

	quint64 dataBufferPA;    // Physical address of data buffer
	quint32 dataLength;      // Expected data transfer length
	bool dataReadIn;             // true = device->host (READ)
	bool dataIn;             // true = device->host, false = host->device

	quint64 statusPA;        // Where to write status byte
	quint64 sensePA;         // Where to write sense data (if error)
	quint32 hbaDeviceUid;    // HBA passes this down
};




// ---------------------------------------------------------------------------
//  Basic SCSI Command Descriptor Block (CDB) container
// ---------------------------------------------------------------------------
struct SCSICdb
{
	QByteArray bytes;          // raw CDB bytes
	int        length{ 0 };   // CDB length (6,10,12,16)

	inline quint8 op() const { return bytes.isEmpty() ? 0 : quint8(bytes[0]); }
	inline const quint8* data() const { return reinterpret_cast<const quint8*>(bytes.constData()); }
	inline quint8        operator[](int i) const { return quint8(bytes[i]); }
};

// ---------------------------------------------------------------------------
//  SCSI Command Result
// ---------------------------------------------------------------------------
struct SCSIResult
{
	quint8   status{ SCSIStatus::GOOD };
	QByteArray  dataOut;     // data returned to initiator (read, inquiry, mode sense)
	int         senseLen{ 0 };
	SCSISenseKey		senseKey;
// 	int		asc{ 0 };
// 	quint8		ascq{ 0 };
	QByteArray  senseData;   // sense buffer for CHECK_CONDITION
	bool        dataIn{ false };   // read direction
	bool        dataOutDir{ false };   // write direction
	int         residual{ 0 };
	int			bytesTransferred{ 0 }; 
	inline void clear()
	{
		status = SCSIStatus::GOOD;
		dataOut.clear();
		senseLen = 0;
		senseData.clear();
		dataIn = false;
		dataOutDir = false;
		residual = 0;
	}
};
