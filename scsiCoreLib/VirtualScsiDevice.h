#ifndef VIRTUAL_SCSI_DEVICE_H
#define VIRTUAL_SCSI_DEVICE_H
// ============================================================================
// VirtualScsiDevice.H  -  Abstract Base Class for Virtual SCSI Targets
// ============================================================================
// This header defines the base interface for all SCSI targets in scsiCoreLib.
// Controllers (in controllerLib) interact with targets through this interface
// only - not through derived classes directly.
//
// Requirements & Rules:
//   - Header-only.
//   - Depends ONLY on QtCore, ScsiTypes.H, ScsiSenseData.H, ScsiCommand.H.
//   - Absolutely no references to AlphaCPU, MMIO, PAL, PTE, or controllerLib.
//   - Pure ASCII, UTF-8 (no BOM).
//
// SCSI References:
//   - SPC-3, Section 4.3: CDB rules.
//   - SPC-3, Section 6.1: INQUIRY standard data.
//   - SBC-3, Section 5: Block-access device behavior.
//   - SAM-2, logical unit / task management model.
//
// ============================================================================


#include "ScsiTypes.H"
#include "ScsiCommand.H"

// ============================================================================
// VirtualScsiDevice
// ============================================================================
//
// This abstract class represents a SCSI Target (Logical Unit). Examples:
//
//    - VirtualScsiDisk       (SBC-3 block device)
//    - VirtualTapeDevice     (SSC-3 / streamer)
//    - VirtualIsoDevice      (MMC-5 / CD-ROM device)
//    - VirtualEnclosure      (SES / SES-2)
//
// Controllers call handleCommand() to execute I/O requests.
// Derived classes implement SCSI command semantics.
//
// ============================================================================

class VirtualScsiDevice
{
public:
	VirtualScsiDevice() noexcept = default;
	virtual ~VirtualScsiDevice() noexcept = default;

	// ------------------------------------------------------------------------
	// Device Type (INQUIRY Peripheral Device Type)
	// ------------------------------------------------------------------------
	//
	// Each virtual device reports a type such as:
	//    0x00 = Direct Access (disk)
	//    0x01 = Sequential Access (tape)
	//    0x05 = CD/DVD device
	//
	// Reference: SPC-3, Table 58 "Peripheral Device Type Codes".
	//
	virtual ScsiPeripheralDeviceType deviceType() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Standard INQUIRY Data
	// ------------------------------------------------------------------------
	//
	// Derived classes must return at least 36 bytes of INQUIRY data.
	// Many OSes (VMS, NT, Linux) parse vendor ID and product ID fields.
	//
	// Reference: SPC-3, Section 6.1 "INQUIRY".
	//
	virtual void buildInquiryData(QByteArray& outBuffer) const noexcept = 0;

	// ------------------------------------------------------------------------
	// Logical Block Size (for SBC-3 block devices)
	// ------------------------------------------------------------------------
	//
	// For block devices, this is the fundamental unit used by READ/WRITE.
	// For tape and CD/DVD, this returns 0 because block size is not fixed
	// or is handled by mode pages instead.
	//
	// Reference: SBC-3, READ CAPACITY (10/16).
	//
	virtual quint32 logicalBlockSize() const noexcept = 0;

	// ------------------------------------------------------------------------
	// LUN Capacity (for SBC-3 block devices)
	// ------------------------------------------------------------------------
	//
	// Must be implemented by disk/ISO devices; tape devices typically
	// return 0 because they are sequential and capacity is not block-based.
	//
	virtual quint64 logicalBlockCount() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Main Command Handler (critical virtual function)
	// ------------------------------------------------------------------------
	//
	// Notes:
	//   - Controllers initialize ScsiCommand with CDB, buffer, etc.
	//   - The target must update:
	//
	//       cmd.status
	//       cmd.serviceResult
	//       cmd.dataTransferred
	//       cmd.senseData (if CheckCondition)
	//
	//   - Targets should never throw; errors are represented using:
	//        - cmd.status = CheckCondition
	//        - cmd.senseData = scsiSense_* helper
	//
	//   - Return value:
	//         true  => command processed (success or CHECK CONDITION)
	//         false => unsupported opcode or target malfunction
	//
	virtual bool handleCommand(ScsiCommand& cmd) noexcept = 0;

	// ------------------------------------------------------------------------
	// Optional: Cache Support & Tagged Queueing
	// ------------------------------------------------------------------------
	//
	// SupportsTaggedQueueing:
	//     If true, the controller may send SIMPLE / ORDERED / HEAD-OF-QUEUE
	//     tagged commands. Many virtual devices simply return false.
	//
	virtual bool supportsTaggedQueueing() const noexcept { return false; }

	// Some devices may want to emulate a write-back cache or allow syncing
	// via SYNCHRONIZE CACHE. Default is "no-op".
	virtual bool flushCache() noexcept { return true; }

	// ------------------------------------------------------------------------
	// Device Reset
	// ------------------------------------------------------------------------
	//
	// Called by controllers when receiving bus resets, device resets, or
	// certain task management functions.
	//
	virtual void reset() noexcept { /* default: do nothing */ }
};

#endif // VIRTUAL_SCSI_DEVICE_H
