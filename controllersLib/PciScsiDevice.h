// ============================================================================
// PciScsiDevice.H  -  Unified PCI SCSI Controller Shell (PCI layer)
// ============================================================================
// This header defines a unifying base class for PCI-attached SCSI controllers.
// It combines:
//
//   - PciScsiControllerBase   (SCSI host adapter + PCI config)
//   - PciScsiMmioInterface    (MMIO register exposure)
//
// The intent is that *all* concrete PCI SCSI controllers in your emulator
// derive from PciScsiDevice and then add their own:
//
//   - Register map (offset -> register decode in mmioRead/mmioWrite)
//   - Doorbell / mailbox semantics for CDB submission
//   - Interrupt line semantics (raise/clear IRQs)
//   - Any script engines or DMA engines (in a higher, emulator-specific layer)
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore, ScsiControllerLib.H, and prior PCI SCSI headers.
//   - Does NOT depend on AlphaCPU, SafeMemory, MMIOManager, PAL, PTE, etc.
//   - Pure ASCII; UTF-8 (no BOM).
//
// Integration model:
//   Emulator PCI/MMIO layer will:
//
//     1. Instantiate a derived class (e.g., QLogicPciScsiDevice).
//     2. Register its BAR(s) with the PCI bus.
//     3. Route MMIO reads/writes to mmioRead8/16/32/64 and mmioWrite8/16/32/64.
//     4. Connect its IRQ line output to the platform IRQ controller.
//
// The SCSI pipeline is handled by GenericScsiHostAdapter (adapter()) and the
// underlying scsiCoreLib virtual devices. DMA and guest memory mapping are
// intentionally left to higher layers.
//
// ============================================================================

#ifndef PCI_SCSI_DEVICE_H
#define PCI_SCSI_DEVICE_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QMutex>

#include "ScsiControllerLib.H"
#include "PciScsiController.H"
#include "PciScsiMmioInterface.H"

// ============================================================================
// PciScsiDevice
// ============================================================================
//
// Abstract base class that joins:
//
//   - PciScsiControllerBase   (SCSI host adapter + PCI config)
//   - PciScsiMmioInterface    (MMIO-visible register set)
//
// Responsibilities of this class:
//
//   - Provide a shared place for:
//       * PCI identity (vendorId, deviceId, classCode, BARs)
//       * Generic SCSI helpers for sync/async command execution
//       * Common IRQ line and enable flag
//   - Leave all *device-specific* details abstract:
//
//       * Exact register layout and semantics
//       * How MMIO writes are interpreted as commands
//       * How status and completion are exposed to the guest
//       * How interrupts are wired into your emulator's IRQ model
//
// Concrete controllers (e.g., QLogicPciScsiDevice) derive from this and
// implement:
//
//   - associatedBarIndex()
//   - barSize()
//   - mmioRead8/16/32/64()
//   - mmioWrite8/16/32/64()
//   - raiseInterrupt() / clearInterrupt() (via emulator IRQ layer)
// ============================================================================

