// ============================================================================
// PciScsiDeviceShell.h - ============================================================================
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
// PciScsiDeviceShell.H  -  Composite PCI SCSI Controller Skeleton
// ============================================================================
// Purpose:
//   This header provides a cohesive "shell" for PCI-attached SCSI controllers.
//   It combines:
//
//     � PciScsiDevice            (PCI SCSI base: config + SCSI host adapter)
//     � PciScsiRegisterBank      (MMIO register file abstraction)
//     � PciScsiMailboxQueue      (decoded mailbox / doorbell commands)
//     � PciScsiInterruptController (IRQ status and mask handling)
//     � PciScsiDmaChannel        (one or more DMA channels)
//
//   The goal is to give you a ready-made skeleton that real controller
//   implementations (QLogic, NCR 53C8xx, KZPBA, etc.) can derive from,
//   while keeping this layer independent of coreLib, AlphaCPU, SafeMemory,
//   and MMIOManager.
//
// Design constraints:
//   - Header-only, no .CPP file.
//   - Depends only on QtCore and the PCI + SCSI controllerLib headers.
//   - Pure ASCII, UTF-8 (no BOM).
//   - NO dependency on AlphaCPU, PALcode, SafeMemory, or MMIOManager.
//
// Responsibilities of PciScsiDeviceShell:
//   - Provide a default MMIO implementation using PciScsiRegisterBank
//     for 8/16/32/64-bit accesses (little-endian).
//   - Expose mailbox, IRQ, and DMA channel objects for derived devices.
//   - Offer overridable hooks for register read/write side effects and
//     interrupt line updates.
//
// Reference documents (conceptual design):
//   - PCI Local Bus Specification, Rev 2.x
//   - SCSI Architecture Model (SAM-2)
//   - Vendor-specific PCI SCSI controller datasheets (QLogic, NCR, etc.).
//
// ============================================================================

#ifndef PCI_SCSI_DEVICE_SHELL_H
#define PCI_SCSI_DEVICE_SHELL_H

#include <QtGlobal>
#include <QString>
#include <QMutex>
#include <QVector>

#include "ScsiControllerLib.H"
#include "PciScsiDevice.H"
#include "PciScsiRegisterBank.H"
#include "PciScsiMailbox.H"
#include "PciScsiInterruptController.H"
#include "PciScsiDmaChannel.H"
#include "PciScsiDmaEngine.H"

// ============================================================================
// PciScsiDeviceShell
// ============================================================================
//
// This class is intended as the common base for concrete PCI SCSI devices.
// It provides:
//
//   - MMIO register storage via PciScsiRegisterBank
//   - Generic MMIO read/write handlers (8/16/32/64) layered on 32-bit registers
//   - A mailbox queue for decoded commands
//   - An interrupt controller helper
//   - A list of DMA channels (each bound to a PciScsiDmaEngine)
//
// Concrete controller responsibilities:
//
//   - Configure PCI identity (vendorId, deviceId, classCode, BAR layout).
//   - Call addRegister(...) during construction to define the MMIO register map.
//   - Create and connect PciScsiDmaEngine instances, then call addDmaChannel().
//   - Implement:
//        associatedBarIndex()
//        barSize()
//        mmioRegionName()
//        raiseInterrupt()
//        clearInterrupt()
//   - Optionally override:
//        onRegisterWrite(offset, value)
//        onRegisterRead(offset, value)
//        processMailboxCommand(const PciScsiMailboxCommand &cmd)
//
// ============================================================================

