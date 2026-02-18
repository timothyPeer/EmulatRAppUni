#ifndef Z:_EMULATRAPPUNI_PALLIB_EV6_GRAINS_PAL_CALL_PAL_INSTRUCTIONGRAIN_H
#define Z:_EMULATRAPPUNI_PALLIB_EV6_GRAINS_PAL_CALL_PAL_INSTRUCTIONGRAIN_H

#include "../../faultLib/PendingEvent_Refined.h"
#include "../../machineLib/PipeLineSlot.h"
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/grains/IntMiscControl_InstructionGrain.h"
#include "PalBoxLib/PalBoxBase.h"


class Pal_CALL_PAL_InstructionGrain : public PalInstructionBase<0x00>
{
public:
	Pal_CALL_PAL_InstructionGrain() = default;
	QString mnemonic() const override { return "CALL_PAL"; }

	void execute(PipelineSlot& slot) const noexcept override
	{
		// 1. Extract PAL function from instruction
		const quint8 palFunc = slot.palDecoded.palFunction;

		// 2. Route to PBox (architectural PAL entry)
		slot.getPBox()->executeCallPal(slot);

		// 3. No normal writeback
		slot.needsWriteback = false;
	}
};


#endif
