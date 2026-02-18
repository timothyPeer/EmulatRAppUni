#ifndef PAL_HW_LD_INSTRUCTIONGRAIN_H
#define PAL_HW_LD_INSTRUCTIONGRAIN_H

// ============================================================================
// HW_LD - Hardware Load (PAL internal instruction)
// ============================================================================
// This is a PAL-privileged memory load instruction.
// Opcode: 0x1B (shared with other PAL instructions)
// Function: 0x?? (specific to HW_LD)
// Platform: PAL_INTERNAL
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

// ============================================================================
// HW_LD Grain - PAL Hardware Load
// ============================================================================

class Pal_HW_LD_InstructionGrain : public PalInstructionBase<0x1B>  
{
public:
	Pal_HW_LD_InstructionGrain() noexcept = default;

	// ========================================================================
	// Opcode - PAL instructions use 0x1B
	// ========================================================================
	AXP_FLATTEN
		quint8 opcode() const override
	{
		return 0x1B;  // PAL format opcode
	}

	// ========================================================================
	// Function Code - Identifies HW_LD within PAL instructions
	// ========================================================================
	AXP_FLATTEN
		quint16 functionCode() const override
	{
		// Check Alpha PAL specification for HW_LD function code
		// Placeholder - replace with actual function code
		return 0x00;  // TODO: Replace with correct HW_LD function code
	}

	// ========================================================================
	// Platform - PAL Internal
	// ========================================================================
	AXP_FLATTEN
		GrainPlatform platform() const override
	{
		return GrainPlatform::Alpha;  // <- Fixed capitalization
	}

	// ========================================================================
	// Mnemonic
	// ========================================================================
	QString mnemonic() const override
	{
		return "HW_LD";  // <- Fixed name (was BUGCHK)
	}

	inline ExecutionBox	ExecutionBox_Grain() { box = ExecutionBox::MBox; }
	// ========================================================================
	// Execute - Route to MBox
	// ========================================================================
	AXP_HOT void  execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		MBox* mbox = ctx.getMBox();
		Q_ASSERT(mbox != nullptr);

		// HW_LD is a PAL-privileged load
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		mbox->executeHW_LD(slot);
	}
};

// ============================================================================
// Registration
// ============================================================================
REGISTER_GRAIN(Pal_HW_LD_InstructionGrain);

#endif // PAL_HW_LD_INSTRUCTIONGRAIN_H