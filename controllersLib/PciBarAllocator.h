// ============================================================================
// PciBarAllocator.h - ============================================================================
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
// PciBarAllocator.H  -  Simple PCI BAR Address Allocator
// ============================================================================
// Purpose:
//   Provides a small, header-only helper for allocating *non-overlapping*
//   PCI BAR address ranges within a configured MMIO window.
//
//   This is the first step in integrating PCI devices (like your KZPBA / ISP1020
//   controllers) into the global MMIO framework:
//
//     1) SettingsLoader + EmulatorSettings
//     2) PciScsiConfigBinder creates PciScsiDeviceShell instances
//     3) PciBarAllocator assigns BAR base addresses inside the host MMIO space
//     4) PciDeviceManager / MMIOManager map those BAR ranges to device
//        mmioReadXX/mmioWriteXX handlers
//
//   This allocator *does not* talk to SafeMemory or MMIOManager directly.
//   It only hands out aligned physical base addresses.
//
// Reference:
//   � PCI Local Bus Specification, Rev. 2.x, BAR and address space section
//     (Memory/IO decode and BASE_ADDRESS registers).
// ============================================================================

#ifndef PCI_BAR_ALLOCATOR_H
#define PCI_BAR_ALLOCATOR_H

#include <QtGlobal>

// ============================================================================
// PciBarInfo
// ============================================================================
//
// Describes a single allocated BAR window. This struct is returned by the
// allocator and can be stored inside a PCI device object or PciDeviceManager.
//
// BAR characteristics like isMemory/is64/prefetchable are typically taken
// from the device model (e.g., PciScsiControllerTemplate::configureBar),
// while base/size are assigned by this allocator.
//
// ============================================================================
struct PciBarInfo
{
	quint64 base;          // Physical base address assigned to the BAR
	quint32 size;          // Size in bytes (must be power-of-two & aligned)
	bool    isMemory;      // true = memory space, false = I/O space
	bool    is64Bit;       // true if BAR is 64-bit
	bool    prefetchable;  // true if prefetchable (PCI memory attribute)

	PciBarInfo() noexcept
		: base(0)
		, size(0)
		, isMemory(true)
		, is64Bit(false)
		, prefetchable(false)
	{
	}
};

// ============================================================================
// PciBarAllocator
// ============================================================================
//
// Simple linear allocator for PCI MMIO/I/O windows. You configure:
//
//   - a starting physical address (windowBase)
//   - an exclusive limit (windowLimit)
//   - a minimum alignment (alignmentBytes, typically 4KiB for PCI)
//
// and then call allocate() for each BAR in turn. The allocator:
//
//   - Aligns the current pointer up to the requested alignment
//   - Checks for overflow beyond windowLimit
//   - Returns a PciBarInfo with base + size
//
// Integration:
//
//   � Typically constructed during emulator startup:
//
//       PciBarAllocator mmioAlloc(0x80000000ULL, 0x90000000ULL, 0x1000);
//
//   � For each PCI device:
//
//       PciBarInfo bar0 = mmioAlloc.allocate(deviceBar0Size);
//       device->setBarBase(0, bar0.base);
//
//   � MMIOManager will later map [bar0.base, bar0.base + bar0.size) to the
//     device's PciScsiMmioInterface implementation.
//
// Design notes:
//   - No threading, used during single-threaded init.
//   - No dependency on SafeMemory, MMIOManager, AlphaCPU.
// ============================================================================
class PciBarAllocator
{
public:
	// ------------------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   windowBase    - starting physical address for this allocation window
	//   windowLimit   - exclusive end address (allocation must stay below this)
	//   alignmentBytes- minimum alignment for allocations (e.g., 0x1000)
	//
	PciBarAllocator(quint64 windowBase,
		quint64 windowLimit,
		quint64 alignmentBytes = 0x1000ULL) noexcept
		: m_current(windowBase)
		, m_limit(windowLimit)
		, m_alignment(alignmentBytes ? alignmentBytes : 0x1000ULL)
	{
	}

	// ------------------------------------------------------------------------
	// allocate()
	// ------------------------------------------------------------------------
	//
	// Allocates a BAR region of the given size with at least the allocator's
	// alignment. Returns a PciBarInfo describing the range.
	//
	// Parameters:
	//   size         - requested BAR size in bytes (must be > 0)
	//   isMemory     - true for memory space, false for I/O space
	//   is64Bit      - true if 64-bit BAR, false for 32-bit
	//   prefetchable - PCI prefetchable attribute
	//
	// On failure (window exhausted or size == 0), returned PciBarInfo.size == 0.
	//
	PciBarInfo allocate(quint32 size,
		bool    isMemory = true,
		bool    is64Bit = false,
		bool    prefetchable = false) noexcept
	{
		PciBarInfo info;

		if (size == 0)
		{
			return info; // invalid size, return zeroed info
		}

		// Align current pointer to alignment boundary.
		const quint64 alignedBase = alignUp(m_current, m_alignment);

		// Check for overflow.
		const quint64 end = alignedBase + static_cast<quint64>(size);
		if (end > m_limit || end <= alignedBase)
		{
			// No space left in this window; return a zero-sized descriptor.
			return info;
		}

		// Commit allocation.
		info.base = alignedBase;
		info.size = size;
		info.isMemory = isMemory;
		info.is64Bit = is64Bit;
		info.prefetchable = prefetchable;

		m_current = end;

		return info;
	}

	// ------------------------------------------------------------------------
	// reset()
	// ------------------------------------------------------------------------
	//
	// Resets allocator to a new base/limit/alignment. Useful if you want
	// to rebuild PCI layout after a configuration change.
	//
	inline void reset(quint64 windowBase,
		quint64 windowLimit,
		quint64 alignmentBytes = 0x1000ULL) noexcept
	{
		m_current = windowBase;
		m_limit = windowLimit;
		m_alignment = alignmentBytes ? alignmentBytes : 0x1000ULL;
	}

	// Current pointer and limit accessors.
	inline quint64 current() const noexcept { return m_current; }
	inline quint64 limit()   const noexcept { return m_limit; }
	inline quint64 alignment() const noexcept { return m_alignment; }

private:
	quint64 m_current;   // next free address
	quint64 m_limit;     // exclusive limit
	quint64 m_alignment; // minimum alignment

	// Aligns value up to the next multiple of alignment.
	static inline quint64 alignUp(quint64 value, quint64 alignment) noexcept
	{
		const quint64 mask = alignment - 1;
		return (value + mask) & ~mask;
	}
};

#endif // PCI_BAR_ALLOCATOR_H
