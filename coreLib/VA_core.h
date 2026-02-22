// ============================================================================
// VA_core.h - Extract the 2-bit segment selector from a virtual address.
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

#ifndef VA_CORE_XX_H__
#define VA_CORE_XX_H__
#include <QtGlobal>
#include "../coreLib/enum_header.h"
#include "../coreLib/types_core.h"
#include "../pteLib/alpha_pte_core.h"
#include "VA_types.h"
#include "../faultLib/fault_core.h"
#include "../memoryLib/memory_core.h"


inline TLB_BANK selectTLBBank(quint64 va) noexcept {
	// Example: high VA = bank 1, else bank 0
	return (va & 0xFFFF800000000000ULL) == 0xFFFF800000000000ULL
		? TLB_BANK::BANK1 : TLB_BANK::BANK0;
}

// Alpha AXP Virtual Address and ASN limits (EV6)

constexpr quint8  VA_BITS = 43;          // 43 bits used for VA
constexpr quint64 MAX_VIRTUAL_ADDRESS = 0x7FFFFFFFFFFULL; // Maximum user VA (43 bits)

// Optional: for sign-extended VA (canonical form)
constexpr quint64 CANONICAL_VA_MASK = (1ULL << VA_BITS) - 1; // Mask to extract 43-bit VA

// Example: To check if an address is within canonical user VA range:
AXP_HOT AXP_ALWAYS_INLINE bool  isCanonicalUserVA(quint64 va) noexcept {
	return va <= MAX_VIRTUAL_ADDRESS;
}

// To check if an ASN is valid:
AXP_HOT AXP_ALWAYS_INLINE bool  isValidASN(ASNType asn) noexcept {
	return asn < MAX_ASN; // 0..255 valid
}

AXP_HOT AXP_ALWAYS_INLINE bool  isValidAddressInASN(VAType va, ASNType asn) noexcept {
	// Check VA within supported virtual address range
	if (va > MAX_VIRTUAL_ADDRESS) return false;

	// Check ASN is in valid range
	if (asn >= MAX_ASN) return false;

	// (Optional) Check current context's valid ASN list, or process mapping table

	return true; // Or more complex check
}

// Example OS-specific VA region constants (customize for your OS)
static constexpr quint64 USER_MIN_48 = 0x0000000000000000ULL;
static constexpr quint64 USER_MAX_48 = 0x0000FFFFFFFFFFFFULL;
static constexpr quint64 KERNEL_MIN_48 = 0xFFFF000000000000ULL;
static constexpr quint64 KERNEL_MAX_48 = 0xFFFFFFFFFFFFFFFFULL;

static constexpr quint64 USER_MIN_43 = 0x0000000000000000ULL;
static constexpr quint64 USER_MAX_43 = 0x000003FFFFFFFFFFULL;
static constexpr quint64 KERNEL_MIN_43 = 0xFFFFFC0000000000ULL;
static constexpr quint64 KERNEL_MAX_43 = 0xFFFFFFFFFFFFFFFFULL;


