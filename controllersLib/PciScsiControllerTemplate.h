// ============================================================================
// PciScsiControllerTemplate.h - ============================================================================
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

// ============================================================================
// PciScsiControllerTemplate.H  -  Skeleton PCI SCSI Controller Template
// ============================================================================
// Purpose:
//   This header provides a ready-to-extend skeleton for PCI-attached SCSI
//   controllers. It combines the composite device shell:
//
//     � PciScsiDeviceShell      (PCI config + MMIO + SCSI adapter)
//     � PciScsiScriptEngine     (optional microcode / script engine)
//
//   and wires them together with a *very small* example register map that
//   demonstrates:
//
//     � Interrupt status / mask registers
//     � A mailbox doorbell register
//     � A DMA control register
//     � A script engine PC register
//
//   This class is intentionally abstract. Actual controllers should derive
//   from PciScsiControllerTemplate and:
//
//     � Configure PCI IDs (vendorId, deviceId, classCode, etc.)
//     � Define BAR size and index
//     � Implement executeOneInstruction()
//     � Implement raiseInterrupt() and clearInterrupt()
//     � Optionally override decodeMailboxDoorbell() and onDmaControlWrite()
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and PCI/SCSI controllerLib headers.
//   - Pure ASCII, UTF-8 (no BOM).
//   - NO dependency on AlphaCPU, SafeMemory, MMIOManager, PAL, or PTE.
//
// Conceptual references:
//   - NCR 53C8xx "SCRIPTS" engine
//   - Adaptec AIC-7xxx "Sequencer"
//   - QLogic ISP RISC controllers
//
// ============================================================================

#ifndef PCI_SCSI_CONTROLLER_TEMPLATE_H
#define PCI_SCSI_CONTROLLER_TEMPLATE_H

#include <QtGlobal>
#include <QString>

#include "PciScsiDeviceShell.H"
#include "PciScsiScriptEngine.H"
#include "PciScsiMailbox.H"
#include "PciScsiDmaChannel.H"
#include "PciScsiDmaEngine.H"
#include "PciScsiInterruptController.H"

// ============================================================================
// PciScsiControllerTemplate
// ============================================================================
//
// This class is intended as a *pattern* for concrete PCI SCSI controllers.
// It provides:
//
//   - A default BAR index and size (override if needed).
//   - A minimal register map with symbolic offsets.
//   - Wiring between:
//        * Interrupt status/mask registers
//        * Mailbox doorbell register and mailbox queue
//        * DMA control register
//        * Script engine program counter
//
// Actual controller models should:
//
//   - Derive from PciScsiControllerTemplate.
//   - Configure vendorId, deviceId, classCode in config().
//   - Possibly adjust the register offsets to match the real device.
//   - Implement executeOneInstruction() for their script engine.
//   - Implement raiseInterrupt() and clearInterrupt() to integrate with
//     the emulator IRQ subsystem.
//
// ============================================================================

