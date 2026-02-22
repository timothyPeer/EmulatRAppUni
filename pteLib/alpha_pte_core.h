// ============================================================================
// alpha_pte_core.h - include/alpha_pte_core.h
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

#ifndef _EMULATRAPPUNI_PTELIB_ALPHA_PTE_CORE_H
#define _EMULATRAPPUNI_PTELIB_ALPHA_PTE_CORE_H

#pragma once
// include/alpha_pte_core.h
#include <QtCore/QtGlobal>
#include "../coreLib/types_core.h"
#include "../coreLib/Axp_Attributes_core.h" 
#include <QtCore/QAtomicInteger>   // defines QAtomicInteger<T>
#include <array>                   // needed for std::array

static constexpr ASNType	 MAX_ASN = 256;

#pragma region Physical Page Number (PFN)


static constexpr quint64 makePFNMask() noexcept {	// EV6: PFN occupies bits [59:32] -> width = 28 bits
	return ((~0ULL >> (64 - PFN_WIDTH)) << PFN_SHIFT);
}

static constexpr quint64 PFN_MASK = ((~0ULL >> (64 - PFN_WIDTH)) << PFN_SHIFT);

#pragma endregion Physical Page Number (PFN)


 // ==================== PRIVILEGE MODES ====================
 enum Mode_Privilege : quint8 {   // ProcessorMode
	 Kernel = 0,  // KRE/KWE at bits 8/12
	 Executive = 1,  // ERE/EWE at bits 9/13
	 Supervisor = 2,  // SRE/SWE at bits 10/14
	 User = 3   // URE/UWE at bits 11/15
 };


	// Canonical protection model
	enum class AccessPerm : quint8 {
		None = 0b000,
		Execute = 0b001,
		Read = 0b010,
		ReadExec = 0b011,
		Write = 0b100,
		WriteExec = 0b101,
		ReadWrite = 0b110,
		Full = 0b111,
	};

	struct EntryLayout {
			PFNType pfn;        // Physical Frame Number
			quint64 protection; // Access permissions
			bool valid;         // Entry validity
			bool dirty;         // Write-modified
			ASNType asn;         // Address Space Number
	};

	static constexpr int kRealmCount = 2;                 // D/I
	static constexpr int kSizeClassCount = 4;

	// If you shard by size class, define a compact id (0..N-1)
	// Map your PageSizeCode to this ID in one place.

	enum class Realm : quint8 { D = 0, I = 1, Both = 2 };         // Data-TB, Instr-TB, Both-TB


	// Page size computation helpers
	class PageSizeHelpers {
	public:
		[[nodiscard]] static constexpr quint64 pageShift(SC_Type gh) noexcept {
			switch (gh) {
			case 0: return 13;  // 8KB
			case 1: return 16;  // 64KB
			case 2: return 19;  // 512KB
			case 3: return 22;  // 4MB
				Q_ASSERT(false && "Invalid GH (sizeClass) in pageShift");
			default: return 13;
			}
		}

		[[nodiscard]] static constexpr quint64 pageSizeBytes(SC_Type gh) noexcept {
			return 1ULL << pageShift(gh);
		}
	};

	// Protection computation helpers
	class ProtectionHelpers {
	public:
		[[nodiscard]] static constexpr bool allowRead(AccessPerm perm, bool userMode) noexcept {
			Q_UNUSED(userMode)
			switch (perm) {
			case AccessPerm::Read:
			case AccessPerm::ReadExec:
			case AccessPerm::ReadWrite:
			case AccessPerm::Full:
				return true;
			default:
				return false;
			}
		}

		[[nodiscard]] static constexpr bool allowWrite(AccessPerm perm, bool userMode) noexcept {
			Q_UNUSED(userMode)
			switch (perm) {
			case AccessPerm::Write:
			case AccessPerm::WriteExec:
			case AccessPerm::ReadWrite:
			case AccessPerm::Full:
				return true;
			default:
				return false;
			}
		}

		[[nodiscard]] static constexpr bool allowExecute(AccessPerm perm, bool userMode) noexcept {
			Q_UNUSED(userMode)
			switch (perm) {
			case AccessPerm::Execute:
			case AccessPerm::ReadExec:
			case AccessPerm::WriteExec:
			case AccessPerm::Full:
				return true;
			default:
				return false;
			}
		}
		// 
		enum class AccessIntent : quint8 {
			Read,    // Data read access
			Write,   // Data write access
			Execute  // Instruction fetch
		};
	};

	struct AsnGenTable
	{
		//QAtomicInteger<quint32> gen[MAX_ASN];
       std::array<QAtomicInt, 256> gen;

		AsnGenTable()
		{
			for (unsigned int i = 0; i < 256; ++i)
			{
				gen[i].storeRelaxed(0);
			}
		}
	};

	struct SweepTable
	{
		//QAtomicInteger<quint8> flag[MAX_ASN];
       std::array<QAtomicInt, 256> flag;
	};

	namespace AlphaN_S {
		// SC_Type in your tree is used as "sizeClass" for SPAM/TLB tags.
		// In this facility, we define sizeClass == GH (0..3), not PageSizeCode.
		// Effective mapping size = 8KB * 8**GH.
		static constexpr quint8 kGH_Min = 0;
		static constexpr quint8 kGH_Max = 3;

		AXP_FLATTEN constexpr quint8 pageShiftFromGH(SC_Type gh) noexcept
		{
			// Base page is 8KB => shift 13.
			// Each increment of GH multiplies pages by 8 => adds 3 to shift.
			// Ref: SRM v6.0, GH definition and alignment rule (PTE<6:5>).
			return static_cast<quint8>(13u + (static_cast<quint8>(gh) * 3u));
		}

		AXP_FLATTEN constexpr quint64 pageMaskFromGH(SC_Type gh) noexcept
		{
			// Mask for the "effective mapped block/page" implied by GH.
			const quint8 sh = pageShiftFromGH(gh);
			return ~((1ULL << sh) - 1ULL);
		}




		// A simple "permission mask" if you want to cache access rights.
		// You can map bits as you like (e.g., bit0=U_R, bit1=U_W, bit2=K_R, bit3=K_W).
		using PermMask = quint8;

		// ======================================================================
		// PermMask Bit Helpers
		// ======================================================================
		static constexpr unsigned PTE_BIT_V = 0;  // Valid
		static constexpr quint64 PTE_MASK_V = (1ULL << PTE_BIT_V);
		static constexpr unsigned PTE_BIT_FOR = 1;  // Fault on Read
		static constexpr unsigned PTE_BIT_FOW = 2;  // Fault on Write
		static constexpr unsigned PTE_BIT_FOE = 3;  // Fault on Execute

		static constexpr unsigned PTE_BIT_ASM = 4;  // Address Space Match
		// Read enables
		static constexpr unsigned PTE_BIT_KRE = 8;
		static constexpr unsigned PTE_BIT_ERE = 9;
		static constexpr unsigned PTE_BIT_SRE = 10;
		static constexpr unsigned PTE_BIT_URE = 11;

		// Write enables
		static constexpr unsigned PTE_BIT_KWE = 12;
		static constexpr unsigned PTE_BIT_EWE = 13;
		static constexpr unsigned PTE_BIT_SWE = 14;
		static constexpr unsigned PTE_BIT_UWE = 15;

		// Reserved
		static constexpr unsigned PTE_BIT_RSVD_START = 16;
		static constexpr unsigned PTE_BIT_RSVD_END = 31;

		// PFN bits
		static constexpr unsigned PTE_BIT_PFN_LSB = 32; // PFN = bits 52:32
		static constexpr unsigned PTE_BIT_PFN_MSB = 52;

		// Reserved (high)
		static constexpr unsigned PTE_BIT_RSVD2_START = 53;
		static constexpr unsigned PTE_BIT_RSVD2_END = 63;

		// ======================================================================
		// Generic extract helpers for PermMask (8-bit TLB permissions)
		// ======================================================================

		// Extract a single bit from an 8-bit PermMask
		static inline bool extractBit(PermMask pm, unsigned bit) noexcept
		{
			return (pm >> bit) & 0x1;
		}

		// Extract a multi-bit range (lo..lo+width-1)
		static inline quint8 extractBits(PermMask pm, unsigned lo, unsigned width) noexcept
		{
			return static_cast<quint8>((pm >> lo) & ((1u << width) - 1u));
		}

		// READ ENABLES
		static AXP_HOT AXP_FLATTEN bool kre(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_KRE); }
		static AXP_HOT AXP_FLATTEN  bool ere(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_ERE); }
		static AXP_HOT AXP_FLATTEN  bool sre(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_SRE); }
		static AXP_HOT AXP_FLATTEN  bool ure(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_URE); }

		// WRITE ENABLES
		static AXP_HOT AXP_FLATTEN  bool kwe(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_KWE); }
		static AXP_HOT AXP_FLATTEN  bool ewe(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_EWE); }
		static AXP_HOT AXP_FLATTEN  bool swe(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_SWE); }
		static AXP_HOT AXP_FLATTEN  bool uwe(PermMask pm) noexcept { return extractBit(pm, PTE_BIT_UWE); }

		// ======================================================================
		// Canonical TLB Permission Mask Bit Layout (8 bits)
		// ======================================================================
		//
		// We store only read/write enables and fault bits (optional) in the TLB.
		// This is the EV6-correct compressed TLB permission representation.
		//
		// Bit 0 : KRE  (Kernel Read Enable)
		// Bit 1 : ERE  (Executive Read Enable)
		// Bit 2 : SRE  (Supervisor Read Enable)
		// Bit 3 : URE  (User Read Enable)
		//
		// Bit 4 : KWE  (Kernel Write Enable)
		// Bit 5 : EWE  (Executive Write Enable)
		// Bit 6 : SWE  (Supervisor Write Enable)
		// Bit 7 : UWE  (User Write Enable)
		//
		// NOTE: If you want to include FOE/FOW/FOR in the mask, extend below.
		// ======================================================================

		static constexpr quint8 Perm_KRE = (1u << 0);
		static constexpr quint8 Perm_ERE = (1u << 1);
		static constexpr quint8 Perm_SRE = (1u << 2);
		static constexpr quint8 Perm_URE = (1u << 3);

		static constexpr quint8 Perm_KWE = (1u << 4);
		static constexpr quint8 Perm_EWE = (1u << 5);
		static constexpr quint8 Perm_SWE = (1u << 6);
		static constexpr quint8 Perm_UWE = (1u << 7);


		static constexpr quint8 PERM_MASK_BITS =
			Perm_KRE | Perm_ERE | Perm_SRE | Perm_URE |
			Perm_KWE | Perm_EWE | Perm_SWE | Perm_UWE;


		// Combined privilege views
		static AXP_HOT AXP_FLATTEN  bool canReadKernel(PermMask pm) noexcept {
			return kre(pm) || ere(pm) || sre(pm);
		}

		static AXP_HOT AXP_FLATTEN  bool canWriteKernel(PermMask pm) noexcept {
			return kwe(pm) || ewe(pm) || swe(pm);
		}

		static AXP_HOT AXP_FLATTEN  bool canReadUser(PermMask pm) noexcept {
			return ure(pm);
		}

		static AXP_HOT AXP_FLATTEN  bool canWriteUser(PermMask pm) noexcept {
			return uwe(pm);
		}
	}

#endif
