#ifndef PAL_TBIS_INSTRUCTIONGRAIN_H
#define PAL_TBIS_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_TBIS_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: TBIS (Translation Buffer Invalidate Single)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFF2 with the
//       correct TBIS PAL function code for your PAL mapping.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF2  // TODO: replace with real TBIS PAL function code
//
// Summary:
//   Invalidates TLB entries corresponding to a single virtual address
//   (and ASN), typically for both ITB and DTB.
//
// References:
//   - Alpha AXP System Reference Manual, TBIS.
//   - EV6 PALcode descriptions for single-entry TLB invalidations.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class CPUStateIPRInterface;
void executeTBIS(CPUStateIPRInterface* cpuState);

class Pal_TBIS_InstructionGrain : public PalInstructionBase<0xFFF2> // TODO: fix PalFunc
{
public:
    Pal_TBIS_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "TBIS"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        CPUStateIPRInterface* iface = static_cast<CPUStateIPRInterface*>(&cpu);
        executeTBIS(iface);

        // TODO:
        //   1) Verify executeTBIS:
        //        - Uses iprs.va and current ASN.
        //        - Invalidates ITB and DTB entries via tlb.tbis(...).
        //   2) Ensure that your SPAM / Ev6 TLB sharding logic is correctly
        //      updated (bucket invalidation vs lazy invalidation).
    }
};

REGISTER_GRAIN(Pal_TBIS_InstructionGrain);

#endif // PAL_TBIS_INSTRUCTIONGRAIN_H
