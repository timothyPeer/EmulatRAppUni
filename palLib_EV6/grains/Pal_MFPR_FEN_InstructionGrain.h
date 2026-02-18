#ifndef PAL_MFPR_FEN_INSTRUCTIONGRAIN_H
#define PAL_MFPR_FEN_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_FEN_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_FEN (Read FP Enable)
//
// Function code:
//   CALL_PAL 0x47
//
// Summary:
//   Reads whether floating-point operations are currently enabled. If FEN is
//   disabled, FP instructions should raise a fault.
//
// References:
//   - Alpha AXP System Reference Manual, FEN bit.
//   - Floating-point trap enabling semantics.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_FEN_InstructionGrain : public PalInstructionBase<0x0B>
{
public:
    Pal_MFPR_FEN_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_FEN"; }

	AXP_HOT void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMFPR_FEN(slot);
	}
};

REGISTER_GRAIN(Pal_MFPR_FEN_InstructionGrain);

#endif // PAL_MFPR_FEN_INSTRUCTIONGRAIN_H
