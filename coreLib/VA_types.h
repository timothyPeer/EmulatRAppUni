// ============================================================================
// VA_types.h - identify the BANK a VA is associated to.
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

#ifndef VA_TYPES_H
#define VA_TYPES_H
#include <QtGlobal>

enum class AddressClass : quint8
{
    Unknown = 0,
    User,           // User mode (VA bits indicate user space)
    Kernel,         // Kernel/system space
    Superpage,      // Superpage region (VPTB-based)
    PALcode,        // PALcode region (implementation-defined)
    IO              // MMIO / system I/O space
};

// identify the BANK a VA is associated to.

enum class TLB_BANK : quint8 { BANK0, BANK1 };

enum class PerfEvent : quint8 {
	USER_VA_ACCESSES,
	KERNEL_VA_ACCESSES,
	INVALID_VA_ACCESSES,
	TLB_BANK1_ACCESSES,
	TLB_BANK0_ACCESSES,
	DTB_BANK0_ACCESSES,
	DTB_BANK1_ACCESSES
};

// ============================================================================
// 1. TranslationResult - For ALL translation functions
// ============================================================================
enum class TranslationResult {
	Success,              // PA valid, proceed
	NonCanonical,         // VA not canonical
	TlbMiss,              // TLB lookup failed (fast path only)
	DlbMiss,			  // Missed Lookup of DTB
	IlbMiss,
	PageNotPresent,       // PTE invalid (page walk)
	FaultOnRead,          // PTE.FOR set
	FaultOnWrite,         // PTE.FOW set
	FaultOnExecute,       // PTE.FOE set
	Unaligned,            // Alignment check failed
	BusError ,             // Memory read failed during walk
	INVALID_PTE,
	PteInvalid,
	NotKseg,				// VA is not in kseg segment - continue to page walk
	AccessViolation,		// Permission denied (kseg from user mode, or PTE ACV)
};

// ============================================================================
// 3. BoxResult - For INSTRUCTION execution side effects ONLY
// ============================================================================
// Keep as-is, but use ONLY for instruction execution results
// Translation faults are handled by caller converting to BoxResult

#endif // VA_TYPES_H
