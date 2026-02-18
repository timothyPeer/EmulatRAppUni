#ifndef PAL_DRAINA_INSTRUCTIONGRAIN_H
#define PAL_DRAINA_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_DRAINA_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: DRAINA
//
// Function code:
//   CALL_PAL 0x02  (DRAINA)
//
// Architectural summary:
//   DRAINA drains the write buffer and ensures that all prior memory writes
//   are visible to the system before subsequent memory operations proceed.
//   It is typically used in conjunction with PAL and low-level OS code to
//   enforce strong ordering for certain sequences.
//
// References:
//   - Alpha AXP System Reference Manual, Version 6 (1994),
//     PALcode chapter, CALL_PAL DRAINA.
//   - Alpha Hardware Reference Manuals, write buffer and drain semantics.
// ============================================================================
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

// ============================================================================
// DRAINA - Drain Write Buffers (0x02 - Privileged)
// ============================================================================
class Pal_DRAINA_InstructionGrain : public PalInstructionBase<0x02>
{
public:
	Pal_DRAINA_InstructionGrain() noexcept = default;
	QString mnemonic() const override { return "DRAINA"; }

protected:
	void execute(PipelineSlot& slot,
		AlphaProcessorContext* ctx) const override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeDRAINA(slot);
	}
};
REGISTER_GRAIN(Pal_DRAINA_InstructionGrain);

#endif // PAL_DRAINA_INSTRUCTIONGRAIN_H



// TODO: DRAINA implementation details
   //   1) Integrate with your write buffer or store queue model
   //      if you have one. For example:
   //        - cpu.drainWriteBuffer()
   //        - memorySubsystem.drainStores()
   //   2) Ensure that all pending writes are committed to SafeMemory
   //      before this PAL returns.
   //   3) Consider interaction with SMP:
   //        - Ensure visibility of drained writes to other CPUs,
   //          possibly in cooperation with MB/WMB semantics.
   //   4) If you do not model a store buffer, you may treat DRAINA
   //      as a full memory barrier primitive and reuse existing
   //      memory barrier helper logic.
   //   5) Provide debug tracing for DRAINA events, including the PC
   //      where it was issued.
   //
   // Source:
   //   - Alpha AXP System Reference Manual, PALcode DRAINA definition,
   //     CALL_PAL 0x02.