#pragma once

#include <QtGlobal>
#include "AlphaPTE_Core.h"


    /*
     * alpha_pte_traits_ev6_dtb.h
     *
     * EV6 (DEC 21264) DTB PTE adapter helpers.
     *
     * This header provides a helper struct that knows how to:
     *
     *   - Decode an EV6 DTB_PTE write-format image into a canonical AlphaPTE.
     *   - Encode a canonical AlphaPTE into the DTB_PTE read-format image.
     *   - Extract a virtual address from a DTB_TAGx value (VA[43:0]).
     *
     * It is intended for use by the DTB_PTE0 / DTB_PTE1 IPR handlers in
     * CPUStateIPRInterface and the M-box IPR hooks.
     *
     * Architectural references:
     *   - Alpha AXP System Reference Manual (ASA), Vol I:
     *       Virtual memory and PTE bit definitions (V, FOE, FOW, FOR, ASM, GH,
     *       per-mode read/write enable bits).
     *   - DEC 21264 Alpha Microprocessor Hardware Reference Manual:
     *       Memory management chapter, DTB_TAG0/1 and DTB_PTE0/1 register formats.
     */



    // ---------------------------------------------------------------------
    // EV6 specialization for ITB PTE handling
    // ---------------------------------------------------------------------
	// ============================================================================
	// EV6 DTB PTE Adapter (non-templated, EV6-only)
	//
	// Provides:
	//   - decodeVAFromDTBTag()
	//   - fromDtbPteWrite()
	//   - toDtbPteRead()
	//
	// Matches EV6 (DEC 21264) DTB_TAGx and DTB_PTEx register behavior.
	//
	// Based entirely on ASA Volume I and 21264 HRM.
	// ============================================================================

	struct Ev6_DtbPteAdapter
	{
		using PTE = AlphaPTE;

		// ---------------------------------------------------------------------
		// EV6 DTB_PTE bit positions (write format)
		//
		// PFN   : bits 51:32 (20 bits)
		// ASM   : bit  34
		// URE   : bit  12
		// SRE   : bit  11
		// ERE   : bit  10
		// KRE   : bit   9
		// UWE   : bit   8
		// SWE   : bit   7
		// EWE   : bit   6
		// KWE   : bit   5
		// FOW   : bit   4
		// FOR   : bit   3
		//
		// FOE is **NOT** present in DTB_PTE write format.
		// ---------------------------------------------------------------------


		static constexpr quint8 ASM_BIT = 34;

		static constexpr quint8 URE = 12;
		static constexpr quint8 SRE = 11;
		static constexpr quint8 ERE = 10;
		static constexpr quint8 KRE = 9;

		static constexpr quint8 UWE = 8;
		static constexpr quint8 SWE = 7;
		static constexpr quint8 EWE = 6;
		static constexpr quint8 KWE = 5;

		static constexpr quint8 FOW = 4;
		static constexpr quint8 FOR_ = 3;

		// Small helper
		static constexpr quint64 maskN(quint8 width) noexcept
		{
			return (width == 64 ? ~0ULL : ((1ULL << width) - 1ULL));
		}

		// ---------------------------------------------------------------------
		// Decode DTB_TAG (VA in low 44 bits)
		// ---------------------------------------------------------------------
		static inline quint64 decodeVAFromDTBTag(TAGType rawTag) noexcept
		{
			constexpr quint64 VAMask = (1ULL << 44) - 1ULL;
			return rawTag & VAMask;
		}

		// ---------------------------------------------------------------------
		// Decode DTB_PTE write format ->  AlphaPTE
		// (used by MTPR DTB_PTE0/1)
		// ---------------------------------------------------------------------
		static inline AlphaPTE fromDtbPteWrite(quint64 raw) noexcept
		{
			AlphaPTE p(0);

			// PFN
			const PFNType pfn =
				static_cast<PFNType>((raw >> PFN_SHIFT) & maskN(PFN_WIDTH));
			p.setPfn(pfn);

			// ASM (global)
			const bool asmFlag = ((raw >> ASM_BIT) & 0x1ULL) != 0;
			p.setAsm(asmFlag);

			// Read permissions (K/E/S/U)
			const bool kre = ((raw >> KRE) & 0x1ULL);
			const bool ere = ((raw >> ERE) & 0x1ULL);
			const bool sre = ((raw >> SRE) & 0x1ULL);
			const bool ure = ((raw >> URE) & 0x1ULL);
			p.setReadPermissions(kre, ere, sre, ure);

			// Write permissions (K/E/S/U)
			const bool kwe = ((raw >> KWE) & 0x1ULL);
			const bool ewe = ((raw >> EWE) & 0x1ULL);
			const bool swe = ((raw >> SWE) & 0x1ULL);
			const bool uwe = ((raw >> UWE) & 0x1ULL);
			p.setWritePermissions(kwe, ewe, swe, uwe);

			// FOW / FOR
			const bool fow = ((raw >> FOW) & 0x1ULL);
			const bool forv = ((raw >> FOR_) & 0x1ULL);

			p.insert<AlphaPTE::PTE_BIT_FOW, 1>(fow);
			p.insert<AlphaPTE::PTE_BIT_FOR, 1>(forv);

			// Mark valid if PFN != 0
			if (pfn != 0)
				p.setValid(true);

			return p;
		}

		// ---------------------------------------------------------------------
		// Encode AlphaPTE ->  DTB_PTE read format
		// (used by MFPR DTB_PTE0/1)
		// ---------------------------------------------------------------------
		static inline quint64 toDtbPteRead(const AlphaPTE& p) noexcept
		{
			quint64 raw = 0;

			// PFN
			const PFNType pfn = p.pfn();
			raw |= (static_cast<PFNType>(pfn) & maskN(PFN_WIDTH)) << PFN_SHIFT;

			// ASM
			raw |= (p.bitASM() ? 1ULL : 0ULL) << ASM_BIT;

			// Read permissions
			bool kre, ere, sre, ure;
			p.getReadPermissions(kre, ere, sre, ure);

			if (kre) raw |= (1ULL << KRE);
			if (ere) raw |= (1ULL << ERE);
			if (sre) raw |= (1ULL << SRE);
			if (ure) raw |= (1ULL << URE);

			// Write permissions
			bool kwe, ewe, swe, uwe;
			p.getWritePermissions(kwe, ewe, swe, uwe);

			if (kwe) raw |= (1ULL << KWE);
			if (ewe) raw |= (1ULL << EWE);
			if (swe) raw |= (1ULL << SWE);
			if (uwe) raw |= (1ULL << UWE);

			// FOW / FOR
			if (p.bitFOW()) raw |= (1ULL << FOW);
			if (p.bitFOR()) raw |= (1ULL << FOR_);

			return raw;
		}
	};


