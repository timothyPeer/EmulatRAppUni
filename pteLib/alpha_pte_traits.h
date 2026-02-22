// ============================================================================
// alpha_pte_traits.h - ============================================================================
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
#include "alpha_pte_core.h"
#include <QtGlobal>
#include "AlphaPTE_Core.h"

// ============================================================================
// EV6 (21164/21264) Page Table & PTE Constants (NON-TEMPLATED)
// ============================================================================
//
// This file provides the *canonical* architectural constants and helpers
// for EV6 PTE decode/encode. No templates, no CPUFamily branching.
//
// This is the "Layer 0 silicon view" of the PTE format.
//
// ============================================================================

// ---------------------------------------------------------------------
// Architectural PTE Bits (EV6)
// ---------------------------------------------------------------------
// <0> Valid bit
static constexpr quint64 VALID_MASK = 0x0000000000000001ULL;

// <3:1> Fault bits: FOR/FOW/FOE
static constexpr quint64 FAULT_MASK = 0x000000000000000EULL;
static constexpr unsigned FAULT_SHIFT = 1;

// <4> ASM (global) bit
static constexpr quint64 ASM_MASK = 0x0000000000000010ULL;
static constexpr unsigned ASM_SHIFT = 4;

// <6:5> GH = TB block hint
static constexpr quint64 GH_MASK = 0x0000000000000060ULL;  // bits 6:5 only
static constexpr unsigned GH_SHIFT = 5;

// <7> Reserved
static constexpr quint64 RSVD_MASK = 0x0000000000000080ULL;

// <11:8>  Read enables: KRE/ERE/SRE/URE
static constexpr quint64 READ_EN_MASK = 0x0000000000000F00ULL;

static constexpr int KRE_SHIFT = 8;
static constexpr int ERE_SHIFT = 9;
static constexpr int SRE_SHIFT = 10;
static constexpr int URE_SHIFT = 11;

// <15:12> Write enables: KWE/EWE/SWE/UWE
static constexpr quint64 WRITE_EN_MASK = 0x000000000000F000ULL;

static constexpr int KWE_SHIFT = 12;
static constexpr int EWE_SHIFT = 13;
static constexpr int SWE_SHIFT = 14;
static constexpr int UWE_SHIFT = 15;

// <31:16> Software-defined bits
static constexpr quint64 SW_MASK = 0x00000000FFFF0000ULL;
// ---------------------------------------------------------------------
// Page geometry (EV6 uses 8 KB base pages)
// ---------------------------------------------------------------------
// static constexpr unsigned PAGE_SHIFT = 13;   // 8KB pages
// static constexpr quint64 PAGE_SIZE = 1ULL << PAGE_SHIFT;

// EV6 physical address width = 44 bits (standard 21264)
static constexpr unsigned PA_BITS = 44;


struct Ev6PteTraits
{
	// ---------------------------------------------------------------------
	// Base Accessors (AlphaPTE::extract<N,M>() is used)
	// ---------------------------------------------------------------------

	static inline bool valid(const AlphaPTE& p) noexcept
	{
		return p.extract<0, 1>() != 0;
	}

	static inline bool global(const AlphaPTE& p) noexcept
	{
		return p.extract<ASM_SHIFT, 1>() != 0;
	}

	// EV6 stores PFN in bits 51..32
	static inline quint32 pfn(const AlphaPTE& p) noexcept
	{
		return static_cast<quint32>(p.extract<PFN_SHIFT, PFN_WIDTH>());
	}

	static inline void setPfn(AlphaPTE& p, quint32 v) noexcept
	{
		p.insert<PFN_SHIFT, PFN_WIDTH>(v);
	}

	static inline unsigned tbBlockHint(const AlphaPTE& p) noexcept
	{
		return p.extract<GH_SHIFT, 2>();
	}

	static inline void setTbBlockHint(AlphaPTE& p, unsigned hint) noexcept
	{
		p.insert<GH_SHIFT, 2>(hint & 0x3);
	}

	// ---------------------------------------------------------------------
	// Permission Mask (what the TLB silicon stores)
	//
	// Mapping:
	//   bit0 = U_R
	//   bit1 = U_W
	//   bit2 = K_R
	//   bit3 = K_W
	// ---------------------------------------------------------------------

	static inline AlphaN_S::PermMask permMask(const AlphaPTE& p) noexcept
	{
		const bool k_r = p.extract<KRE_SHIFT, 1>();
		const bool k_w = p.extract<KWE_SHIFT, 1>();
		const bool u_r = p.extract<URE_SHIFT, 1>();
		const bool u_w = p.extract<UWE_SHIFT, 1>();

		return (u_r ? 0x01 : 0) |
			(u_w ? 0x02 : 0) |
			(k_r ? 0x04 : 0) |
			(k_w ? 0x08 : 0);
	}

	// ---------------------------------------------------------------------
	// Sanitization (mask out illegal bits)
	// ---------------------------------------------------------------------
	static inline void sanitize(AlphaPTE& p) noexcept
	{
		constexpr quint64 ALLOWED =
			VALID_MASK |
			FAULT_MASK |
			ASM_MASK |
			GH_MASK |
			READ_EN_MASK |
			WRITE_EN_MASK |
			SW_MASK |
			PFN_MASK;

		// Clear all bits except the allowed architectural ones
		p.raw &= ALLOWED;

		// EV6 requires <7> = 0 (reserved)
		p.raw &= ~RSVD_MASK;
	}
};
