#ifndef PAL_MFPR_USP_INSTRUCTIONGRAIN_H
#define PAL_MFPR_USP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_USP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_USP
//
// Function code:
//   CALL_PAL 0x32
//
// Summary:
//   Reads the User Stack Pointer (USP) from the processor IPR state.
//
// References:
//   - Alpha AXP System Reference Manual, USP IPR
//   - PALcode kernel entry/exit specifications for OSF and VMS
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_USP_InstructionGrain : public PalInstructionBase<0x32>
{
public:
    Pal_MFPR_USP_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_USP"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide accessor such as:
        //        quint64 val = cpu.readIPR_USP_Active(id);
        //   2) Determine PAL return register for MFPR results.
        //   3) Write value to integer register via cpu.writeIntReg().
        //   4) Add logging for USP changes and debugging of user stack state.
        //
        // Example placeholder:
        //   quint64 usp = cpu.readIPR_USP_Active(id);
        //   cpu.writeIntReg(ret, usp);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_USP_InstructionGrain);

#endif // PAL_MFPR_USP_INSTRUCTIONGRAIN_H
