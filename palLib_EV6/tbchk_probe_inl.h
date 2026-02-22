// ============================================================================
// tbchk_probe_inl.h - tbchk_spam_probe_inl.h
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

#ifndef VS2022_EMULATR_EMULATRAPPUNI_PALLIB_EV6_TBCHK_PROBE_INL_H
#define VS2022_EMULATR_EMULATRAPPUNI_PALLIB_EV6_TBCHK_PROBE_INL_H

// tbchk_spam_probe_inl.h
// ============================================================================
// TBCHK probe wired to SPAMShardManager (header-only, ASCII)
// ============================================================================
//
// Implements TBCHK presence probing by calling your authoritative SPAM/TLB cache.
//
// Authoritative behavior (encoding only, probe policy is yours):
//   Alpha AXP SRM v6 (1994), OpenVMS AXP Software (II-A),
//   Section 5.3.18 "Translation Buffer Check (TBCHK)", page 5-24.
//     - Operand VA is the address to be checked (any address in page).
//     - ASN-qualified if ASNs are implemented.
//     - Return encoding:
//         Not implemented: bit63=1, bit0=0
//         Implemented:     bit63=0, bit0=1 if present else 0
//
// Integration with your SPAMShardManager:
//   SPAMShardManager::tlbLookup() already enforces "probe all GH values" and
//   checks both global and non-global tags. It is read-only and SMP-friendly.
//
// Policy note:
//   SRM text says "Translation Buffer" generically. This helper probes BOTH:
//     - DTB (Realm::D) and
//     - ITB (Realm::I)
//   and returns present if either hits.
//
// TODO: If you later want DTB-only behavior, remove the ITB probe below.
//

#include <QtCore/QtGlobal>

#include "pteLib/alpha_spam_manager.h"   // SPAMShardManager
#include "pteLib/alpha_spam_types.h"     // Realm, ASNType, PFNType, SC_Type
#include "pteLib/alpha_pte_core.h"       // AlphaN_S::PermMask (or your core header)
#include "corelib/Axp_Attributes_core.h"
#include "coreLib/types_core.h"
#include "pteLib/Ev6SiliconTLB_Singleton.h"

// ----------------------------------------------------------------------------
// Probe presence of a cached translation for (va, asn).
// Uses SPAMShardManager::tlbLookup(), which probes all GH (3..0) and both
// global and non-global tags.
// ----------------------------------------------------------------------------
AXP_HOT AXP_ALWAYS_INLINE bool tbchkProbePresent_EV6(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
{
	auto& spam = globalEv6SPAM();

	PFNType pfn = 0;
	AlphaN_S::PermMask perm{};
	SC_Type sizeClass = 0;

	// Probe DTB (D-stream)
	const bool hitD = spam.tlbLookup(cpuId, Realm::D, va, asn, pfn, perm, sizeClass);

	// Probe ITB (I-stream)
	const bool hitI = spam.tlbLookup(cpuId, Realm::I, va, asn, pfn, perm, sizeClass);

	return hitD || hitI;
}



#endif
