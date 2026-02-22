// ============================================================================
// alpha_pte_traits_ev6_itb.h - alpha pte traits ev6 itb
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

#include <QtGlobal>
#include "ev6_ITBPteWrite_Register.h"
#include "Ev6_ItbPteRead_Register.h"
#include "alpha_pte_core.h"
#include "enum_header.h"
#include "types_core.h"

/*
 * alpha_pte_traits_ev6_itb.h
 *
 * EV6 (DEC 21264) ITB PTE adapter helpers.
 *
 * This header shows how to map between:
 *   - architectural AlphaPTE (memory PTE image), and
 *   - ITB_PTE IPR encodings (read and write) using:
 *       Ev6_ItbPteRead_Register   // read format
 *       ev6_ITBPteWrite_Register  // write format
 *
 * Reference:
 *   DEC 21264 Alpha Microprocessor Hardware Reference Manual,
 *   MMU chapter, "Instruction Translation Buffer Page Table
 *   Entry Register" section.
 */


    template<CPUFamily Fam, unsigned VaBits>
    struct PTETraits;

    // ---------------------------------------------------------------------
    // EV6 specialization for ITB PTE handling
    // ---------------------------------------------------------------------
    template<>
    struct PTETraits<CPUFamily::EV6, 44U>
    {
        using PTE = AlphaPTE;

		// ---------------------------------------------------------------------
	    // Decode VA from ITB_TAG image (EV6).
	    //
	    // This is used when PALcode has written ITB_TAG via:
	    //     MTPR ITB_TAG, Rx
	    //
	    // and we later need the virtual address again when processing the
	    // corresponding ITB_PTE write:
	    //     MTPR ITB_PTE, Ry
	    //
	    // For now we treat ITB_TAG as containing the virtual address in the
	    // low 44 bits. We simply mask off higher bits and return VA[43:0].
	    //
	    // References:
	    //   - DEC 21264 Hardware Reference Manual, MMU chapter,
	    //     ITB_TAG / ITB_PTE programming model.
	    //   - Alpha Architecture Reference Manual, virtual address format
	    //     (44-bit VA, VA[43:13] VPN, VA[12:0] byte offset).
	    // ---------------------------------------------------------------------
		static inline quint64 decodeVAFromITBTag(TAGType rawTag) noexcept
		{
			// Architectural VA width for EV6: 44 bits
			constexpr quint64 VAMask = (1ULL << 44) - 1ULL;
			return rawTag & VAMask;
		}

        /*
         * Decode an ITB_PTE write-format image into a canonical AlphaPTE.
         *
         * This is used when the guest executes:
         *     MTPR ITB_PTE, Rn
         *
         * rawItbPte is the value written to the ITB_PTE IPR.
         */
        static inline PTE fromItbPteWrite(quint64 rawItbPte) noexcept
        {
            using WriteReg = ev6_ITBPteWrite_Register;

            PTE pte(0);

            // PFN: ITB_PTE write format uses PFN[52:32].
            const PFNType pfn = WriteReg::getPfn(rawItbPte);
            pte.setPfn(pfn);

            // ASM bit: carry into architectural ASM / ASMb field if you track it.
            const bool asmFlag = WriteReg::getAsm(rawItbPte);
            pte.setAsm(asmFlag);

            // Read permissions: U/S/E/K read enables.
            // Map them into your internal "protection byte" layout.
            const bool kre = WriteReg::getKre(rawItbPte);
            const bool ere = WriteReg::getEre(rawItbPte);
            const bool sre = WriteReg::getSre(rawItbPte);
            const bool ure = WriteReg::getUre(rawItbPte);

            pte.setReadPermissions(kre, ere, sre, ure);

            // Write permissions for ITB are normally irrelevant (I-stream),
            // but if your AlphaPTE tracks them, you can set them to 0 or
            // mirror kernel read as a conservative choice.
            pte.setWritePermissions(false, false, false, false);

            // ITB_PTE does not expose FOW / FOR for I-stream, so leave
            // fault-on-read/write execute bits as zero here. Execution
            // control is normally via FOE in the memory PTE.
            // pte.setFaultOnRead(false);
            // pte.setFaultOnWrite(false);

            // Mark PTE as valid if PFN is non-zero and OS would normally
            // treat it as a valid mapping. You can also require ASM here,
            // depending on how strictly you want to emulate IPR semantics.
            if (pfn != 0) {
                pte.setValid(true);
            }

            return pte;
        }

        /*
         * Encode a canonical AlphaPTE into an ITB_PTE read-format image.
         *
         * This is used when the guest executes:
         *     MFPR ITB_PTE, Rn
         *
         * The EV6 manual defines the ITB_PTE read layout using a packed
         * USEK field in bits [11:8] and PFN[52:32] plus ASM[4].
         *
         * Ev6_ItbPteRead_Register abstracts those details for us.
         */
        static inline quint64 toItbPteRead(const PTE& pte) noexcept
        {
            using ReadReg = Ev6_ItbPteRead_Register;

            quint64 raw = 0;

            // PFN goes into the same [52:32] slot for read-back.
            const PFNType pfn = pte.pfn();
            raw = ReadReg::setPfn(raw, pfn);

            // ASM bit: reflect current PTE ASMb state.
            raw = ReadReg::setAsm(raw, pte.getAsm());

            // Per-mode read permissions: map from canonical protection to USEK.
            bool kre, ere, sre, ure;
            pte.getReadPermissions(kre, ere, sre, ure);

            raw = ReadReg::setKre(raw, kre);
            raw = ReadReg::setEre(raw, ere);
            raw = ReadReg::setSre(raw, sre);
            raw = ReadReg::setUre(raw, ure);

            return raw;
        }
    };


