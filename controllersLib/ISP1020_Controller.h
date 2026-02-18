#ifndef ISP1020_CONTROLLER_H
#define ISP1020_CONTROLLER_H
// ============================================================================
// ISP1020_Controller.H  -  QLogic ISP1020 PCI SCSI Controller Skeleton
// ============================================================================
// Purpose:
//   Skeleton PCI SCSI controller modeling the QLogic ISP1020 chip used by
//   various DEC KZPBA adapters. This class is an abstract, header-only
//   template that:
//
//     - Derives from PciScsiControllerTemplate
//     - Sets PCI vendor and device IDs for ISP1020
//     - Provides hooks for:
//         * Interrupt integration
//         * Mailbox doorbell decoding
//         * DMA control register handling
//         * Script engine instruction execution
//
//   This header does NOT depend on coreLib, AlphaCPU, SafeMemory, PAL, or
//   MMIOManager. It is intended to be used from the emulator's PCI layer.
//   DMA and guest memory movement are handled by higher layers.
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and PCI/SCSI controllerLib headers.
//   - Pure ASCII text.
// ============================================================================

#include <QtGlobal>
#include "PciScsiControllerTemplate.h"
#include "PciScsiController.h"

// ============================================================================
// ISP1020_Controller
// ============================================================================
//
// Notes:
//   - PCI vendor ID for QLogic is typically 0x1077.
//   - Device ID for ISP1020 is typically 0x1020.
//   - PCI class code for SCSI storage is 0x01 (Mass Storage, SCSI).
//
//   These values can be adjusted later to match exact variants.
// ============================================================================

class ISP1020_Controller : public PciScsiControllerTemplate
{
public:
	explicit ISP1020_Controller(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn,
		bool           threadSafe = false) noexcept
		: PciScsiControllerTemplate(bus, initiatorName, initiatorWwn, threadSafe)
	{
		// Configure PCI identity.
		PciConfigSpace& cfg = config();
		cfg.vendorId = 0x1077;   // QLogic
		cfg.deviceId = 0x1020;   // ISP1020

		cfg.classCode = 0x01;     // Mass storage
		cfg.subclass = 0x00;     // SCSI (classic)
		cfg.progIf = 0x00;     // Programming interface (device specific)
		cfg.revisionId = 0x01;     // Example revision, may be adjusted

		setName("ISP1020_Controller");
		setMmioRegionName("ISP1020_MMIO");
	}

	virtual ~ISP1020_Controller() noexcept override = default;

protected:
	// ------------------------------------------------------------------------
	// Script engine instruction execution (placeholder)
	// ------------------------------------------------------------------------
	//
	// TODO:
	//   - Implement actual ISP1020 microcode instruction decoding.
	//   - Integrate with:
	//       * mailboxQueue()
	//       * irqController()
	//       * DMA channels
	//
	virtual void executeOneInstruction(quint32 instruction) noexcept override
	{
		Q_UNUSED(instruction);
		// TODO: Decode and execute ISP1020 RISC engine instruction.
		// This will eventually manipulate SCSI transactions, DMA,
		// and interrupts according to the chip's manual.
	}

	// ------------------------------------------------------------------------
	// Mailbox doorbell decode
	// ------------------------------------------------------------------------
	//
	// TODO:
	//   - Decode 32-bit doorbell value according to ISP1020 mailbox format.
	//   - Construct PciScsiMailboxCommand instances and push into
	//     mailboxQueue().
	//   - Call processMailboxCommand(cmd) for immediate handling.
	//
	virtual void decodeMailboxDoorbell(quint32 value) noexcept override
	{
		Q_UNUSED(value);
		// Example placeholder:
		// PciScsiMailboxCommand cmd;
		// cmd.type = PciScsiMailboxCommandType::SubmitCdb;
		// mailboxQueue().enqueue(cmd);
		// processMailboxCommand(cmd);
	}

	// ------------------------------------------------------------------------
	// DMA control register
	// ------------------------------------------------------------------------
	//
	// TODO:
	//   - Map bits in DMA_CONTROL register to DMA channel operations.
	//   - For example:
	//       bit 0 = start channel 0
	//       bit 1 = stop channel 0
	//       bit 2 = reset channel 0
	//
	virtual void onDmaControlWrite(quint32 value) noexcept override
	{
		Q_UNUSED(value);
		// Example placeholder:
		// PciScsiDmaChannel* ch0 = dmaChannel(0);
		// if (!ch0) return;
		// if (value & 0x1) ch0->start();
		// if (value & 0x2) ch0->stop();
		// if (value & 0x4) ch0->reset();
	}

	// ------------------------------------------------------------------------
	// Interrupt integration
	// ------------------------------------------------------------------------
	//
	// These overrides are where your emulator specific IRQ wiring will go.
	// The base template already tracks a simple "asserted" flag. Here we add
	// explicit TODOs to wire into the global IRQ controller later.
	//
	virtual void raiseInterrupt() noexcept override
	{
		PciScsiControllerTemplate::raiseInterrupt();
		// TODO: Notify emulator IRQ subsystem that this PCI device's
		//       interrupt line (irqLine()) is asserted.
	}

	virtual void clearInterrupt() noexcept override
	{
		PciScsiControllerTemplate::clearInterrupt();
		// TODO: Notify emulator IRQ subsystem that this PCI device's
		//       interrupt line is cleared.
	}
};

#endif // ISP1020_CONTROLLER_H
