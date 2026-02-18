#ifndef PAL_MFPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H
#define PAL_MFPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_DC_TEST_CTL_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_DC_TEST_CTL
//
// Function code:
//   CALL_PAL 0x58  (implementation-specific, EV6 cache test)
//
// Summary:
//   Reads the Dcache Test Control register. Mainly useful for diagnostic and
//   hardware bring-up tools.
//
// References:
//   - Alpha EV6 Hardware Reference Manual, DC_TEST_CTL.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_DC_TEST_CTL_InstructionGrain : public PalInstructionBase<0x58>
{
public:
    Pal_MFPR_DC_TEST_CTL_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_DC_TEST_CTL"; }

protected:
protected:
	AXP_HOT void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->dispatchCallPal(slot);
	}

};

REGISTER_GRAIN(Pal_MFPR_DC_TEST_CTL_InstructionGrain);

#endif // PAL_MFPR_DC_TEST_CTL_INSTRUCTIONGRAIN_H
