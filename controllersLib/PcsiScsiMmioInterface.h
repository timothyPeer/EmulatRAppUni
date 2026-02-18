// ============================================================================
// PciScsiMmioInterface.H  -  MMIO Bridge Interface for PCI SCSI Controllers
// ============================================================================
// This header defines the abstract MMIO interface for PCI-attached SCSI
// controllers. It sits directly above PciScsiControllerBase and directly
// below the emulator's MMIOManager / PCI bus logic.
//
// A real PCI SCSI controller emulation will:
//   - Derive from PciScsiControllerBase
//   - Implement PciScsiMmioInterface
//   - Map BAR regions and decode guest MMIO reads/writes
//   - Produce ScsiTransactions upon MMIO register access
//   - Forward results via ScsiHostAdapterBackend::onTransactionComplete()
//
// Design constraints:
//   - Header-only (no CPP).
//   - Depends on QtCore + controllerLib (ScsiControllerLib.H).
//   - NO dependency on coreLib, AlphaCPU, SafeMemory, or MMIOManager.
//   - Pure ASCII, UTF-8 (no BOM).
//
// Purpose:
//   Provide a stable, clean interface for "PCI-device-like" behavior
//   without forcing any knowledge of the emulator internals into SCSI code.
//
// ============================================================================

#ifndef PCI_SCSI_MMIO_INTERFACE_H
#define PCI_SCSI_MMIO_INTERFACE_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>

#include "PciScsiController.H"

// ============================================================================
// PciScsiMmioInterface
// ============================================================================
//
// This abstract class defines the MMIO-visible register model of a PCI SCSI
// controller. The emulator's PCI bus / MMIO system will call these methods
// whenever the guest reads or writes addresses that fall within one of the
// controller's BAR regions.
//
// Your real SCSI controller (e.g., QLogic or NCR53C895) will implement:
//   - mmioRead8/16/32/64
//   - mmioWrite8/16/32/64
//
// In most PCI SCSI devices, the MMIO interface includes:
//   - FIFO/doorbell registers
//   - Interrupt status & mask registers
//   - SCSI script processor or command queue
//   - DMA descriptors (if present)
//   - Mailboxes for sending CDBs from guest to adapter
//
// ============================================================================

class PciScsiMmioInterface
{
public:
	virtual ~PciScsiMmioInterface() noexcept = default;

	// ------------------------------------------------------------------------
	// Identify the BAR this MMIO interface belongs to.
	// (Useful if the device has multiple BARs.)
	// ------------------------------------------------------------------------
	virtual int associatedBarIndex() const noexcept = 0;

	// ------------------------------------------------------------------------
	// MMIO READS (guest -> controller)
	// ------------------------------------------------------------------------

	virtual quint8  mmioRead8(quint64 offset) noexcept = 0;
	virtual quint16 mmioRead16(quint64 offset) noexcept = 0;
	virtual quint32 mmioRead32(quint64 offset) noexcept = 0;
	virtual quint64 mmioRead64(quint64 offset) noexcept = 0;

	// ------------------------------------------------------------------------
	// MMIO WRITES (guest -> controller)
	// ------------------------------------------------------------------------

	virtual void mmioWrite8(quint64 offset, quint8  value) noexcept = 0;
	virtual void mmioWrite16(quint64 offset, quint16 value) noexcept = 0;
	virtual void mmioWrite32(quint64 offset, quint32 value) noexcept = 0;
	virtual void mmioWrite64(quint64 offset, quint64 value) noexcept = 0;

	// ------------------------------------------------------------------------
	// Utility: BAR size lookup for MMIO manager
	// ------------------------------------------------------------------------
	virtual quint32 barSize() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Utility: return human-readable name (useful for debugging)
	// ------------------------------------------------------------------------
	virtual QString mmioRegionName() const noexcept = 0;
};

#endif // PCI_SCSI_MMIO_INTERFACE_H

