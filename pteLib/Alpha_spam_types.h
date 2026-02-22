// ============================================================================
// Alpha_spam_types.h - include/Alpha/alpha_spam_types.h
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

// include/Alpha/alpha_spam_types.h
#pragma once
#include "AlphaPTE_Core.h"
#include "alpha_pte_core.h"
#include "../coreLib/types_core.h"

	/*
		SPAMTag::sizeClass is treated as GH (PTE<6:5>) for EV6 TB fills.
		Effective mapping size shift = 13 + 3*GH.
		Ref: Alpha AXP SRM v6.0 (1994), PTE GH bits 6-5.
	*/

	template<typename Traits>
	struct SPAMTag
	{
		VAType  vpn;          // VA >> pageShiftFromGH(sizeClass)
		SC_Type sizeClass;    // GH: 0..3
		Realm   realm;        // 0 = D-TB, 1 = I-TB
		bool    matchAllASNs; // ASM behavior (matches all ASNs)

		static constexpr quint64 computeVPNFromVA(VAType va, SC_Type gh) noexcept
		{
			// GH is a hint that TB may treat a block as a single larger translation.
			// Use GH-derived shift to compute the tag VPN.
			return static_cast<quint64>(va >> AlphaN_S::pageShiftFromGH(gh));
		}

		bool operator==(const SPAMTag& o) const noexcept
		{
			return vpn == o.vpn && sizeClass == o.sizeClass && realm == o.realm;
		}
	};

	template<typename Traits>
	struct SPAMEntry
	{
		using Tag = SPAMTag<Traits>;
		Tag tag;

		quint32 globalGenAtFill{ 0 };
		AlphaPTE pteRaw{};
		ASNType  asnGenAtFill{ 0 };
		ASNType  asn{ 0 };
		PFNType  pfn{ 0 };
		AlphaN_S::PermMask permMask{};
		SC_Type  sizeClass{ 0 };     // store GH here as well (single source of truth per entry)

		struct {
			bool global : 1;
			bool valid : 1;
			bool locked : 1;
			bool transitioning : 1;
		} flags{};

		quint8 lruNibble{ 0 };
		quint8 pageShift{ 0 };

		constexpr bool isValid() const noexcept { return flags.valid && !flags.transitioning; }
		// Rule 1 - Invariant: tag.sizeClass must match entry.sizeClass
		AXP_FLATTEN void assertConsistency() const noexcept
		{
			Q_ASSERT(tag.sizeClass == sizeClass);
			Q_ASSERT(pageShift == PageSizeHelpers::pageShift(sizeClass));

			if (flags.valid && !flags.transitioning) {
				Q_ASSERT(pfn != 0);  // Valid entries should have PFN
			}
		}

		AXP_FLATTEN void syncDerivedFromSizeClass() noexcept
		{
			// Keep per-entry pageShift consistent with GH.
			pageShift = AlphaN_S::pageShiftFromGH(sizeClass);
			// Assert consistency after sync
			assertConsistency();
		}

		AXP_FLATTEN quint64 getPageMask() const noexcept
		{
			// GH-derived mask for matching addresses within the implied mapped block.
			return AlphaN_S::pageMaskFromGH(sizeClass);
		}
	};