// Helper: Is VA in 48-bit mode?
AXP_HOT AXP_ALWAYS_INLINE bool  isVA48(VAType va_ctl) noexcept { return (va_ctl & 0x2) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool  isVA43(VAType va_ctl) noexcept { return !isVA48(va_ctl); }
AXP_HOT AXP_ALWAYS_INLINE bool  isBitEndian(VAType va_ctl) noexcept { return (va_ctl & 0x1) != 0; }

// Extract VPN
// inline quint64 extractVPN(VAType va, VAType va_ctl) noexcept {
// 	return isVA48(va_ctl) ? ((va & 0xFFFFFFFFFFFE0000ULL) >> 13)
// 		: ((va & 0x7FFFFFE0000ULL) >> 13);
// }

// Extract offset
inline quint64 extractOffset(VAType va) noexcept {
	return va & 0x1FFFULL;
}

// VA_FORM computation (per your spec)
inline quint64 computeVA_FORM(VAType va, VAType va_ctl, quint64 vptb) noexcept {
	quint64 form = (vptb & 0xFFFFFFFFFFF80000ULL); // VPTB[63:33]
	if (isVA48(va_ctl)) {
		form |= ((va & 0x3F00000000000ULL) >> 6);
		form |= ((va & 0xFFFFE0000ULL) << 16);
		form |= (quint64(qint64(va) >> 58) << 38);
	}
	else {
		if (va_ctl & 0x4) { // VA_CTL_FORM_MASK
			form |= (vptb & 0xC0000000ULL);
			form |= ((va & 0xFFFFE0000ULL) << 3);
		}
		else {
			form |= ((va & 0x20000000000ULL) >> 10);
			form |= ((va & 0xFFFFE0000ULL) << 3);
		}
	}
	if (va_ctl & 0x4) form |= (1ULL << 2);
	if (isBitEndian(va_ctl)) form |= 0x1;
	return form;
}

// Classify address (user/kernel/unknown)
AXP_HOT inline AddressClass classifyVA(VAType va, VAType va_ctl) noexcept {
	if (isVA48(va_ctl)) {
		if (va >= USER_MIN_48 && va <= USER_MAX_48)   return AddressClass::User;
		if (va >= KERNEL_MIN_48 && va <= KERNEL_MAX_48) return AddressClass::Kernel;
	}
	else {
		if (va >= USER_MIN_43 && va <= USER_MAX_43)   return AddressClass::User;
		if (va >= KERNEL_MIN_43 && va <= KERNEL_MAX_43) return AddressClass::Kernel;
	}
	return AddressClass::Unknown;
}


AXP_HOT AXP_ALWAYS_INLINE bool  isUserVA(VAType va, quint64 va_ctl)   noexcept { return classifyVA(va, va_ctl) == AddressClass::User; }
AXP_HOT AXP_ALWAYS_INLINE bool  isKernelVA(VAType va, quint64 va_ctl) noexcept { return classifyVA(va, va_ctl) == AddressClass::Kernel; }


#pragma region Canonical VA + Alignment Helpers

// Simple "canonical VA" check using your existing VA classification logic.
// Canonical == belongs to either the user or kernel region defined in VA_Core.
AXP_HOT AXP_ALWAYS_INLINE bool  isCanonicalVA(VAType va, VAType va_ctl) noexcept
{
	AddressClass cls = classifyVA(va, va_ctl);
	return cls != AddressClass::Unknown;
}

// EV6-style alignment helper.
// You can extend this as you wire in specific instruction sizes.
AXP_HOT AXP_ALWAYS_INLINE bool  ev6CheckAlignment(VAType va, AccessKind access) noexcept
{
	// NOTE:
	//  - For now we assume:
	//      * EXECUTE        : always OK (I-fetch alignment handled in I-stream)
	//      * READ/WRITE     : 8-byte alignment for "natural" LDQ/STQ.
	//    You can specialize this per instruction later (LDL vs LDQ_U, etc).
	switch (access)
	{
	case AccessKind::EXECUTE:
		return true; // instruction alignment handled elsewhere

	case AccessKind::READ:
	case AccessKind::WRITE:
		// Default: require 8-byte alignment
		return (va & 0x7ULL) == 0ULL;

	default:
		return true;
	}
}

#pragma endregion Canonical VA + Alignment Helpers

#pragma region kSeg Helper Functions

// ============================================================================
// Kseg (Kernel Superpage) Detection & Translation
// ============================================================================
//
// Alpha Virtual Address Space Segments (top 2 VA bits):
//
//   VA[segHi:segLo] == 00    seg0   (mapped via page tables, user+kernel)
//   VA[segHi:segLo] == 01    INVALID (access violation trap)
//   VA[segHi:segLo] == 10    kseg   (direct physical map, kernel only)
//   VA[segHi:segLo] == 11    seg1   (mapped via page tables, kernel only)
//
// Segment bit positions depend on VA size configured in I_CTL[VA_48]:
//   I_CTL[VA_48] = 0  43-bit VA  segment bits = VA[42:41]
//   I_CTL[VA_48] = 1  48-bit VA  segment bits = VA[47:46]
//
// Kseg identity-maps virtual to physical with no TLB or page walk:
//   PA = VA[43:0]   (EV6 physical address size = 44 bits)
//
// Kseg is kernel-only. User-mode access to kseg  access violation.
//
// Reference: Alpha Architecture Reference Manual, Section 5.3.2
//            21264/EV6 Hardware Reference Manual, Section 5.2.2
// ============================================================================

/// @brief Extract the 2-bit segment selector from a virtual address.
///
/// @param va     Full 64-bit virtual address (sign-extended)
/// @param va_ctl Cached I_CTL register value (bit 1 = VA_48)
/// @return Segment selector: 0=seg0, 1=invalid, 2=kseg, 3=seg1

AXP_HOT AXP_ALWAYS_INLINE
quint8 extractSegment(VAType va, VAType va_ctl) noexcept
{
	// I_CTL bit 1: VA_48 mode select
	//   0  43-bit VA, segment bits at [42:41]
	//   1  48-bit VA, segment bits at [47:46]
	const int segShift = (va_ctl & 0x2) ? 46 : 41;
	return static_cast<quint8>((va >> segShift) & 0x3);
}

/// @brief Test whether a VA falls in kseg (kernel direct-mapped superpage).
///
/// @param va     Full 64-bit virtual address
/// @param va_ctl Cached I_CTL register value
/// @return true if VA is in kseg segment

AXP_HOT AXP_ALWAYS_INLINE
bool isKseg(VAType va, VAType va_ctl) noexcept
{
	return extractSegment(va, va_ctl) == 2;   // segment 10 binary
}

/// @brief Convert a kseg virtual address to physical address.
///
/// Identity mapping: PA = VA[43:0]
/// EV6 physical address is 44 bits wide.
///
/// @param va  Kseg virtual address (caller must verify isKseg first)
/// @return Physical address (44-bit)

AXP_HOT AXP_ALWAYS_INLINE
PAType ksegToPhysical(VAType va) noexcept
{
	constexpr quint64 EV6_PA_MASK = (1ULL << 44) - 1;   // 0x00000FFFFFFFFFFF
	return static_cast<PAType>(va & EV6_PA_MASK);
}

/// @brief Fast-path kseg translation for use in ev6TranslateFullVA.
///
/// Call BEFORE page table walk. If VA is in kseg and mode is kernel,
/// returns Success with pa_out set - no TLB lookup, no page walk.
///
/// If VA is not kseg, returns NotKseg so caller continues to page walk.
/// If VA is kseg but mode is not kernel, returns AccessViolation.
///
/// @param va      Full 64-bit virtual address
/// @param va_ctl  Cached I_CTL register value
/// @param mode    Current processor mode (kernel/exec/super/user)
/// @param pa_out  [out] Physical address if kseg hit
/// @return Success if kseg translated, NotKseg if not kseg, 
///         AccessViolation if kseg from non-kernel mode

AXP_HOT AXP_ALWAYS_INLINE
TranslationResult tryKsegTranslate(
	VAType          va,
	VAType          va_ctl,
	Mode_Privilege  mode,
	PAType& pa_out) noexcept
{
	if (!isKseg(va, va_ctl))
		return TranslationResult::NotKseg;         // Not kseg  caller does page walk

	// Kseg is kernel-only
	if (mode != Mode_Privilege::Kernel)
		return TranslationResult::AccessViolation;  // User/super/exec  ACV trap

	// Direct identity map: PA = VA[43:0]
	pa_out = ksegToPhysical(va);
	return TranslationResult::Success;
}

#pragma endregion kSeg Helper Functions



// ============================================================================
// VA/Mem result translation helpers
// ============================================================================

inline TrapCode_Class translateResultToTrap(TranslationResult r, AccessKind k) noexcept
{
	switch (r) {
	case TranslationResult::TlbMiss:
		return (k == AccessKind::EXECUTE)
			? TrapCode_Class::ITB_MISS
			: TrapCode_Class::DTB_MISS;

	case TranslationResult::AccessViolation:
		return (k == AccessKind::EXECUTE)
			? TrapCode_Class::ITB_ACCESS_VIOLATION
			: TrapCode_Class::DTB_ACCESS_VIOLATION;

	case TranslationResult::NonCanonical:
	case TranslationResult::Unaligned:
		return TrapCode_Class::DTB_FAULT;

	default:
		return TrapCode_Class::MACHINE_CHECK;
	}
}

inline TrapCode_Class translateStatusToTrap(MEM_STATUS st) noexcept
{
	switch (st) {
	case MEM_STATUS::Ok:
		return TrapCode_Class::NONE;

	case MEM_STATUS::AccessViolation:
		return TrapCode_Class::DTB_ACCESS_VIOLATION;

	case MEM_STATUS::Un_Aligned:
		return TrapCode_Class::DTB_FAULT;

	case MEM_STATUS::BusError:
	default:
		return TrapCode_Class::MACHINE_CHECK;
	}
}


AXP_HOT AXP_ALWAYS_INLINE bool  isInSuperpage(quint64 va)  noexcept {
	// PAL/SRM region (always physical)
	if (va >= 0x20000000 && va < 0x20200000) {
		return true;
	}

	// MMIO regions (always physical)
	if (va >= 0xF0000000 && va < 0x100000000ULL) {
		return true;
	}

	// High MMIO
	if (va >= 0x1000000000ULL && va < 0x2000000000ULL) {
		return true;
	}

	// HWRPB region
	if (va < 0x10000) {
		return true;
	}

	return false;
}


#endif // VA_core_h__
