// ============================================================================
// TLB_Helpers_inl.h - Extract Virtual Page Number from TLB tag
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

#ifndef TLB_HELPERS_H_H
#define TLB_HELPERS_H_H

// ============================================================================
// TLB Tag Extraction (Generic - works for both DTB and ITB)
// ============================================================================
#include <QtGlobal>
#include "pteLib/alpha_pte_core.h"
#include "pteLib/AlphaPTE_Core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/types_core.h"

static constexpr unsigned DTB_TAG_ASN_BITS = 8;
static constexpr quint64 DTB_TAG_ASN_MASK = (1ULL << DTB_TAG_ASN_BITS) - 1;

/**
 * @brief Extract Virtual Page Number from TLB tag
 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
 * @return VPN (Virtual Page Number)
 */
AXP_ALWAYS_INLINE quint64 extractVPNFromTLBTag(quint64 tag) noexcept
{
    // VPN is everything above ASN
    return tag >> DTB_TAG_ASN_BITS;
}

/**
 * @brief Extract virtual address from TLB tag
 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
 * @return Page-aligned virtual address
 */
AXP_ALWAYS_INLINE  quint64 extractVAFromTLBTag(quint64 vpn,
    const quint8 sizeClass,
    quint64 originalVA) noexcept
{
    const quint64 shift = PageSizeHelpers::pageShift(sizeClass);
    const quint64 pageOffsetMask = (1ULL << shift) - 1;
    return (vpn << shift) | (originalVA & pageOffsetMask);
}

/**
 * @brief Extract virtual address from TLB tag
 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
 * @return Page-aligned virtual address
 */
AXP_ALWAYS_INLINE quint64 extractVAFromTLBTag(quint64 tag) noexcept {
    const quint64 vpnMask = 0xFFFFFFFFFFFE000ULL;  // Bits [63:13]
    return tag & vpnMask;
}

/**
 * @brief Extract ASN from TLB tag
 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
 * @return Address Space Number (bits 12:5)
 */
AXP_ALWAYS_INLINE ASNType extractASNFromTLBTag(quint64 tag) noexcept {
    return (tag >> 5) & 0xFF;  // Bits [12:5]
}

AXP_ALWAYS_INLINE  PFNType extractPFNFromPTE(quint64 pteRaw) noexcept
{
    // Canonical Alpha memory PTE: PFN in bits 63..32
    // Mask to the width you actually implement (28 bits here)
    return static_cast<PFNType>((pteRaw >> 32) & ((1ULL << 28) - 1));
}

AXP_ALWAYS_INLINE  SC_Type extractSizeClassFromPTE(const AlphaPTE& pte) noexcept
{
    return pte.gh();
}

// ============================================================================
// Deprecated Aliases (backward compatibility - remove after refactoring)
// ============================================================================
[[deprecated("Use extractVPNFromTLBTag instead")]]
AXP_ALWAYS_INLINE quint64 extractVPNFromDTBTag(quint64 tag) noexcept {
    return extractVPNFromTLBTag(tag);
}

[[deprecated("Use extractVPNFromTLBTag instead")]]
AXP_ALWAYS_INLINE quint64 extractVPNFromITBTag(quint64 tag) noexcept {
    return extractVPNFromTLBTag(tag);
}

[[deprecated("Use extractASNFromTLBTag instead")]]
AXP_ALWAYS_INLINE ASNType extractASNFromITBTag(quint64 tag) noexcept {
    return extractASNFromTLBTag(tag);
}
#endif // TBL_HELPERS_H_H   