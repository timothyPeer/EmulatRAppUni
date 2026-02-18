#ifndef PAL_HALT_INSTRUCTIONGRAIN_H
#define PAL_HALT_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_HALT_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: HALT
//
// Function code:
//   CALL_PAL 0x00  (HALT)
//
// Architectural summary:
//   HALT stops execution of the current processor and enters a halted state.
//   The platform firmware or console may be notified and may provide a
//   restart mechanism.
//
// References:
//   - Alpha AXP System Reference Manual, Version 6 (1994)
//     PALcode chapter, CALL_PAL HALT description.
//   - Alpha 21064 / 21164 Hardware Reference Manuals, HALT behavior.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

// ============================================================================
// HALT - Halt Processor (0x00 - Privileged)
// ============================================================================
class Pal_HALT_InstructionGrain : public PalInstructionBase<0x00>
{
public:
	Pal_HALT_InstructionGrain() noexcept = default;
	QString mnemonic() const override { return "HALT"; }

protected:
	void execute(PipelineSlot& slot,
		AlphaProcessorContext* ctx) const override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeHALT(slot);
	}
};
REGISTER_GRAIN(Pal_HALT_InstructionGrain);
#endif // PAL_HALT_INSTRUCTIONGRAIN_H

// TODO: HALT implementation details
//   1) Mark this vCPU as halted in AlphaProcessorContext state.
//      - Define and set a cpu.haltRequested or cpu.setHalted(true).
//   2) Ensure that the main execution loop checks the halted state
//      and stops issuing instructions for this CPU.
//   3) Optionally log the HALT event, including PC, PS, and HWPCB
//      for debugging and console diagnostics.
//   4) Notify the EmulatorManager or system console layer so that
//      a user-visible "CPU halted" indication can be shown.
//   5) Ensure that any pending traps or interrupts are quiesced or
//      handled according to your emulator's shutdown semantics.
//
// Source:
//   - Alpha AXP System Reference Manual, PALcode HALT definition,
//     CALL_PAL 0x00.