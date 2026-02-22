// ============================================================================
// mmio_PlatformTemplates.h - Platform-specific defaults (loaded from hwModel="ES40")
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
#include <QString>
#include <QVector>
#include <QtGlobal>
#include "../CoreLib/mmio_core.h"



/*
MMIOBase Calculation:
The leading 0X0000 is padding; the apostrophes are just digit groupings for readability.

	Positional Hex: 0x0000'A000'0000 = A x 16^7 = 10 x 0x10000000 = 10 x 268,435,456 = 2,684,354,560
	Binary Magnitude: 0xA0000000 = 0b1010 followed by 28 zero bits -> 2^28 x 10 = 268,435,456 x 10
	Human Units: 2,684,354,560 / 2^30 = 2.5 GiB

	mmio32 < 4 GiB -> for 32-bit BARs,
	mmio64 > 4 GiB -> for 64-bit BARs / future big devices

	Analogous Functionality:
	PlatformTemplate serves as the board/backplane map (I/O apertures + IRQ number pools),
	SMP, caches, and coherency are layered on above it.
*/


// Platform-specific defaults (loaded from hwModel="ES40")
struct PlatformTemplate {
	QString modelName;
	quint16 numHoses = 0;
	QVector<Hose> hoses;
};

// Example: ES40 platform
PlatformTemplate loadES40Template() {
	PlatformTemplate tpl;
	tpl.modelName = "ES40";
	tpl.numHoses = 2;
	tpl.hoses.reserve(tpl.numHoses);

	// two hoses, each with distinct 32-bit MMIO, 64-bit MMIO, and an IRQ sub-range
	// Provides deterministic spaces to the ResourceAllocated to carve 
	// BARs and assign device vectors without colliding with exceptions/software-int/AST vector domains.
	// 
	// IRQ vector pool sizes: 
	//  - TODO calculate the per-hose IRQ pools, for a 4-CPU config (typical in larger systems)
	//         enlarge per-hose IRQ pools (e.g., Hose0 0x400..0x460, Hose1 0x460..0x4A0) so the allocator 
	//         never exhausts routes.
	//
	//   -     Routing policy(outside the template) : 
	//         map hose-> CPU affinity sets(e.g., Hose0 -> CPUs{ 0,1 }, Hose1 -> CPUs{ 2,3 }) in IRQController.
	//         This improves locality while keeping the same vector numbers.
	//    TODO Add a Configuration option to configure hose-n-CPU Affinity.
	// 
	// 
	// 
	// Hose 0
	{

		Hose h(0);
		h.hoseId = 0;
		// mmio32: 0xB000_0000 .. 0xC000_0000 (256 MiB, < 4 GiB BARS)
		h.mmio32.base = 0x0000'B000'0000ull;
		h.mmio32.size = 0x0000'C000'0000ull;  // 256 MiB window (< 4 GiB BARS)

		// mmio64: 0x0000_8000_0000 .. 0x0000_A000_0000 (512 MiB)
		h.mmio64.base = 0x0000'8000'0000ull;  // 512 GiB > 4 GiB BARS
		h.mmio64.size = 0x0000'A000'0000ull;  // +512 MiB > 4 GiB BARS

		// IRQ vectors: choose a domain that does not overlap reserved ranges
		h.irqDomain.base = 0x400;   // example pool start
		h.irqDomain.limit = 0x420;   // exclusive (48..63)

		// Initialize cursors to bases
		h.mmio32.cursor = h.mmio32.base;
		h.mmio64.cursor = h.mmio64.base;
		h.irqDomain.cursor = h.irqDomain.base;

		tpl.hoses.push_back(h);
	}
	// Hose 1
	{

		Hose h(1);
		h.hoseId = 1;

		// mmio32: 0xC000_0000 .. 0xD000_0000 (256 MiB, < 4 GiB)
		h.mmio32.base = 0x0000'C000'0000ull;
		h.mmio32.size = 0x0000'D000'0000ull;

		// mmio64: 0x0000_A000_0000 .. 0x0000_C000_0000 (512 MiB)
		h.mmio64.base = 0x0000'A000'0000ull;
		h.mmio64.size = 0x0000'C000'0000ull;

		// IRQ vectors: second pool
		h.irqDomain.base = 64;
		h.irqDomain.limit = 80;   // exclusive (64..79)

		// Initialize cursors to bases
		h.mmio32.cursor = h.mmio32.base;
		h.mmio64.cursor = h.mmio64.base;
		h.irqDomain.cursor = h.irqDomain.base;

		tpl.hoses.push_back(h);
	}
	return tpl;
}



