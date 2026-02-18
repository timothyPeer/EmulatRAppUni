// ============================================================================
// PciScsiLib.H  -  Umbrella Header for PCI-attached SCSI Controllers
// ============================================================================
// Purpose:
//   This header aggregates all PCI-layer SCSI controller building blocks
//   you now have in your project. Including this single file gives a PCI
//   device implementation access to:
//
//     • SCSI controllerLib (ScsiControllerLib.H)
//     • PCI SCSI base classes and helpers:
//         - PciScsiControllerBase
//         - PciScsiMmioInterface
//         - PciScsiDevice
//         - PciScsiRegisterBank
//         - PciScsiMailbox (+ PciScsiMailboxQueue)
//         - PciScsiInterruptController
//         - PciScsiDmaEngine
//         - PciScsiDmaChannel
//         - PciScsiDeviceShell
//         - PciScsiScriptEngine
//         - PciScsiControllerTemplate
//     • Concrete controller skeletons:
//         - ISP1020_Controller
//         - KZPBA_Controller
//     • High-level SCSI controller wrapper:
//         - VirtualScsiController
//
// Design constraints:
//   - Header-only, no CPP.
//   - Depends on QtCore + scsiCoreLib + controllerLib + PCI SCSI headers.
//   - NO dependency on AlphaCPU, SafeMemory, MMIOManager, PAL, or PTE.
//   - Pure ASCII, UTF-8 (no BOM).
//
// Usage:
//   From your PCI bus / MMIO layer, include:
//
//       #include "PciScsiLib.H"
//
//   Then instantiate one of the provided skeletons (e.g. ISP1020_Controller
//   or KZPBA_Controller), wire its BAR into the PCI bus, and route MMIO
//   reads/writes to mmioReadXX/mmioWriteXX.
//
// ============================================================================

#ifndef PCI_SCSI_LIB_H
#define PCI_SCSI_LIB_H

// ---------------------------------------------------------------------------
// Core SCSI controller layer (controllerLib)
// ---------------------------------------------------------------------------
#include "ScsiControllerLib.H"
#include "ScsiInitiatorPort.H"

// ---------------------------------------------------------------------------
// PCI SCSI base abstractions
// ---------------------------------------------------------------------------
#include "PciScsiController.H"
#include "PciScsiMmioInterface.H"
#include "PciScsiDevice.H"

// Register and MMIO abstractions
#include "PciScsiRegisterBank.H"
#include "PciScsiMailbox.H"
#include "PciScsiInterruptController.H"

// DMA abstractions
#include "PciScsiDmaEngine.H"
#include "PciScsiDmaChannel.H"

// Composite device shell and script engine
#include "PciScsiDeviceShell.H"
#include "PciScsiScriptEngine.H"
#include "PciScsiControllerTemplate.H"

// ---------------------------------------------------------------------------
// Concrete controller skeletons
// ---------------------------------------------------------------------------

// QLogic ISP1020 PCI SCSI controller skeleton
#include "ISP1020_Controller.H"

// DEC KZPBA (ISP1020-based) PCI SCSI controller wrapper
#include "KZPBA_Controller.H"

// High-level bus/controller helper
#include "VirtualScsiController.H"

#endif // PCI_SCSI_LIB_H

