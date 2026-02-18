#ifndef PAL_MFPR_ASN_INSTRUCTIONGRAIN_H
#define PAL_MFPR_ASN_INSTRUCTIONGRAIN_H

// ============================================================================
// MFPR_ASN - Move From Processor Register (ASN)
// CALL_PAL 0x06 (Privileged)
//
// Grain Responsibility: Route to IBox
// IBox Responsibility: Read ASN from IPR, write to R0, coordinate PALmode/PC
// ============================================================================

#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "PalInstructionBase.h"
class Pal_MFPR_ASN_InstructionGrain : public PalInstructionBase<0x06>
{
public:
	Pal_MFPR_ASN_InstructionGrain() noexcept = default;

	QString mnemonic() const override
	{
		return "MFPR_ASN";
	}
	AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot,	AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMTPR_ASN(slot);
	}
};

REGISTER_GRAIN(Pal_MFPR_ASN_InstructionGrain);

#endif // PAL_MFPR_ASN_INSTRUCTIONGRAIN_H
