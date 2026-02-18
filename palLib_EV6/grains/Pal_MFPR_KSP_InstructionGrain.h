#ifndef PAL_MFPR_KSP_INSTRUCTIONGRAIN_H
#define PAL_MFPR_KSP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_KSP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_KSP
//
// Function code:
//   CALL_PAL 0x30
//
// Summary:
//   Reads Kernel Stack Pointer (KSP) from IPR storage. Required during
//   kernel entry, interrupt/trap setup, and context switching.
//
// References:
//   - Alpha AXP System Reference Manual, Kernel Stack Pointer rules.
//   - VMS/OSF PALcode documentation.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

class Pal_MFPR_KSP_InstructionGrain : public PalInstructionBase<0x30>
{
public:
    Pal_MFPR_KSP_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_KSP"; }

protected:

	AXP_HOT void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->dispatchCallPal(slot);
	}
};

REGISTER_GRAIN(Pal_MFPR_KSP_InstructionGrain);

#endif // PAL_MFPR_KSP_INSTRUCTIONGRAIN_H
