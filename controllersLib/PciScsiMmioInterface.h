// ============================================================================
// PciScsiMmioInterface.H  -  Abstract MMIO Interface for PCI SCSI Controllers
// ============================================================================
// Purpose:
//   Defines the minimal MMIO interface required for any PCI-attached
//   SCSI controller in the emulator. This interface is implemented by:
//
//       • PciScsiDevice
//       • PciScsiDeviceShell
//       • Any derived concrete controller
//
//   The emulator's PCI/MMIO layer will call these methods for guest reads
//   and writes into the device's BAR-mapped regions.
//
//   This interface does NOT assume:
//     - SafeMemory
//     - AlphaCPU
//     - MMIOManager
//     - Physical/virtual address translation
//
//   Only the derived PCI SCSI device knows how to interpret the offsets.
//
// Design Constraints:
//   - Header-only
//   - QtCore only: QtGlobal, QString, QByteArray
//   - Pure ASCII (UTF-8, no BOM)
//   - No MMIOManager, no SafeMemory
//
// ============================================================================

#ifndef PCI_SCSI_MMIO_INTERFACE_H
#define PCI_SCSI_MMIO_INTERFACE_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include "PAL_core.h"

// ============================================================================
// PciScsiMmioInterface
// ============================================================================
//
// This is the core MMIO abstraction. Any PCI SCSI controller MUST implement:
//
//   • associatedBarIndex()
//   • barSize()
//   • mmioRegionName()
//   • mmioRead8/16/32/64()
//   • mmioWrite8/16/32/64()
//
// These functions are invoked when the guest performs MMIO accesses using:
//
//   - LDxU / STx   operations (Alpha side)
//   - PCI config routines (PCI bus emulation)
//   - BAR-mapped MMIO reads/writes
//
// ============================================================================
class PciScsiMmioInterface
{
	quint32 m_deviceUid;		// MMIOManager unique identification member
public:
	virtual ~PciScsiMmioInterface() noexcept = default;

	// ------------------------------------------------------------------------
	// BAR index + size
	// ------------------------------------------------------------------------

	// Which BAR number (0–5) is used for this MMIO region.
	virtual int associatedBarIndex() const noexcept = 0;

	// Size of the BAR-backed region (in bytes).
	virtual quint32 barSize() const noexcept = 0;

	// Human-friendly name for logging/debugging.
	virtual QString mmioRegionName() const noexcept = 0;

	// ------------------------------------------------------------------------
	// MMIO Reads (guest ->
	// controller)
	// ------------------------------------------------------------------------

	virtual quint8  mmioRead8(quint64 offset) noexcept = 0;
	virtual quint16 mmioRead16(quint64 offset) noexcept = 0;
	virtual quint32 mmioRead32(quint64 offset) noexcept = 0;
	virtual quint64 mmioRead64(quint64 offset) noexcept = 0;

	// ------------------------------------------------------------------------
	// MMIO Writes (guest ->
	// controller)
	// ------------------------------------------------------------------------

	virtual void mmioWrite8(quint64 offset, quint8  value) noexcept = 0;
	virtual void mmioWrite16(quint64 offset, quint16 value) noexcept = 0;
	virtual void mmioWrite32(quint64 offset, quint32 value) noexcept = 0;
	virtual void mmioWrite64(quint64 offset, quint64 value) noexcept = 0;
	quint32 deviceUid() const noexcept { return m_deviceUid; }
	virtual void setDeviceUid(quint32 uid) noexcept { m_deviceUid = uid; }

	virtual void mmioReset() noexcept  { /* default: no-op */ }
	virtual void mmioFence(palCore_FenceKind) noexcept  { /* default: no-op */ }

};

#endif // PCI_SCSI_MMIO_INTERFACE_H
