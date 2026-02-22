// ============================================================================
// ScsiTypes.h - ScsiTypes.H - Core SCSI type definitions for scsiCoreLib
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

#ifndef SCSI_TYPES_H
#define SCSI_TYPES_H
//============================================================================
// ScsiTypes.H - Core SCSI type definitions for scsiCoreLib
// ============================================================================
// This header defines the fundamental SCSI enumerations and small helper
// types used by the SCSI core and virtual SCSI devices.
//
// Design constraints:
// - Header-only, no implementation .CPP needed.
// - Depends only on QtCore (for fixed-width types) and the C++ standard
// library. There must be no dependency on coreLib, controllerLib, or any
// Alpha CPU / MMIO / PAL / PTE related code.
// - Suitable for use by both SCSI controllers (in controllerLib) and
// SCSI targets (virtual devices in scsiCoreLib).
//
// References:
// - SCSI Primary Commands - 3 (SPC-3), especially:
// * Section 4.5 "Status codes"
// * Section 4.4 "Sense data format"
// * Section 4.7 "Task attributes"
// * Section 6.1 "INQUIRY data and Peripheral Device Type"
// - SCSI Architecture Model - 2 (SAM-2), task management model.
//
// NOTE: This header is intended to be included by other SCSI headers such as
// ScsiOpcodes.H, ScsiCdb.H, ScsiSenseData.H, and VirtualScsiDevice.H.
// ============================================================================



// QtCore provides the fixed-size integer typedefs (quint8, quint16, etc.)
#include <QtGlobal>

// Standard library includes used only for basic types and convenience.
// (Kept minimal on purpose to avoid unnecessary coupling.)
#include <cstdint>

//
// Data direction for SCSI I/O transfers.
//
// This enum describes the expected direction of data movement between
// the initiator (e.g., the host adapter) and the target (virtual SCSI
// device). It is independent of any bus protocol and may be used by
// controllerLib to size and prepare DMA buffers.
//
// Reference: SAM-2, "Task" and "Command" models.
//
enum class ScsiDataDirection : quint8
{
	None = 0, // No data phase (e.g., TEST UNIT READY).
	ToDevice = 1, // Data flows from initiator to target (WRITE).
	FromDevice = 2, // Data flows from target to initiator (READ, INQUIRY).
	Bidirectional = 3 // Full-duplex or two-phase transfer (rare in practice).
};

//
// SCSI status codes.
//
// These status codes are returned by targets at command completion.
// Only the subset commonly used by disk/tape/optical targets are
// enumerated here, but the enum allows extension if needed.
//
// Reference: SPC-3, Section 4.5 "Status code".
//
enum class ScsiStatus : quint8
{
	Good = 0x00, // Command completed successfully.
	CheckCondition = 0x02, // Sense data available, error or request.
	ConditionMet = 0x04, // For SEARCH, PRE-FETCH, etc.
	Busy = 0x08, // Target or logical unit is busy.
	Intermediate = 0x10, // Obsolete in many modern devices.
	IntermediateConditionMet = 0x14, // Obsolete; for linked commands.
	ReservationConflict = 0x18, // Persistent reservation conflict.
	CommandTerminated = 0x22, // Obsolete; replaced by Task Aborted.
	QueueFull = 0x28, // Device queue is full.
	ACAActive = 0x30, // Auto Contingent Allegiance is active.
	TaskAborted = 0x40 // Task was aborted (e.g., by TMF).
};

//
// SCSI sense keys.
//
// These are stored in the "Sense Key" field of fixed or descriptor sense
// data. They broadly classify error conditions (e.g., NOT_READY vs.
// MEDIUM_ERROR).
//
// Reference: SPC-3, Section 4.4 "Sense data format" and related tables.
//
enum class ScsiSenseKey : quint8
{
	NoSense = 0x00, // No specific sense key; see ASC/ASCQ.
	RecoveredError = 0x01, // Recovered by device, info may be reported.
	NotReady = 0x02, // Device or medium not ready (e.g., no disk).
	MediumError = 0x03, // Unrecoverable read/write error on medium.
	HardwareError = 0x04, // Non-medium hardware failure (controller).
	IllegalRequest = 0x05, // Invalid CDB, LBA, field, or opcode.
	UnitAttention = 0x06, // Media change, reset, or other attention.
	DataProtect = 0x07, // Access blocked (e.g., write-protect).
	BlankCheck = 0x08, // End-of-tape or unwritten area on tape.
	VendorSpecific = 0x09, // Device-specific conditions.
	CopyAborted = 0x0A, // COPY or COPY-like operation aborted.
	AbortedCommand = 0x0B, // Command aborted (not by TMF).
	VolumeOverflow = 0x0D, // End-of-volume on streamed media.
	Miscompare = 0x0E // Data miscompare on verify, etc.
};