class PciScsiDevice : public PciScsiControllerBase,
	public PciScsiMmioInterface
{
public:
	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   bus           - ScsiBus to which the adapter's targets/LUNs attach.
	//   initiatorName - logical initiator name (e.g., "PCI-SCSI0").
	//   initiatorWwn  - 64-bit identity for this adapter.
	//
	explicit PciScsiDevice(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn) noexcept
		: PciScsiControllerBase(bus, initiatorName, initiatorWwn)
		, m_irqLine(0xFF)
		, m_irqEnabled(false)
	{
	}

	virtual ~PciScsiDevice() noexcept override = default;

	// Device Accessors



	// ------------------------------------------------------------------------
	// IRQ configuration helpers (PCI-level view)
	// ------------------------------------------------------------------------
	//
	// These are controller-local representations of the PCI interrupt line
	// and enable flag. Actual delivery of interrupts to CPUs is left to the
	// emulator's PCI and IRQ subsystems, via raiseInterrupt() / clearInterrupt().
	//
	inline void setIrqLine(quint8 line) noexcept
	{
		m_irqLine = line;
		config().interruptLine = line;
	}

	inline quint8 irqLine() const noexcept
	{
		return m_irqLine;
	}

	inline void enableInterrupts(bool enable) noexcept
	{
		m_irqEnabled = enable;
	}

	inline bool interruptsEnabled() const noexcept
	{
		return m_irqEnabled;
	}

	// ------------------------------------------------------------------------
	// SCSI helper methods
	// ------------------------------------------------------------------------
	//
	// These helpers give device implementations a safe, concise way to
	// talk to the underlying SCSI infrastructure without mixing PCI/MMIO
	// logic into SCSI details.
	//
	// 1. executeCdbSync:
	//     Perform a blocking SCSI operation using ScsiHostAdapter base.
	//
	// 2. buildTransaction:
	//     Create and initialize a ScsiTransaction for async scheduling.
	//
	// 3. submitAsyncTransaction:
	//     Submit a transaction into the GenericScsiHostAdapter worker thread.
	//
	// Note:
	//   - These helpers do NOT manage data buffers or DMA. Pointers to
	//     guest buffers and length fields are set up by the caller (your
	//     PCI/MMIO code) through the ScsiCommand embedded in the transaction.
	//
	// ------------------------------------------------------------------------

	// Synchronous, one-shot command execution.
	//
	// Typical usage inside a derived controller:
	//
	//   ScsiCommand cmd;
	//   cmd.cdb           = cdbPtr;
	//   cmd.cdbLength     = cdbLen;
	//   cmd.dataBuffer    = dmaBufferPtr;
	//   cmd.dataTransferLength = dmaLength;
	//   cmd.dataDirection = ScsiDataDirection::FromDevice;
	//
	//   bool ok = executeCdbSync(targetId, ScsiLun(lunId), cmd);
	//
	inline bool executeCdbSync(quint8       targetId,
		ScsiLun      lun,
		ScsiCommand& cmd) noexcept
	{
		return adapter().executeCommand(targetId, lun, cmd);
	}

	// Allocate and partially initialize an async ScsiTransaction.
	//
	// This helper:
	//   - Allocates a new ScsiTransaction on the heap.
	//   - Initializes targetId, lun, and transactionId.
	//   - Leaves cmd fields (CDB, buffers, direction) for the caller to fill.
	//
	// The caller is responsible for deleting the transaction after completion.
	//
	inline ScsiTransaction* buildTransaction(quint8    targetId,
		ScsiLun   lun,
		ScsiTargetPort* targetPort) noexcept
	{
		if (!targetPort)
		{
			return nullptr;
		}

		ScsiTransaction* txn = new ScsiTransaction();
		txn->target = targetPort;
		txn->targetId = targetId;
		txn->lun = lun;
		txn->cmd.lun = lun;

		// Assign transactionId and bind to initiator/adapter via helper.
		adapter().prepareTransaction(*txn, targetPort, targetId, lun);

		return txn;
	}

	// Submit a pre-built transaction for asynchronous processing via the
	// GenericScsiHostAdapter worker thread.
	//
	// The transaction must already have its CDB, buffers, and direction set.
	// The backend (typically your derived PCI device) will receive completion
	// notifications via onTransactionComplete().
	//
	inline void submitAsyncTransaction(ScsiTransaction* txn) noexcept
	{
		adapter().submitAsync(txn);
	}

	// ------------------------------------------------------------------------
	// PciScsiMmioInterface pure virtuals to be implemented by concrete devices
	// ------------------------------------------------------------------------
	//
	// These methods define the MMIO register space as seen by the guest.
	// Your concrete controller must implement a register map that interprets
	// offset values and returns or stores register contents appropriately.
	//
	// Typical behavior:
	//   - doorbell registers that trigger SCSI operations
	//   - status registers that reflect adapter and queue state
	//   - interrupt status and mask registers
	//
	// ------------------------------------------------------------------------

	// Which BAR index exposes this MMIO region for this device.
	virtual int associatedBarIndex() const noexcept override = 0;

	// BAR size (in bytes) for MMIO mapping.
	virtual quint32 barSize() const noexcept override = 0;

	// Human-readable MMIO region name for logging.
	virtual QString mmioRegionName() const noexcept override = 0;

	// MMIO read/write entry points (guest visible).
	virtual quint8  mmioRead8(quint64 offset) noexcept override = 0;
	virtual quint16 mmioRead16(quint64 offset) noexcept override = 0;
	virtual quint32 mmioRead32(quint64 offset) noexcept override = 0;
	virtual quint64 mmioRead64(quint64 offset) noexcept override = 0;

	virtual void mmioWrite8(quint64 offset, quint8  value) noexcept override = 0;
	virtual void mmioWrite16(quint64 offset, quint16 value) noexcept override = 0;
	virtual void mmioWrite32(quint64 offset, quint32 value) noexcept override = 0;
	virtual void mmioWrite64(quint64 offset, quint64 value) noexcept override = 0;

protected:
	// ------------------------------------------------------------------------
	// Interrupt signaling hooks
	// ------------------------------------------------------------------------
	//
	// These hooks abstract the notion of interrupt delivery. Since this PCI
	// layer is still independent of your emulator core, we declare the hooks
	// abstract and let the derived PCI-device implementation map them to:
	//
	//   - PCI INTA# line and IRQController signaling
	//   - MSI/MSI-X (if you later model them)
	//
	// Contracts:
	//   - raiseInterrupt() should only signal when interruptsEnabled() is true.
	//   - clearInterrupt() should clear any pending state as needed.
	//
	// ------------------------------------------------------------------------

	virtual void raiseInterrupt() noexcept = 0;
	virtual void clearInterrupt() noexcept = 0;

private:
	quint8 m_irqLine;
	bool   m_irqEnabled;

};

#endif // PCI_SCSI_DEVICE_H
