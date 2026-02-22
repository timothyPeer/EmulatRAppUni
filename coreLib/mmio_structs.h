// ============================================================================
// mmio_structs.h - ============================================================================
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

#pragma once
#include <QtGlobal>
#include <QSet>
#include "mmio_core.h"

#include "DMA_core.h"

// ============================================================================
// BAR TEMPLATE (MMIO WINDOW REQUIREMENTS)
// ============================================================================

struct BarTemplate {
	// Identity
	quint8 barIndex;              // Physical BAR number (0-5 for PCI)
	QString name;                 // Optional alias ("registers", "buffers")

	// Size and alignment
	quint64 size;                 // Requested size in bytes
	quint64 minAlignment;         // Minimum alignment (must be power of 2)

	// PCI attributes
	bool is64Bit;                 // 64-bit BAR (spans 2 BAR slots)?
	bool prefetchable;            // Prefetchable memory?

	// MMIO region attributes (copied to MMIOWindow)
	quint8 allowedWidths;         // Bitmask: 0x01=byte, 0x02=word, 0x04=long, 0x08=quad
	bool stronglyOrdered;         // Serialize all accesses?
	bool sideEffectOnRead;        // Read has side-effects (FIFO pop, clear-on-read)?
	bool sideEffectOnWrite;       // Write has side-effects (doorbell, FIFO push)?
	mmio_Endianness regEndian;         // Register endianness (BIG or LITTLE)

	// Constructor with defaults
	BarTemplate()
		: barIndex(0)
		, size(0)
		, minAlignment(4096)
		, is64Bit(false)
		, prefetchable(false)
		, allowedWidths(0x0F)  // All widths by default
		, stronglyOrdered(false)
		, sideEffectOnRead(false)
		, sideEffectOnWrite(false)
		, regEndian(mmio_Endianness::LITTLE)
	{
	}
};



struct BARDescriptor {
	quint8 barIndex;
	quint64 size;

	quint64 minAlignment;
	quint64 maxAddress = ~0ULL;

	bool is64Bit;
	bool ioSpace = false;
	bool prefetchable = false;

	// Access attributes (template defaults)
	quint32 allowedWidths = 0x0F;
	bool sideEffectRead = false;
	bool sideEffectWrite = true;
	bool stronglyOrdered = false;
};