//
// Response code values for sense data.
//
// These determine the layout of the sense data (fixed vs. descriptor,
// current vs. deferred).
//
// Reference: SPC-3, Section 4.4.1 "Response code".
//
enum class ScsiSenseResponseCode : quint8
{
	CurrentFixed = 0x70, // Current errors, fixed-format sense.
	DeferredFixed = 0x71, // Deferred errors, fixed-format sense.
	CurrentDescriptor = 0x72, // Current errors, descriptor-format sense.
	DeferredDescriptor = 0x73 // Deferred errors, descriptor-format sense.
};

//
// Task attributes for tagged command queuing.
//
// These indicate how a command should be ordered in the target's internal
// queue when tagged command queuing is enabled.
//
// Reference: SAM-2, "Task Attributes".
//
enum class ScsiTaskAttribute : quint8
{
	Simple = 0x00, // Most common; executed in arrival order.
	HeadOfQueue = 0x01, // Execute before all SIMPLE commands.
	Ordered = 0x02, // Enforce ordering w.r.t other tasks.
	ACA = 0x04 // Auto Contingent Allegiance task.
};

//
// Peripheral Device Type values.
//
// These values appear in the "Peripheral Device Type" field of the
// standard INQUIRY data response. They describe the type of logical
// unit (disk, tape, CD/DVD, etc.).
//
// Reference: SPC-3, Section 6.1 "INQUIRY".
//
enum class ScsiPeripheralDeviceType : quint8
{
	DirectAccessBlockDevice = 0x00, // Disks (HDD/SSD).
	SequentialAccessDevice = 0x01, // Tapes.
	PrinterDevice = 0x02, // Rarely used now.
	ProcessorDevice = 0x03, // Historically for CPUs or bridges.
	WriteOnceDevice = 0x04, // WORM devices.
	CdDvdDevice = 0x05, // CD-ROM / DVD logical unit.
	ScannerDevice = 0x06, // Obsolete in practice.
	OpticalMemoryDevice = 0x07, // Optical disks (MO).
	MediumChangerDevice = 0x08, // Jukebox / library robot.
	CommunicationsDevice = 0x09, // Communication devices.
	StorageArrayControllerDevice = 0x0C, // RAID controllers (exposed logically).
	EnclosureServicesDevice = 0x0D, // SES / SES2 enclosure services.
	SimplifiedDirectAccessDevice = 0x0E, // Reduced-command-set disk.
	OpticalCardReaderWriter = 0x0F, // Optical card devices.
	BridgeControllerCommands = 0x10, // Bridge controllers.
	ObjectBasedStorageDevice = 0x11, // Object storage (OSD).
	AutomationDriveInterface = 0x12, // Automation/drive interface.
	WellKnownLogicalUnit = 0x1E, // Well-known LUN.
	UnknownOrNoDevice = 0x1F // No device / unknown type.
};

//
// Simple enumeration for SCSI service result codes at the emulator level.
//
// This is distinct from ScsiStatus: ScsiStatus is what the target reports
// to the guest OS, while ScsiServiceResult indicates what happened inside
// the emulator while attempting to process the command.
//
// For example, the emulator may fail to read from a backing file (I/O error)
// even though the guest would see a "MediumError" or "HardwareError" via
// ScsiStatus and sense data.
//
// These values are not part of any SCSI standard; they are purely internal
// to scsiCoreLib and controllerLib.
//
enum class ScsiServiceResult : quint8
{
	Success = 0, // Command accepted and processed; see ScsiStatus.
	HostAdapterError = 1, // Error in host adapter / controller emulation.
	TransportError = 2, // Emulated bus/transport failure.
	TargetError = 3, // Target (virtual device) could not process cmd.
	DataOverrun = 4, // More data than expected was supplied.
	DataUnderrun = 5, // Less data than expected was supplied.
	InternalError = 6 // Internal emulator logic error.
};

//
// Helper struct for representing a SCSI LUN (Logical Unit Number).
//
// Many SCSI transports encode LUNs in different formats (flat, hierarchical).
// For the purposes of this emulator, we often just need a small, decoded
// representation that can be used as an index into a LUN map.
//
// This struct is intentionally minimal and can be extended later if you
// decide to support full hierarchical addressing.
//
// Reference: SAM-2, "Logical Units".
//
struct ScsiLun
{
	quint16 value; // Simple 16-bit LUN value for internal mapping.

	ScsiLun() noexcept
		: value(0)
	{
	}

	explicit ScsiLun(quint16 v) noexcept
		: value(v)
	{
	}

	// Comparison operators enable use in maps or sets.
	bool operator==(const ScsiLun& other) const noexcept
	{
		return value == other.value;
	}

	bool operator!=(const ScsiLun& other) const noexcept
	{
		return value != other.value;
	}

	bool operator<(const ScsiLun& other) const noexcept
	{
		return value < other.value;
	}


};

#endif // SCSI_TYPES_H