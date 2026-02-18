#ifndef PAL_HW_REI_INSTRUCTIONGRAIN_H
#define PAL_HW_REI_INSTRUCTIONGRAIN_H
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

class Pal_HW_RET_InstructionGrain : public PalInstructionBase<0x1E>
{
protected:

public:
	Pal_HW_RET_InstructionGrain() noexcept = default;
	AXP_HOT	void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept
	{
		// HW_MFPR is a PAL-privileged load
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeHW_REI(slot);
	}


	quint16 functionCode() const override
	{
		return 0x0;
	}


	QString mnemonic() const override
	{
		return "HW_RET";
	}


	quint8 opcode() const override
	{
		return 0x1E;
	}


	GrainType grainType() const override
	{
		return GrainType::Pal;
	}

	inline GrainPlatform platform() const override
	{
		return GrainPlatform::Alpha;
	}

	// Set the execution box
	  // Set the execution box
	inline ExecutionBox	ExecutionBox_Grain() { box = ExecutionBox::MBox; }

};

#endif // PAL_HW_REI_INSTRUCTIONGRAIN_H
