// ============================================================================
// buildCanonicalTLBTag_inl.h - ---------------------------------------------------------------------------
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

#ifndef BUILDCANONICALTLBTAG_INL_H
#define BUILDCANONICALTLBTAG_INL_H
// ---------------------------------------------------------------------------
// buildCanonicalTLBTag
//
// Build a canonical SPAM/TLB tag from VA + realm + sizeClass (GH).
// This MUST match SPAMShardManager::makeTag semantics.
//
// Source:
// - Alpha AXP Architecture Reference Manual
//   PTE.GH (Granularity Hint) -> pageShift
// ---------------------------------------------------------------------------
AXP_FLATTEN
    SPAMTag<Ev6PTETraits> buildCanonicalTLBTag(
        VAType va,
        Realm realm,
        SC_Type sizeClass
        ) noexcept
{
    // Derive page shift from GH
    const quint64 pageShift = PageSizeHelpers::pageShift(sizeClass);

    // Canonicalize VA to page base
    const VAType canonVA = va & ~((1ULL << pageShift) - 1);

    // Build tag using the SAME logic as SPAMShardManager / Traits
    SPAMTag<Ev6PTETraits> tag{};
    tag.vpn = canonVA >> pageShift;
    tag.sizeClass = sizeClass;
    tag.realm = realm;
    tag.matchAllASNs = false; // updated later from ASM if needed

    return tag;
}

#endif // BUILDCANONICALTLBTAG_INL_H
