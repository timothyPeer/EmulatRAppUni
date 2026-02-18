#ifndef PAL_MTPR_ASN_INSTRUCTIONGRAIN_H
#define PAL_MTPR_ASN_INSTRUCTIONGRAIN_H

// ============================================================================
// MTPR_ASN - Move To Processor Register (ASN)
// CALL_PAL 0x07 (Privileged)
//
// Grain Responsibility: Route to IBox
// IBox Responsibility: Write ASN to IPR, coordinate side effects via MBox
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_ASN_InstructionGrain : public PalInstructionBase<0x0??>
{
public:
	Pal_MTPR_ASN_InstructionGrain() noexcept = default;

	QString mnemonic() const override
	{
		return "MTPR_ASN";
	}
	AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeMTPR_ASN(slot);
	}
};

REGISTER_GRAIN(Pal_MTPR_ASN_InstructionGrain);

#endif // PAL_MTPR_ASN_INSTRUCTIONGRAIN_H