class PciScsiControllerTemplate : public PciScsiDeviceShell,
	public PciScsiScriptEngine
{
public:
	// ------------------------------------------------------------------------
	// Register layout constants (example pattern)
	// ------------------------------------------------------------------------
	//
	// These offsets are purely illustrative. Derived controllers may:
	//   - Reuse them directly if they like the layout, or
	//   - Override onRegisterWrite/onRegisterRead and ignore these symbols,
	//     using their own device-specific layout instead.
	//
	enum RegisterOffset : quint64
	{
		REG_INT_STATUS = 0x00,   // Interrupt status (write to acknowledge)
		REG_INT_MASK = 0x04,   // Interrupt mask
		REG_MB_DOORBELL = 0x08,   // Mailbox doorbell (guest submits commands)
		REG_DMA_CONTROL = 0x0C,   // DMA control (start/stop/reset)
		REG_SCRIPT_PC = 0x10    // Script engine program counter (optional)
	};

	static constexpr int     DefaultBarIndex = 0;
	static constexpr quint32 DefaultBarSize = 0x1000u;   // 4 KiB example

	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------

	PciScsiControllerTemplate(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn,
		bool           threadSafe = false) noexcept
		: PciScsiDeviceShell(bus, initiatorName, initiatorWwn, threadSafe)
		, PciScsiScriptEngine(threadSafe)
		, m_mmioName("PCI-SCSI-MMIO")
		, m_defaultBarIndex(DefaultBarIndex)
		, m_defaultBarSize(DefaultBarSize)
		, m_irqAsserted(false)
	{
		// Example: configure BAR 0 with default size as memory, non-prefetch.
		configureBar(m_defaultBarIndex,
			m_defaultBarSize,
			true,   // isMemory
			false,  // is64
			false); // prefetchable

		// Register a minimal example MMIO map. Derived controllers can add
		// more registers or override offsets entirely.
		//
		// Note:
		//   - INT_STATUS is ReadWrite (write to acknowledge bits).
		//   - INT_MASK is ReadWrite.
		//   - MB_DOORBELL is WriteOnly.
		//   - DMA_CONTROL is ReadWrite.
		//   - SCRIPT_PC is ReadWrite.
		//
		addRegister(REG_INT_STATUS,
			PciScsiRegAccess::ReadWrite,
			0,                // reset
			0xFFFFFFFFu,      // readMask
			0xFFFFFFFFu,      // writeMask
			"INT_STATUS");

		addRegister(REG_INT_MASK,
			PciScsiRegAccess::ReadWrite,
			0,                // all masked by default
			0xFFFFFFFFu,
			0xFFFFFFFFu,
			"INT_MASK");

		addRegister(REG_MB_DOORBELL,
			PciScsiRegAccess::WriteOnly,
			0,
			0x00000000u,      // never read
			0xFFFFFFFFu,
			"MB_DOORBELL");

		addRegister(REG_DMA_CONTROL,
			PciScsiRegAccess::ReadWrite,
			0,
			0xFFFFFFFFu,
			0xFFFFFFFFu,
			"DMA_CONTROL");

		addRegister(REG_SCRIPT_PC,
			PciScsiRegAccess::ReadWrite,
			0,
			0xFFFFFFFFu,
			0xFFFFFFFFu,
			"SCRIPT_PC");
	}

	virtual ~PciScsiControllerTemplate() noexcept override = default;

	// ------------------------------------------------------------------------
	// MMIO region description
	// ------------------------------------------------------------------------

	virtual int associatedBarIndex() const noexcept override
	{
		return m_defaultBarIndex;
	}

	virtual quint32 barSize() const noexcept override
	{
		return m_defaultBarSize;
	}

	virtual QString mmioRegionName() const noexcept override
	{
		return m_mmioName;
	}

	inline void setMmioRegionName(const QString& name) noexcept
	{
		m_mmioName = name;
	}

	inline void setDefaultBarIndex(int index) noexcept
	{
		m_defaultBarIndex = index;
	}

	inline void setDefaultBarSize(quint32 size) noexcept
	{
		m_defaultBarSize = size;
	}

	// ------------------------------------------------------------------------
	// Interrupt signaling hooks
	// ------------------------------------------------------------------------
	//
	// Note:
	//   These overrides provide a minimal internal state flag only. Real
	//   controllers should override these again in their final class to
	//   integrate with the emulator IRQ subsystem (e.g., IRQController).
	//
	//   The flag irqAsserted() exists primarily for debugging and tests.
	//
	virtual void raiseInterrupt() noexcept override
	{
		m_irqAsserted = true;
	}

	virtual void clearInterrupt() noexcept override
	{
		m_irqAsserted = false;
	}

	inline bool irqAsserted() const noexcept
	{
		return m_irqAsserted;
	}

protected:
	// ------------------------------------------------------------------------
	// Register side-effect overrides
	// ------------------------------------------------------------------------
	//
	// PciScsiDeviceShell will call onRegisterWrite/onRegisterRead whenever
	// a register is accessed. Here we wire the example registers into:
	//
	//   - PciScsiInterruptController (INT_STATUS / INT_MASK)
	//   - Mailbox doorbell decoding
	//   - DMA control helper
	//   - Script engine PC register
	//
	// Derived controllers may override these methods but are encouraged to
	// call the base implementation so that the generic wiring still works.
	//
	// ------------------------------------------------------------------------

	virtual void onRegisterWrite(quint64 offset, quint32 value) noexcept override
	{
		switch (static_cast<RegisterOffset>(offset))
		{
		case REG_INT_STATUS:
			// Guest writes 1 bits to acknowledge corresponding status bits.
			irqController().acknowledge(value);
			updateInterruptLine();
			break;

		case REG_INT_MASK:
			irqController().setMask(value);
			updateInterruptLine();
			break;

		case REG_MB_DOORBELL:
			decodeMailboxDoorbell(value);
			break;

		case REG_DMA_CONTROL:
			onDmaControlWrite(value);
			break;

		case REG_SCRIPT_PC:
			// For controllers with script engines, allow guest to set PC.
			setScriptProgramCounter(value);
			break;

		default:
			// Non-template registers, or device-specific ones, can be
			// handled in derived classes.
			break;
		}
	}

	virtual void onRegisterRead(quint64 offset, quint32 value) noexcept override
	{
		Q_UNUSED(offset);
		Q_UNUSED(value);
		// Derived controllers may use this to implement "read to clear"
		// semantics for certain status registers, if required.
	}

	// ------------------------------------------------------------------------
	// Mailbox doorbell decode helper
	// ------------------------------------------------------------------------
	//
	// This method is invoked when the MB_DOORBELL register is written.
	// The base implementation does not assume any specific format. It
	// simply exposes a hook for derived controllers to interpret the
	// 32-bit value as a mailbox command and push commands into the
	// mailboxQueue(), then optionally process them.
	//
	// Example for derived controller:
	//
	//   void decodeMailboxDoorbell(quint32 v) noexcept override {
	//       PciScsiMailboxCommand cmd;
	//       cmd.type      = PciScsiMailboxCommandType::SubmitCdb;
	//       cmd.targetId  = static_cast<quint8>(v & 0xFF);
	//       cmd.lun       = static_cast<quint8>((v >> 8) & 0xFF);
	//       mailboxQueue().enqueue(cmd);
	//       processMailboxCommand(cmd);
	//   }
	//
	virtual void decodeMailboxDoorbell(quint32 value) noexcept
	{
		Q_UNUSED(value);
	}

	// ------------------------------------------------------------------------
	// DMA control helper
	// ------------------------------------------------------------------------
	//
	// This method is invoked when the DMA_CONTROL register is written.
	// The template makes no assumption about bit layout. Derived controllers
	// should override this and interpret bits as start, stop, reset, etc.,
	// calling into their PciScsiDmaChannel instances as appropriate.
	//
	virtual void onDmaControlWrite(quint32 value) noexcept
	{
		Q_UNUSED(value);
	}

	// ------------------------------------------------------------------------
	// Script engine PC helper
	// ------------------------------------------------------------------------
	//
	// This method is invoked when REG_SCRIPT_PC is written. For controllers
	// without script engines, derived classes can ignore it or override it.
	//
	inline void setScriptProgramCounter(quint32 pcValue) noexcept
	{
		// pcValue is in units of instruction words by default. You can
		// reinterpret it differently in a derived class if needed.
		setPc(static_cast<quint64>(pcValue));
	}

	// ------------------------------------------------------------------------
	// Script engine hook: must be implemented by derived controllers
	// ------------------------------------------------------------------------
	//
	// executeOneInstruction():
	//   Derived controllers must provide an implementation that decodes
	//   a 32-bit instruction and updates internal state, DMA, mailbox, or
	//   SCSI transactions accordingly.
	//
	virtual void executeOneInstruction(quint32 instruction) noexcept override = 0;

private:
	QString m_mmioName;
	int     m_defaultBarIndex;
	quint32 m_defaultBarSize;
	bool    m_irqAsserted;
};

#endif // PCI_SCSI_CONTROLLER_TEMPLATE_H
