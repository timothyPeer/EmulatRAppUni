#ifndef PAL_TBIA_INSTRUCTIONGRAIN_H
#define PAL_TBIA_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_TBIA_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: TBIA (Translation Buffer Invalidate All)
//
// NOTE: PalFunc value below is a placeholder. You should replace 0xFFF0
//       with the correct CALL_PAL TBIA function code for your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF0  // TODO: replace with real TBIA PAL function code
//
// Summary:
//   Invalidates all TLB entries for the calling CPU. On real hardware, this
//   is often used during heavy context changes or global mapping updates.
//
// References:
//   - Alpha AXP System Reference Manual, CALL_PAL and TB maintenance.
//   - Implementation-specific PALcode docs (EV5/EV6 TBIA).
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

// Forward declarations for helper integration
class CPUStateIPRInterface;
void executeTBIA(CPUStateIPRInterface* cpuState);

class Pal_TBIA_InstructionGrain : public PalInstructionBase<0xFFF0> // TODO: fix PalFunc
{
public:
    Pal_TBIA_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "TBIA"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        // Bridge AlphaProcessorContext to the existing TBIA helper which operates
        // on CPUStateIPRInterface.
        CPUStateIPRInterface* iface = static_cast<CPUStateIPRInterface*>(&cpu);
        executeTBIA(iface);

        // TODO:
        //   1) Confirm that executeTBIA performs:
        //        - ITB + DTB invalidation for this cpuId.
        //        - Correct interaction with your Ev6SiliconTLB_Singleton.
        //   2) If you later add SMP-wide TBIA semantics (broadcast invalidation),
        //      ensure the helper and/or this grain triggers appropriate IPIs.
    }
};

REGISTER_GRAIN(Pal_TBIA_InstructionGrain);

#endif // PAL_TBIA_INSTRUCTIONGRAIN_H
