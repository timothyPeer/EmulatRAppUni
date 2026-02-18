#ifndef PAL_HW_MTPR_INSTRUCTIONGRAIN_H
#define PAL_HW_MTPR_INSTRUCTIONGRAIN_H

// ============================================================================
// HW_MTPR - Hardware Move TO Processor Register
// ============================================================================
// Opcode: 0x1D, Function: 0x00
// PAL-format instruction (not CALL_PAL)
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

class Pal_HW_MTPR_InstructionGrain : public PalInstructionBase<0x1D>
{
public:
	Pal_HW_MTPR_InstructionGrain() noexcept = default;

	AXP_HOT AXP_FLATTEN
		void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		ctx.getMBox()->executeHW_MTPR(slot);
	}

	quint8 opcode() const override { return 0x1D; }
	quint16 functionCode() const override { return 0x0; }
	QString mnemonic() const override { return "HW_MTPR"; }
	GrainType grainType() const override { return GrainType::Pal; }

	GrainPlatform platform() const override
	{
		return GrainPlatform::Pal_Internal;
	}

	ExecutionBox executionBox() const override
	{
		return ExecutionBox::MBox;
	}
};

REGISTER_GRAIN(Pal_HW_MTPR_InstructionGrain);

#endif // PAL_HW_MTPR_INSTRUCTIONGRAIN_H