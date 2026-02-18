#ifndef PAL_GENTRAP_INSTRUCTIONGRAIN_H
#define PAL_GENTRAP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_GENTRAP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: GENTRAP (General Trap)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFE3 with the
//       correct CALL_PAL function code for GENTRAP.
//
/
// Summary:
//   Triggers a software-requested trap into the operating system. The
//   trap code is typically passed in an integer register, and PALcode
//   constructs an exception frame and vectors to the OS trap handler.
//
// References:
//   - Alpha AXP System Reference Manual, CALL_PAL GENTRAP.
//   - OS PALcode descriptions of trap codes and exception vectors.
// ============================================================================

#include "PalInstructionBase.h"
#include <QtGlobal>
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"

// ============================================================================
// GENTRAP - Generic Trap (0xAA)
// ============================================================================
class Pal_GENTRAP_InstructionGrain : public PalInstructionBase<0xAA>
{
public:
	Pal_GENTRAP_InstructionGrain() noexcept = default;
	QString mnemonic() const override { return "GENTRAP"; }

protected:
	void execute(PipelineSlot& slot,
		AlphaProcessorContext* ctx) const override
	{
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
		ctx.getMBox()->executeGENTRAP(slot);
	}
};
REGISTER_GRAIN(Pal_GENTRAP_InstructionGrain);

#endif // PAL_GENTRAP_INSTRUCTIONGRAIN_H


// TODO:
	//   1) Define a mapping from trapArg to your internal TrapCode enum
	//      or OS-specific trap numbers.
	//   2) Construct an exception frame:
	//        - Save PC, PS, GP, argument registers, and any required
	//          floating-point state as defined by your PAL ABI.
	//   3) Use SCBB to locate the appropriate trap vector for the
	//      GENTRAP event and transfer control there.
	//   4) Mark the CPU as "in trap" or "in exception" so REI/RETSYS can
	//      unwind it correctly.
	//   5) Provide detailed logging of trapArg, PC, PS, and mode.
	//
	// Example:
	//   cpu.raiseSoftwareTrap(trapArg);