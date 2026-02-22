// ============================================================================
// PciScsiRegisterBank.h - ============================================================================
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
// PciScsiRegisterBank.H  -  Generic Register File for PCI SCSI Controllers
// ============================================================================
// Purpose:
//   PCI SCSI devices require a structured set of registers exposed through
//   MMIO. Some are read-only, some read/write, some require masking, and
//   some are write-only (doorbells).
//
//   PciScsiRegisterBank provides:
//     � A clean abstraction for device registers
//     � A small registration API for adding registers
//     � Safe read/write helpers with masking & side effects
//     � No device-specific logic - derived controllers implement callbacks
//
// Design constraints:
//   - Header-only, no .CPP
//   - Depends only on QtCore (QtGlobal, QMutex)
//   - Pure ASCII, UTF-8 (no BOM)
//   - NO dependency on AlphaCPU, SafeMemory, PAL, PTE, MMIOManager
//
// Integration:
//   A PCI SCSI device will embed a PciScsiRegisterBank and use it inside
//   its mmioReadXX/mmioWriteXX implementations to store or fetch register
//   contents.
//
// ============================================================================

#ifndef PCI_SCSI_REGISTER_BANK_H
#define PCI_SCSI_REGISTER_BANK_H

#include <QtGlobal>
#include <QMutex>
#include <QVector>
#include <QString>

// ============================================================================
// PciScsiRegAccess
// ============================================================================
enum class PciScsiRegAccess : quint8
{
	ReadWrite = 0,
	ReadOnly,
	WriteOnly
};

// ============================================================================
// PciScsiRegister
// ============================================================================
struct PciScsiRegister
{
	quint64          offset;        // offset within BAR region
	quint32          value;         // 32-bit register value
	quint32          readMask;      // mask applied to read
	quint32          writeMask;     // mask applied to write
	PciScsiRegAccess access;        // R/W or RO/WO behavior

	// Optional human-friendly name
	QString          name;

	PciScsiRegister() noexcept
		: offset(0)
		, value(0)
		, readMask(0xFFFFFFFFu)
		, writeMask(0xFFFFFFFFu)
		, access(PciScsiRegAccess::ReadWrite)
		, name()
	{
	}
};

// ============================================================================
// PciScsiRegisterBank
// ============================================================================
//
// A PCI device typically keeps a register map array such as:
//
//      offset +0x00 -> STATUS
//      offset +0x04 -> MASK
//      offset +0x08 -> DOORBELL
//      offset +0x0C -> DMA_PTR
//
// Derived PCI SCSI controllers register each mmio-visible register
// using addRegister(). They can then override onWrite() / onRead()
// for custom side effects.
//
// ============================================================================
class PciScsiRegisterBank
{
public:
	explicit PciScsiRegisterBank(bool threadSafe = false) noexcept
		: m_threadSafe(threadSafe)
	{
	}

	~PciScsiRegisterBank() noexcept = default;

	// ------------------------------------------------------------------------
	// Register creation
	// ------------------------------------------------------------------------

	inline void addRegister(quint64          offset,
		PciScsiRegAccess access,
		quint32          resetValue,
		quint32          readMask,
		quint32          writeMask,
		const QString& name = QString()) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		PciScsiRegister r;
		r.offset = offset;
		r.value = resetValue;
		r.access = access;
		r.readMask = readMask;
		r.writeMask = writeMask;
		r.name = name;

		m_regs.append(r);
	}

	// ------------------------------------------------------------------------
	// Register lookup
	// ------------------------------------------------------------------------

	inline PciScsiRegister* find(quint64 offset) noexcept
	{
		for (int i = 0; i < m_regs.size(); ++i)
		{
			if (m_regs[i].offset == offset)
				return &m_regs[i];
		}
		return nullptr;
	}

	inline const PciScsiRegister* find(quint64 offset) const noexcept
	{
		for (int i = 0; i < m_regs.size(); ++i)
		{
			if (m_regs[i].offset == offset)
				return &m_regs[i];
		}
		return nullptr;
	}

	// ------------------------------------------------------------------------
	// Read/Write handling
	// ------------------------------------------------------------------------

	inline quint32 read32(quint64 offset) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		PciScsiRegister* r = find(offset);
		if (!r)
			return 0;

		if (r->access == PciScsiRegAccess::WriteOnly)
			return 0;

		quint32 masked = (r->value & r->readMask);
		onRead(offset, masked);
		return masked;
	}

	inline void write32(quint64 offset, quint32 val) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		PciScsiRegister* r = find(offset);
		if (!r)
			return;

		if (r->access == PciScsiRegAccess::ReadOnly)
			return;

		quint32 masked = (val & r->writeMask);
		r->value = masked;

		onWrite(offset, masked);
	}

	// ------------------------------------------------------------------------
	// Utility: reset all registers to 0.
	// ------------------------------------------------------------------------
	inline void resetAll() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		for (int i = 0; i < m_regs.size(); ++i)
		{
			m_regs[i].value = 0;
		}
	}

protected:
	// ------------------------------------------------------------------------
	// Side-effect hooks (for derived PCI controllers)
	// ------------------------------------------------------------------------
	//
	// You override these in your specific device classes to respond to
	// register changes (doorbells, queue resets, DMA kicks, etc.).
	// ------------------------------------------------------------------------

	virtual void onWrite(quint64 offset, quint32 value) noexcept
	{
		Q_UNUSED(offset);
		Q_UNUSED(value);
	}

	virtual void onRead(quint64 offset, quint32 value) noexcept
	{
		Q_UNUSED(offset);
		Q_UNUSED(value);
	}

private:
	QVector<PciScsiRegister> m_regs;
	bool                     m_threadSafe;
	mutable QMutex           m_mutex;
};

#endif // PCI_SCSI_REGISTER_BANK_H