PlatformTemplate loadES40_4CPU_Template() {
	auto tpl = loadES40Template(); // start from your ES40
	// Enlarge per-hose IRQ pools to accommodate more devices/queues
	tpl.hoses[0].irqDomain.base = 0x400;
	tpl.hoses[0].irqDomain.limit = 0x460; // +96
	tpl.hoses[1].irqDomain.base = 0x460;
	tpl.hoses[1].irqDomain.limit = 0x4A0; // +64
	// (Optional) widen MMIO windows if you expect more/larger BARs
	return tpl;
}

PlatformTemplate loadGXTemplate() {
	PlatformTemplate tpl;
	tpl.modelName = "GX-Series";
	tpl.numHoses = 4;
	tpl.hoses.reserve(tpl.numHoses);
	for (int h = 0; h < 4; ++h) {
		Hose a(h);
		a.hoseId = h;
		// Make bigger apertures to host many 64-bit BAR devices
		a.mmio32.base = 0x0000'B000'0000ull + (quint64)h * 0x0100'0000ull; // per-hose 256 MiB
		a.mmio32.size = a.mmio32.base + 0x0100'0000ull;
		a.mmio64.base = 0x0001'0000'0000ull + (quint64)h * 0x0400'0000ull; // 1 GiB per hose
		a.mmio64.size = a.mmio64.base + 0x0400'0000ull;

		// Non-overlapping 128-vector slabs
		a.irqDomain.base = 0x500 + 128 * h;
		a.irqDomain.limit = a.irqDomain.base + 128;

		a.mmio32.cursor = a.mmio32.base;
		a.mmio64.cursor = a.mmio64.base;
		a.irqDomain.cursor = a.irqDomain.base;
		tpl.hoses.push_back(a);
	}
	return tpl;
}

PlatformTemplate loadDS20Template() {
	PlatformTemplate tpl;
	tpl.modelName = "DS20";
	tpl.numHoses = 2;
	tpl.hoses.reserve(2);

	// Hose 0
	{
		Hose a(0);
		a.hoseId = 0;
		a.mmio32.base = 0x0000'B800'0000ull; a.mmio32.size = a.mmio32.base + 0x0200'0000ull; // 32?64-bit friendly
		a.mmio64.base = 0x0000'9000'0000ull; a.mmio64.size = a.mmio64.base + 0x0200'0000ull; // 512 MiB each
		a.irqDomain.base = 0x300; a.irqDomain.limit = 0x340; // 64 vectors
		a.mmio32.cursor = a.mmio32.base; a.mmio64.cursor = a.mmio64.base; a.irqDomain.cursor = a.irqDomain.base;
		tpl.hoses.push_back(a);
	}
	// Hose 1
	{
		Hose  a(1);
		a.hoseId = 1;
		a.mmio32.base = 0x0000'BA00'0000ull; a.mmio32.size = a.mmio32.base + 0x0200'0000ull;
		a.mmio64.base = 0x0000'9200'0000ull; a.mmio64.size = a.mmio64.base + 0x0200'0000ull;
		a.irqDomain.base = 0x340; a.irqDomain.limit = 0x370; // 48 vectors
		a.mmio32.cursor = a.mmio32.base; a.mmio64.cursor = a.mmio64.base; a.irqDomain.cursor = a.irqDomain.base;
		tpl.hoses.push_back(a);
	}
	return tpl;
}

PlatformTemplate loadDS10Template() {
	PlatformTemplate tpl;
	tpl.modelName = "DS10";
	tpl.numHoses = 1;
	tpl.hoses.reserve(1);

	Hose  a(0);
	a.hoseId = 0;
	a.mmio32.base = 0x0000'BC00'0000ull; a.mmio32.size = a.mmio32.base + 0x0100'0000ull; // 256 MiB
	a.mmio64.base = 0x0000'9400'0000ull; a.mmio64.size = a.mmio64.base + 0x0200'0000ull; // 512 MiB (optional)
	a.irqDomain.base = 0x280; a.irqDomain.limit = 0x2A0; // 32 vectors
	a.mmio32.cursor = a.mmio32.base; a.mmio64.cursor = a.mmio64.base; a.irqDomain.cursor = a.irqDomain.base;
	tpl.hoses.push_back(a);

	return tpl;
}

