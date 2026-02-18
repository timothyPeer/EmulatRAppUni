#ifndef ONWRITE_DTB_PTE_COMMON_INL_H
#define ONWRITE_DTB_PTE_COMMON_INL_H

#include "AlphaCPU.h"
#include <QtGlobal>

// ============================================================================
// EV6 DTB_PTE0 / DTB_PTE1 write hook (common helper)
//
// References:
//   - DEC 21264 Hardware Reference Manual (HRM), Chapter 5
//     DTB_PTE0 / DTB_PTE1 / DTB_TAG0 / DTB_TAG1.
//   - Alpha System Architecture, Vol. I, section on translation buffers.
//
// Behavior:
//   1) Decode EV6 "DTB_PTE write format" into a canonical AlphaPTE using
//      Ev6_DtbPteAdapter::fromDtbPteWrite().
//   2) Update architectural IPR mirrors (dtb_pte0 / dtb_pte1).
//   3) Stage the canonical PTE via CPU side-effect hooks (stageDTB0PTE,
//      stageDTB1PTE, stageDTBPTE).
//   4) Optionally commit the PTE into the D-stream TLB via the global
//      Ev6TLBInterface facade (Ev6SiliconTLB_Singleton::interface()).
// ============================================================================
inline void onWrite_DTB_PTE_common(
    AlphaCPU* argCpu,
    quint64               oldValue,
    quint64               newValue,
    bool                  bank1
    )
{
    Q_UNUSED(oldValue)

    if (!argCpu) {
        return;
    }

    const quint8 cpuId = argCpu->cpuId();

    // Architectural IPR mirrors for DTB_TAG0/1 and DTB_PTE0/1
    auto& iprs = globalIPRBank()[cpuId];

    // ------------------------------------------------------------------------
    // 1) Decode EV6 DTB_PTE write image -> canonical AlphaPTE
    //
    // Ev6_DtbPteAdapter::fromDtbPteWrite() expects the raw DTB_PTE write image
    // (PFN in bits 52:32, ASM, URE/SRE/ERE/KRE, UWE/SWE/EWE/KWE, FOW/FOR).
    // See: alpha_pte_traits_ev6_dtb.h
    // ------------------------------------------------------------------------
    AlphaPTE pte = Ev6_DtbPteAdapter::fromDtbPteWrite(newValue);

    // ------------------------------------------------------------------------
    // 2) Update architectural DTB_PTEx mirror and select the matching TAGx
    // ------------------------------------------------------------------------
    const quint64 tag = bank1 ? iprs.dtb_tag1 : iprs.dtb_tag0;

    if (bank1) {
        iprs.dtb_pte1 = newValue;
    }
    else {
        iprs.dtb_pte0 = newValue;
    }

    // ------------------------------------------------------------------------
    // 3) Stage PTE into CPU side-effect pipeline
    //
    // These are your scoreboard hooks; they allow a later retirement step
    // to decide when to actually program the SPAM/TLB hardware.
    // ------------------------------------------------------------------------
    if (bank1) {
        argCpu->stageDTB1PTE(pte.raw);
    }
    else {
        argCpu->stageDTB0PTE(pte.raw);
    }
    argCpu->stageDTBPTE(pte.raw);

    // ------------------------------------------------------------------------
    // 4) Optional: eagerly commit to the D-stream TLB (Realm::D)
    //
    //   - VA is recovered from DTB_TAGx (low 44 bits).
    //   - ASN is taken from the active ASN for this CPU.
    //   - "Global" is derived from ASM (ASM == 0 -> global).
    //
    // If you prefer a pure "stage then commit" model, you can comment this
    // block out and let a later pipeline phase call insertDTB().
    // ------------------------------------------------------------------------
    const VAType va =
        Ev6_DtbPteAdapter::decodeVAFromDTBTag(tag);  // low 44 bits are VA

    const ASNType asn =
        static_cast<quint8>(getASN_Active(cpuId) & 0xFFu);  // active ASN

    const bool isGlobal = (pte.bitASM() == 0);  // ASM==0 => global mapping

    // TLB facade around silicon SPAM/TLB hardware
    auto& tlb = globalEv6Silicon();
    auto& spam = tlb.spam();

    // Commit this entry into the D-stream TLB (DTB realm)
    spam.insertEntry(cpuId,Realm::D,0, va, pte.pfn(),asn,  isGlobal);

    // If you later add statistics or round-robin replacement:
    // tlb.bumpDTBInsertPointer(cpuId);
}
#endif // ONWRITE_DTB_PTE_COMMON_INL_H