class PciScsiDeviceShell : public PciScsiDevice,
	protected PciScsiRegisterBank
{
public:
	// ------------------------------------------------------------------------
	// Constructor / Destructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   bus           - ScsiBus to attach devices to.
	//   initiatorName - logical initiator name (for SCSI initiator port).
	//   initiatorWwn  - 64-bit WWN-style identity for the adapter.
	//   threadSafe    - if true, top-level shell uses an internal mutex.
	//
	explicit PciScsiDeviceShell(ScsiBus* bus,
		const QString& initiatorName,
		quint64        initiatorWwn,
		bool           threadSafe = false) noexcept
		: PciScsiDevice(bus, initiatorName, initiatorWwn)
		, PciScsiRegisterBank(threadSafe)
		, m_irqController(threadSafe)
		, m_threadSafe(threadSafe)
	{
	}

	virtual ~PciScsiDeviceShell() noexcept override = default;

	// ------------------------------------------------------------------------
	// Mailbox accessors
	// ------------------------------------------------------------------------

	inline PciScsiMailboxQueue& mailboxQueue() noexcept
	{
		return m_mailbox;
	}

	inline const PciScsiMailboxQueue& mailboxQueue() const noexcept
	{
		return m_mailbox;
	}

	// ------------------------------------------------------------------------
	// Interrupt controller accessors
	// ------------------------------------------------------------------------

	inline PciScsiInterruptController& irqController() noexcept
	{
		return m_irqController;
	}

	inline const PciScsiInterruptController& irqController() const noexcept
	{
		return m_irqController;
	}

	// Call this after modifying the interrupt controller state or mask.
	// It will raise or clear the device interrupt line based on
	// irqController().hasPending() and interruptsEnabled().
	inline void updateInterruptLine() noexcept
	{
		if (irqController().hasPending() && interruptsEnabled())
		{
			raiseInterrupt();
		}
		else
		{
			clearInterrupt();
		}
	}

	// ------------------------------------------------------------------------
	// DMA channel management
	// ------------------------------------------------------------------------

	// Add a channel bound to the given DMA engine.
	// The engine is non-owning; it must outlive the channel.
	inline void addDmaChannel(quint8            channelId,
		const QString& name,
		PciScsiDmaEngine* engine) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_shellMutex : nullptr);
		m_dmaChannels.append(PciScsiDmaChannel(channelId, name, engine, m_threadSafe));
	}

	// Retrieve a channel by index; returns nullptr if out of range.
	inline PciScsiDmaChannel* dmaChannel(int index) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_shellMutex : nullptr);
		if (index < 0 || index >= m_dmaChannels.size())
		{
			return nullptr;
		}
		return &m_dmaChannels[index];
	}

	inline const PciScsiDmaChannel* dmaChannel(int index) const noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_shellMutex : nullptr);
		if (index < 0 || index >= m_dmaChannels.size())
		{
			return nullptr;
		}
		return &m_dmaChannels[index];
	}

	inline int dmaChannelCount() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_shellMutex : nullptr);
		return m_dmaChannels.size();
	}

	// ------------------------------------------------------------------------
	// MMIO implementation (from PciScsiMmioInterface via PciScsiDevice)
	// ------------------------------------------------------------------------
	//
	// Default mapping:
	//   - All MMIO accesses are modeled as little-endian operations into
	//     32-bit registers defined by addRegister() at dword-aligned offsets.
	//   - Reads and writes use PciScsiRegisterBank::read32/write32().
	//   - Devices may override any of these if they require special behavior.
	//
	// NOTE:
	//   associatedBarIndex(), barSize(), and mmioRegionName() remain pure
	//   virtual and must be provided by the concrete controller.
	//
	// ------------------------------------------------------------------------

	// 8-bit read via 32-bit register
	virtual quint8 mmioRead8(quint64 offset) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 val32 = read32(aligned);
		const int     shift = static_cast<int>((offset & 0x3u) * 8u);
		return static_cast<quint8>((val32 >> shift) & 0xFFu);
	}

	// 16-bit read via 32-bit register
	virtual quint16 mmioRead16(quint64 offset) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 val32 = read32(aligned);

		const int byteOffset = static_cast<int>(offset & 0x3u);
		// Align to 16-bit boundary inside the 32-bit word.
		const int shift = (byteOffset & ~0x1) * 8;
		return static_cast<quint16>((val32 >> shift) & 0xFFFFu);
	}

	// 32-bit read via register bank
	virtual quint32 mmioRead32(quint64 offset) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		return read32(aligned);
	}

	// 64-bit read as two consecutive 32-bit reads (little-endian).
	virtual quint64 mmioRead64(quint64 offset) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 lo = read32(aligned);
		const quint32 hi = read32(aligned + 4);
		return (static_cast<quint64>(hi) << 32) | static_cast<quint64>(lo);
	}

	// 8-bit write via 32-bit register
	virtual void mmioWrite8(quint64 offset, quint8 value) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 old32 = read32(aligned);

		const int shift = static_cast<int>((offset & 0x3u) * 8u);
		const quint32 mask = ~(static_cast<quint32>(0xFFu) << shift);
		const quint32 new32 = (old32 & mask)
			| (static_cast<quint32>(value) << shift);

		write32(aligned, new32);
	}

	// 16-bit write via 32-bit register
	virtual void mmioWrite16(quint64 offset, quint16 value) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 old32 = read32(aligned);

		const int byteOffset = static_cast<int>(offset & 0x3u);
		const int shift = (byteOffset & ~0x1) * 8;
		const quint32 mask = ~(static_cast<quint32>(0xFFFFu) << shift);
		const quint32 new32 = (old32 & mask)
			| (static_cast<quint32>(value) << shift);

		write32(aligned, new32);
	}

	// 32-bit write via register bank
	virtual void mmioWrite32(quint64 offset, quint32 value) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		write32(aligned, value);
	}

	// 64-bit write as two consecutive 32-bit writes (little-endian).
	virtual void mmioWrite64(quint64 offset, quint64 value) noexcept override
	{
		const quint64 aligned = offset & ~static_cast<quint64>(0x3u);
		const quint32 lo = static_cast<quint32>(value & 0xFFFFFFFFu);
		const quint32 hi = static_cast<quint32>((value >> 32) & 0xFFFFFFFFu);

		write32(aligned, lo);
		write32(aligned + 4, hi);
	}

