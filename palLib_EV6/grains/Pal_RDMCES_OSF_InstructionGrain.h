#ifndef PAL_RDMCES_INSTRUCTIONGRAIN_H
#define PAL_RDMCES_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_RDMCES_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode (OSF/Tru64 specific): RDMCES
//
// PURPOSE:
//   Returns the current Machine Check Error Summary (MCES) value to the caller.
//   RDMCES is used by OSF kernels to examine current hardware error state.
//
// NOTE:
//   The PAL function code below (0xFFE8) is a placeholder. You will replace it
//   during the opcode/function audit with the correct OSF PAL function number.
//
// Function code (placeholder):
//   CALL_PAL 0xFFE8    // TODO: replace with correct OSF RDMCES PAL function code
//
// REFERENCES:
//   - Alpha Architecture Reference Manual (AARM), Machine Check Section.
//   - Tru64/OSF PALcode specifications.
//   - MCES layout and error bits (EV4/EV5/EV6 specific).
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/InstructionGrain_core.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_RDMCES_InstructionGrain : public PalInstructionBase<0x10> // TODO: fix PalFunc
{
public:
    Pal_RDMCES_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "RDMCES";
    }

	inline GrainPlatform platform() const override
	{
		return GrainPlatform::Unix;   
	}
protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& argCpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(argCpu);
        const int id     = argCpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add accessor:
        //        quint64 mces = cpu.readIPR_MCES_Active(id);
        //
        //   2) Return MCES value via PAL ABI return register (often R0/v0):
        //        cpu.writeIntReg(retReg, mces);
        //
        //   3) Log MCES reads for debugging machine check handling.
        //
        //   4) Confirm OSF semantics: some implementations mask privilege bits
        //      or require specific mode restrictions before RDMCES is allowed.
        //
        // Example:
        //   quint64 mces = cpu.readIPR_MCES_Active(id);
        //   cpu.writeIntReg(0, mces); // Example return register, adjust as needed

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_RDMCES_InstructionGrain);

#endif // PAL_RDMCES_INSTRUCTIONGRAIN_H
