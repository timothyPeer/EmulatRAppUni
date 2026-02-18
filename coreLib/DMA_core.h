#pragma once
#include <QtGlobal>
#include "corelib_global.h"

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