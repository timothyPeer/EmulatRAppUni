#ifndef PAL_HW_MFPR_INSTRUCTIONGRAIN_H
#define PAL_HW_MFPR_INSTRUCTIONGRAIN_H

// ============================================================================
// HW_MFPR - Hardware Move FROM Processor Register
// ============================================================================
// Opcode: 0x19, Function: 0x00
// PAL-format instruction (not CALL_PAL)
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

class Pal_HW_MFPR_InstructionGrain : public PalInstructionBase<0x19>
{
public:
	Pal_HW_MFPR_InstructionGrain() noexcept = default;

	AXP_HOT AXP_FLATTEN
		void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		ctx.getMBox()->executeHW_MFPR(slot);
	}

	quint8 opcode() const override { return 0x19; }
	quint16 functionCode() const override { return 0x0; }
	QString mnemonic() const override { return "HW_MFPR"; }
	GrainType grainType() const override { return GrainType::Pal; }

	GrainPlatform platform() const override
	{
		return GrainPlatform::Alpha;
	}

	ExecutionBox executionBox() const override
	{
		return ExecutionBox::MBox;
	}
};

REGISTER_GRAIN(Pal_HW_MFPR_InstructionGrain);  // 

#endif // PAL_HW_MFPR_INSTRUCTIONGRAIN_H  // 