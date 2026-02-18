#ifndef PAL_RDWRITEUNIQUE_OSF_INSTRUCTIONGRAIN_H
#define PAL_RDWRITEUNIQUE_OSF_INSTRUCTIONGRAIN_H
#include <QtGlobal>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/DecodedInstruction.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../MBoxLib_EV6/MBoxBase.h"

// ============================================================================
// WRUNIQUE - Write Unique (Thread Pointer) (0x9F)
// ============================================================================
class Pal_WRUNIQUE_InstructionGrain : public PalInstructionBase<0x9F>
{
public:
    Pal_WRUNIQUE_InstructionGrain() noexcept = default;
    QString mnemonic() const override { return "WRUNIQUE"; }
	inline GrainPlatform platform() const override
	{
		return GrainPlatform::Unix;
	}
protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext* ctx) const override
    {
        ctx.getMBox()->executeWRUNIQUE(di);
    }
};
REGISTER_GRAIN(Pal_WRUNIQUE_InstructionGrain);

#endif // PAL_RDWRITEUNIQUE_OSF_INSTRUCTIONGRAIN_H
