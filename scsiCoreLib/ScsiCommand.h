// ============================================================================
// ScsiCommand.H  -  Core SCSI Command Object for scsiCoreLib
// ============================================================================
// This header defines the fundamental SCSI command object used for the
// interaction between:
//
//   - SCSI controllers (in controllerLib)
//   - Virtual SCSI targets/devices (in scsiCoreLib)
//
// A ScsiCommand instance represents a single CDB + associated data buffers,
// status, and sense data. Controllers fill it in and submit it to a target;
// the target updates status, sense, and transfer counts, and returns.
//
// Design Constraints:
//   - Header-only, no .CPP is required.
//   - Depends only on QtCore (QtGlobal) and the C++ standard library.
//   - No dependency on coreLib, controllerLib, AlphaCPU, MMIO, PAL, IPR,
//     PTE, or any other CPU-related facility.
//   - Pure ASCII, UTF-8 (no BOM).
//
// SCSI References:
//   - SCSI Primary Commands - 3 (SPC-3)
//       * Section 4.3   "Commands overview"
//       * Section 4.4   "Sense data format"
//       * Section 4.5   "Status codes"
//   - SCSI Block Commands - 3 (SBC-3)
//       * READ/WRITE, READ CAPACITY, SYNCHRONIZE CACHE, etc.
//   - SCSI Architecture Model - 2 (SAM-2)
//       * Task and command model, task attributes.
//
// ============================================================================

#ifndef SCSI_COMMAND_H
#define SCSI_COMMAND_H

#include <QtGlobal>
#include <cstdint>
#include <cstring>

#include "ScsiTypes.H"      // ScsiStatus, ScsiDataDirection, ScsiLun, etc.
#include "ScsiSenseData.H"  // ScsiFixedSenseData

// ============================================================================
// ScsiCommand - representation of a single SCSI command
// ============================================================================
//
// This struct is the "contract" between a SCSI controller and a SCSI target.
//
// Typical lifecycle:
//   1. The controller decodes a CDB from a guest I/O descriptor.
//   2. It populates a ScsiCommand instance:
//        - cdb pointer and length
//        - dataDirection
//        - dataBuffer and dataTransferLength
//        - LUN
//   3. It calls target->handleCommand(cmd).
//   4. The target:
//        - Performs the requested operation (READ, WRITE, INQUIRY, etc.).
//        - Updates:
//            cmd.dataTransferred
//            cmd.status
//            cmd.serviceResult
//            cmd.senseData (if status == CheckCondition)
//   5. The controller completes the guest I/O, translating status/sense to
//      the guest-visible completion status.
//
// Note:
//   - The ScsiCommand struct does not own any external data buffers; it only
//     contains raw pointers and lengths. The controller or a higher-level
//     DMA subsystem is responsible for allocating and managing the buffers.
//
// ============================================================================

struct ScsiCommand
{
	// ------------------------------------------------------------------------
	// CDB fields
	// ------------------------------------------------------------------------

	// Pointer to the CDB bytes for this command.
	// This must remain valid for the duration of the command.
	//
	// Reference: SPC-3 Section 4.3.1 "Command descriptor block (CDB)".
	//
	const quint8* cdb;

	// Length of the CDB in bytes (typically 6, 10, 12, or 16).
	quint8 cdbLength;

	// Logical Unit Number that this command is addressed to.
	// Normally supplied by transport or controller context.
	ScsiLun lun;

	// Task attribute requested by the initiator (for tagged queueing).
	// Simple by default.
	ScsiTaskAttribute taskAttribute;

	// ------------------------------------------------------------------------
	// Data transfer fields
	// ------------------------------------------------------------------------

	// Direction of data movement, if any.
	//
	//   None       - No data phase (e.g., TEST UNIT READY).
	//   ToDevice   - Initiator to target (WRITE commands).
	//   FromDevice - Target to initiator (READ commands).
	//
	// Reference: SAM-2, "Task" and "Command" models.
	//
	ScsiDataDirection dataDirection;

	// Pointer to the data buffer associated with the command.
	//
	// For READ-like operations, this is the buffer into which the target
	// writes data. For WRITE-like operations, this is the buffer from which
	// the target reads data.
	//
	// Ownership:
	//   - The controller (or a higher-level buffer manager) owns this memory.
	//   - The target must not free or reallocate the buffer.
	//
	void* dataBuffer;

	// Requested transfer length (in bytes or blocks, depending on command).
	//
	// For SBC-3 block commands like READ10/WRITE10, this is normally the
	// number of logical blocks that should be transferred, as decoded
	// from the CDB. The actual number of bytes is typically:
	//
	//   transferLengthBlocks * logicalBlockSize
	//
	// but the controller or target may cache the logicalBlockSize separately.
	//
	quint32 dataTransferLength;

