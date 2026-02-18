#ifndef PAL_TBISD_INSTRUCTIONGRAIN_H
#define PAL_TBISD_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_TBISD_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: TBISD (Translation Buffer Invalidate Single, Data)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFF3 with the
//       correct TBISD PAL function code as defined by your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF3  // TODO: replace with real TBISD PAL function code
//
// Summary:
//   Invalidates a single DTB entry corresponding to a virtual address
//   in the data TLB realm.
//
// References:
//   - Alpha AXP System Reference Manual, TBISD.
//   - EVx PALcode documentation for selective DTB invalidation.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class CPUStateIPRInterface;
// NOTE: The provided TBISD helper header currently has a naming issue,
// but we declare what we intend to call here for clarity.
void executeTBISD(CPUStateIPRInterface* cpuState);

class Pal_TBISD_InstructionGrain : public PalInstructionBase<0xFFF3> // TODO: fix PalFunc
{
public:
    Pal_TBISD_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "TBISD"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        CPUStateIPRInterface* iface = static_cast<CPUStateIPRInterface*>(&cpu);
        executeTBISD(iface);

        // TODO:
        //   1) Align the helper implementation:
        //        - Current file executeTBISD.h appears to define executeTBISI.
        //        - You will likely rename or split helpers to match TBISD vs TBISI.
        //   2) Ensure only DTB entries are invalidated (Realm::D).
        //   3) Integrate with your SPAMShardManager for per-realm invalidations.
    }
};

REGISTER_GRAIN(Pal_TBISD_InstructionGrain);

#endif // PAL_TBISD_INSTRUCTIONGRAIN_H
