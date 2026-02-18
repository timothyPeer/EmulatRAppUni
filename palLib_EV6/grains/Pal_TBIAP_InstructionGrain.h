#ifndef PAL_TBIAP_INSTRUCTIONGRAIN_H
#define PAL_TBIAP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_TBIAP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: TBIAP (Translation Buffer Invalidate All Process)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFF1 with the
//       proper PAL function code for TBIAP in your chosen PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF1  // TODO: replace with real TBIAP PAL function code
//
// Summary:
//   Invalidates all TLB entries for all processes (often an alias for
//   "invalidate entire TLB" plus ASN epoch changes).
//
// References:
//   - Alpha AXP System Reference Manual, CALL_PAL TBI* functions.
//   - Implementation PALcode docs (OSF/VMS) describing TBIAP.
// ============================================================================

#include "PalInstructionBase.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_TBIAP_InstructionGrain : public PalInstructionBase<0xFFF1> // TODO: fix PalFunc
{
public:
    Pal_TBIAP_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "TBIAP"; }

protected:
    void executePAL(PipelineSlot& di,
        AlphaProcessorContext* ctx) const override
    {
        Q_UNUSED(di);

        m_mbox->executeTBIAP(iface);

        // TODO:
        //   1) Confirm that executeTBIAP issues a full TLB invalidation
        //      via Ev6SiliconTLB_Singleton::interface().tbiAll().
        //   2) Decide whether ASN generation or global epoch bumping is
        //      required for your model and add it to the helper if needed.
    }
};

REGISTER_GRAIN(Pal_TBIAP_InstructionGrain);

#endif // PAL_TBIAP_INSTRUCTIONGRAIN_H
