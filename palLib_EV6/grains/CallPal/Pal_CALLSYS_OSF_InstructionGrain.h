#ifndef PAL_CALLSYS_OSF_INSTRUCTIONGRAIN_H
#define PAL_CALLSYS_OSF_INSTRUCTIONGRAIN_H

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
// CALLSYS - System Call (0x83)
// ============================================================================
class Pal_CALLSYS_OSF_InstructionGrain : public PalInstructionBase<0x83>
{
public:
    Pal_CALLSYS_OSF_InstructionGrain() noexcept = default;
    QString mnemonic() const override { return "CALLSYS"; }
    inline GrainPlatform platform() const override
    {
        return GrainPlatform::Unix;
    }

protected:
    void execute(PipelineSlot& slot,
                    AlphaProcessorContext* ctx) const override
    {
		// Route to MBox -
        // MBox complete side effects
        // Update pipeline stage_WB as slot.needsWriteback = false;
        ctx.getMBox()->executeCALLSYS(slot);
    }
};
REGISTER_GRAIN(Pal_CALLSYS_OSF_InstructionGrain);
#endif // Pal_BUGCHK_InstructionGrain_h__
