#ifndef PAL_TBISI_INSTRUCTIONGRAIN_H
#define PAL_TBISI_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_TBISI_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: TBISI (Translation Buffer Invalidate Single, Instruction)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFF4 with the
//       correct TBISI PAL function code for your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF4  // TODO: replace with real TBISI PAL function code
//
// Summary:
//   Invalidates an ITB entry for a single virtual address, for the
//   current ASN or as defined by PALcode.
//
// References:
//   - Alpha AXP System Reference Manual, TBISI.
//   - Implementation notes for instruction-side TLB invalidation.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class CPUStateIPRInterface;
void executeTBISI(CPUStateIPRInterface* cpuState);

class Pal_TBISI_InstructionGrain : public PalInstructionBase<0xFFF4> // TODO: fix PalFunc
{
public:
    Pal_TBISI_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "TBISI"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        CPUStateIPRInterface* iface = static_cast<CPUStateIPRInterface*>(&cpu);
        executeTBISI(iface);

        // TODO:
        //   1) Confirm that executeTBISI:
        //        - Reads iprs.va and ASN.
        //        - Invalidates only ITB entries (Realm::I).
        //   2) Ensure instruction cache and decode caches are consistent if
        //      you are modeling them explicitly.
    }
};

REGISTER_GRAIN(Pal_TBISI_InstructionGrain);

#endif // PAL_TBISI_INSTRUCTIONGRAIN_H
