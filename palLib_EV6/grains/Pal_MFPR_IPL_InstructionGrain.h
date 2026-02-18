#ifndef PAL_MFPR_IPLR_INSTRUCTIONGRAIN_H
#define PAL_MFPR_IPLR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_IPL_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_IPL
//
// Function code:
//   CALL_PAL 0x00E
//
// Summary:
//   Reads the Interrupt Priority Level Register (IPLR). This defines the
//   current interrupt mask level of the processor.
//
// References:
//   - Alpha AXP System Reference Manual, IPLR register.
//   - Interrupt handling and processor priority mechanisms.
// ============================================================================
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_IPL_InstructionGrain : public PalInstructionBase<0x00E>
{
public:
    Pal_MFPR_IPL_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_IPL"; }

	AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMFPR_IPL(slot);
	}
};

REGISTER_GRAIN(Pal_MFPR_IPL_InstructionGrain);

#endif // PAL_MFPR_IPLR_INSTRUCTIONGRAIN_H
