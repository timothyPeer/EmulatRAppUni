#ifndef PAL_RDUNIQUE_OSF_NSTRUCTIONGRAIN_H
#define PAL_RDUNIQUE_OSF_NSTRUCTIONGRAIN_H
#include <QtGlobal>
#include "../coreLib//Axp_Attributes_core.h"
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/DecodedInstruction.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../MBoxLib_EV6/MBoxBase.h"

// ============================================================================
// RDUNIQUE - Read Unique (Thread Pointer) (0x9E)
// ============================================================================
class Pal_RDUNIQUE_OSF_InstructionGrain : public PalInstructionBase<0x9E>
{
public:
    Pal_RDUNIQUE_OSF_InstructionGrain() noexcept = default;
    QString mnemonic() const override { return "RDUNIQUE"; }

	inline GrainPlatform platform() const override
	{
		return GrainPlatform::Unix;
	}

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext* ctx) const override
    {
        ctx.getMBox()->executeRDUNIQUE(di);
    }
};
REGISTER_GRAIN(Pal_RDUNIQUE_OSF_InstructionGrain);

#endif // PAL_RDUNIQUE_OSF_NSTRUCTIONGRAIN_H
