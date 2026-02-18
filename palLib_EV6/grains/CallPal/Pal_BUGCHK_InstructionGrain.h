#ifndef Pal_BUGCHK_InstructionGrain_h__
#define Pal_BUGCHK_InstructionGrain_h__

// ============================================================================
// Common CALL_PAL Grains - Simple Routing Pattern
// All grains just route to IBox for implementation
// ============================================================================


#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

// ============================================================================
// BUGCHK - Bug Check Trap (0x81)
// ============================================================================
class Pal_BUGCHK_InstructionGrain : public PalInstructionBase<0x81>
{
public:
	Pal_BUGCHK_InstructionGrain() noexcept = default;
	QString mnemonic() const override { return "BUGCHK"; }

/*protected:*/
	void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeBUGCHK(slot);
	}
};
REGISTER_GRAIN(Pal_BUGCHK_InstructionGrain);

        // TODO:
        //   1) Schedule a severe trap or panic signal in the emulator.
        //   2) Transfer control to the system's bugcheck handler.
        //   3) Log the condition with full register dump.
        //   4) This is generally unrecoverable at the PAL layer.
        //
        // Example:
        //   cpu.raiseTrap(TrapCode::BUGCHECK);

#endif // Pal_BUGCHK_InstructionGrain_h__