protected:
	// ------------------------------------------------------------------------
	// Register side-effect hooks
	// ------------------------------------------------------------------------
	//
	// PciScsiRegisterBank::onWrite/onRead are overridden to provide a
	// controller-level hook for register side effects, such as:
	//
	//   - Doorbell writes that enqueue mailbox commands
	//   - DMA control register writes that start or stop channels
	//   - Status register reads that implicitly clear bits
	//
	// Derived controllers should override onRegisterWrite/onRegisterRead
	// rather than onWrite/onRead directly.
	//
	// ------------------------------------------------------------------------

	virtual void onRegisterWrite(quint64 offset, quint32 value) noexcept
	{
		Q_UNUSED(offset);
		Q_UNUSED(value);
	}

	virtual void onRegisterRead(quint64 offset, quint32 value) noexcept
	{
		Q_UNUSED(offset);
		Q_UNUSED(value);
	}

	// Optional helper: process a decoded mailbox command.
	// Derived controllers can override this to interpret high-level mailbox
	// operations (SubmitCdb, ResetBus, etc.).
	virtual void processMailboxCommand(const PciScsiMailboxCommand& cmd) noexcept
	{
		Q_UNUSED(cmd);
	}

	// ------------------------------------------------------------------------
	// PciScsiRegisterBank overrides
	// ------------------------------------------------------------------------

	virtual void onWrite(quint64 offset, quint32 value) noexcept override
	{
		// First, give derived controller a chance to react.
		onRegisterWrite(offset, value);

		// If register write encodes a mailbox operation, derived controller
		// should decode it and push PciScsiMailboxCommand instances into
		// m_mailbox, then optionally call processMailboxCommand directly.
		//
		// This default implementation does not assume any particular layout.
	}

	virtual void onRead(quint64 offset, quint32 value) noexcept override
	{
		Q_UNUSED(value);
		onRegisterRead(offset, value);
	}

private:
	PciScsiMailboxQueue           m_mailbox;
	PciScsiInterruptController    m_irqController;
	QVector<PciScsiDmaChannel>    m_dmaChannels;

	bool                          m_threadSafe;
	mutable QMutex                m_shellMutex;
};

#endif // PCI_SCSI_DEVICE_SHELL_H
