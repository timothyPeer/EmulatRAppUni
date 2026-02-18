#ifndef PAL_MFPR_AST_INSTRUCTIONGRAIN_H
#define PAL_MFPR_AST_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_AST_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_AST
//
// Function code:
//   CALL_PAL 0x08
//
// Summary:
//   Reads the AST level from the AST processor register. AST (Asynchronous
//   System Trap) level influences interrupt and trap behavior.
//
// References:
//   - Alpha AXP System Reference Manual, AST register rules.
//   - OS PALcode specifications (VMS, OSF).
// ============================================================================
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_AST_InstructionGrain : public PalInstructionBase<0x000??>
{
public:
    Pal_MFPR_AST_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_AST"; }

	AXP_HOT void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMFPR_AST(slot);
	}
};

REGISTER_GRAIN(Pal_MFPR_AST_InstructionGrain);

#endif // PAL_MFPR_AST_INSTRUCTIONGRAIN_H
