#ifndef PAL_MTPR_IPLR_INSTRUCTIONGRAIN_H
#define PAL_MTPR_IPLR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_IPLR_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_IPLR
//
// Function code:
//   CALL_PAL 0x39
//
// Summary:
//   Writes the Interrupt Priority Level Register (IPLR). Raising IPL masks
//   lower-priority interrupts; lowering IPL may expose pending ones.
//
// References:
//   - Alpha AXP System Reference Manual, IPLR.
//   - Interrupt delivery, pending interrupt rules.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_IPL_InstructionGrain : public PalInstructionBase<0x0F>
{
public:
	Pal_MTPR_IPL_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_IPL"; }

protected:
	AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMTPR_IPL(slot);
	}
};

REGISTER_GRAIN(Pal_MTPR_IPL_InstructionGrain);

#endif // PAL_MTPR_IPLR_INSTRUCTIONGRAIN_H
