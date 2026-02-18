#ifndef PAL_BPT_INSTRUCTIONGRAIN_H
#define PAL_BPT_INSTRUCTIONGRAIN_H

// ============================================================================
// BPT - Breakpoint Trap
// CALL_PAL 0x80 (Unprivileged - note: 0x41 in your file may be incorrect)
//
// Per Alpha Architecture:
// "The BPT instruction is provided for program debugging. It switches to 
//  kernel mode and pushes R2..R7, the updated PC, and PS on the kernel 
//  stack. It then dispatches to the address in the Breakpoint SCB vector."
//
// Grain Responsibility: Route to IBox
// IBox Responsibility: 
//   - Switch to kernel mode
//   - Push exception frame (R2-R7, PC, PS) to kernel stack
//   - Dispatch to breakpoint vector in SCB
//   - Coordinate PALmode/PC
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

class Pal_BPT_InstructionGrain : public PalInstructionBase<0x80>
{
public:
	Pal_BPT_InstructionGrain() noexcept = default;

	QString mnemonic() const override { return "BPT"; }

protected:
	void execute(PipelineSlot& slot,
		AlphaProcessorContext* ctx) const override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeBPT_VMS(slot);
	}
};

REGISTER_GRAIN(Pal_BPT_InstructionGrain);

#endif // PAL_BPT_INSTRUCTIONGRAIN_H