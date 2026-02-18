#ifndef PAL_IMB_INSTRUCTIONGRAIN_H
#define PAL_IMB_INSTRUCTIONGRAIN_H
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

// ============================================================================
// IMB - Instruction Memory Barrier (0x86)
// ============================================================================
class Pal_IMB_InstructionGrain : public PalInstructionBase<0x86>
{
public:
    Pal_IMB_InstructionGrain() noexcept = default;
    QString mnemonic() const override { return "IMB"; }

protected:
  AXP_HOT  void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
    {
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getPalBox()->executeIMB(slot);
    }
};
REGISTER_GRAIN(Pal_IMB_InstructionGrain);
#endif // PAL_IMB_INSTRUCTIONGRAIN_H
