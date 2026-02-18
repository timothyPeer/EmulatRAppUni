#ifndef PAL_MTPR_FEN_INSTRUCTIONGRAIN_H
#define PAL_MTPR_FEN_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_FEN_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_FEN (Write FP Enable)
//
// Function code:
//   CALL_PAL 0x48
//
// Summary:
//   Enables or disables floating-point operations. When FEN=0, any FP
//   instruction triggers a fault.
//
// References:
//   - Alpha AXP System Reference Manual, FEN control.
//   - FP trap behavior, kernel-mode FP enable.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_FEN_InstructionGrain : public PalInstructionBase<0x00B>
{
public:
    Pal_MTPR_FEN_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_FEN"; }

	AXP_HOT void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMTPR_FEN(slot);
	}
};

REGISTER_GRAIN(Pal_MTPR_FEN_InstructionGrain);

#endif // PAL_MTPR_FEN_INSTRUCTIONGRAIN_H