	// Actual number of bytes transferred by the target.
	//
	// For example:
	//   - If the command completes successfully, this should typically match
	//     dataTransferLength (in bytes).
	//   - If the command fails part-way, this may be less than requested.
	//
	quint32 dataTransferred;

	// ------------------------------------------------------------------------
	// Status and service result
	// ------------------------------------------------------------------------

	// SCSI status code reported by the target.
	//
	//   Good             - Command completed successfully.
	//   CheckCondition   - Sense data valid (error or unit attention).
	//   Busy, QueueFull  - Target not able to process right now.
	//
	// Reference: SPC-3 Section 4.5 "Status codes".
	//
	ScsiStatus status;

	// Service result from the emulator's perspective (host-side outcome).
	//
	// This is distinct from status:
	//   - status is what the guest OS will see.
	//   - serviceResult is how the emulator reports internal outcomes
	//     (e.g., host file I/O error, internal logic error).
	//
	ScsiServiceResult serviceResult;

	// ------------------------------------------------------------------------
	// Sense data (for CHECK CONDITION)
	// ------------------------------------------------------------------------

	// Fixed-format sense data buffer (18 bytes).
	//
	// The target may populate this when setting:
	//   status == ScsiStatus::CheckCondition
	//
	// Reference: SPC-3 Section 4.4.2 "Fixed format sense data".
	//
	ScsiFixedSenseData senseData;

	// Number of valid sense bytes in senseData (<= 18).
	quint8 senseDataLength;

	// Indicates whether the senseData field has been initialized with
	// meaningful content. When false, senseData should be treated as
	// undefined.
	bool senseValid;

	// ------------------------------------------------------------------------
	// Miscellaneous control flags
	// ------------------------------------------------------------------------

	// When true, indicates that this command should be treated as a
	// "force unit attention clear" or similar; this is left for use
	// by higher-level logic if needed. For most basic virtual devices,
	// it can be ignored or always left false.
	bool clearUnitAttention;

	// Reserved for future extensions (alignment-friendly).
	quint8 reserved0;
	quint16 reserved1;

	// ------------------------------------------------------------------------
	// Constructor and helper methods
	// ------------------------------------------------------------------------

	ScsiCommand() noexcept
	{
		reset();
	}

	// Reset all fields to a known default state.
	//
	// Defaults:
	//   - cdb            = nullptr
	//   - cdbLength      = 0
	//   - lun            = 0
	//   - taskAttribute  = ScsiTaskAttribute::Simple
	//   - dataDirection  = ScsiDataDirection::None
	//   - dataBuffer     = nullptr
	//   - dataTransferLength = 0
	//   - dataTransferred    = 0
	//   - status         = ScsiStatus::Good
	//   - serviceResult  = ScsiServiceResult::Success
	//   - senseData      = cleared
	//   - senseDataLength= 0
	//   - senseValid     = false
	//
	inline void reset() noexcept
	{
		cdb = nullptr;
		cdbLength = 0;
		lun = ScsiLun(0);
		taskAttribute = ScsiTaskAttribute::Simple;

		dataDirection = ScsiDataDirection::None;
		dataBuffer = nullptr;
		dataTransferLength = 0;
		dataTransferred = 0;

		status = ScsiStatus::Good;
		serviceResult = ScsiServiceResult::Success;

		senseData = ScsiFixedSenseData();
		senseDataLength = 0;
		senseValid = false;

		clearUnitAttention = false;
		reserved0 = 0;
		reserved1 = 0;
	}

	// Convenience helper to set a CHECK CONDITION with supplied sense.
	//
	// The target can call this when an error is detected:
	//
	//   cmd.setCheckCondition(scsiSense_InvalidFieldInCdb());
	//
	inline void setCheckCondition(const ScsiFixedSenseData& s) noexcept
	{
		status = ScsiStatus::CheckCondition;
		serviceResult = ScsiServiceResult::Success; // host-side success
		senseData = s;
		senseDataLength = static_cast<quint8>(ScsiFixedSenseData::size());
		senseValid = true;
	}

	// Clear any existing sense data and set GOOD status.
	inline void clearSense() noexcept
	{
		senseData = ScsiFixedSenseData();
		senseDataLength = 0;
		senseValid = false;
		// Caller may still change status later; we do not force it here.
	}

	// Helper that returns true if this command has a data phase
	// (either ToDevice or FromDevice).
	inline bool hasDataPhase() const noexcept
	{
		return dataDirection == ScsiDataDirection::ToDevice
			|| dataDirection == ScsiDataDirection::FromDevice
			|| dataDirection == ScsiDataDirection::Bidirectional;
	}
};

#endif // SCSI_COMMAND_H
