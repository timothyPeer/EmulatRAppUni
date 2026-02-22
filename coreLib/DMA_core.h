// ============================================================================
// DMA_core.h - DMA support
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

#ifndef VS2022_EMULATR_EMULATRAPPUNI_CORELIB_DMA_CORE_H
#define VS2022_EMULATR_EMULATRAPPUNI_CORELIB_DMA_CORE_H

#include <QtGlobal>


struct DMACapabilities {
	// DMA support
	bool supported;               // Device can perform DMA?

	// Addressing
	quint8 addressingBits;        // DMA address width (32 or 64)
	quint64 dmaMask;              // Address mask (e.g., 0xFFFFFFFF for 32-bit)

	// Coherency
	bool coherent;                // Hardware cache coherency (snoop)?
	bool needsDoorbellFence;      // Requires explicit posted-write drain?

	// Constructor with defaults
	DMACapabilities()
		: supported(false)
		, addressingBits(32)
		, dmaMask(0xFFFFFFFF)
		, coherent(false)
		, needsDoorbellFence(false)
	{
	}
};
#endif
