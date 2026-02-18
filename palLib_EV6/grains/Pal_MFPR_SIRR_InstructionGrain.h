#ifndef PAL_MFPR_SIRR_INSTRUCTIONGRAIN_H
#define PAL_MFPR_SIRR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_SIRR_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_SIRR
//
// Function code:
//   CALL_PAL 0x3A
//
// Summary:
//   Reads the Software Interrupt Request Register (SIRR), containing pending
//   software interrupts that may be signaled to the processor.
//
// References:
//   - Alpha AXP System Reference Manual, SIRR IPR.
//   - Software interrupt triggering (SIRR bits).
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_SIRR_InstructionGrain : public PalInstructionBase<0x3A>
{
public:
    Pal_MFPR_SIRR_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_SIRR"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add cpu.readIPR_SIRR_Active(id).
        //   2) Write the bitmask value to a PAL return register.
        //   3) Logging: software interrupt vector states.
        //
        // Example:
        //   quint64 sirr = cpu.readIPR_SIRR_Active(id);
        //   cpu.writeIntReg(ret, sirr);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_SIRR_InstructionGrain);

#endif // PAL_MFPR_SIRR_INSTRUCTIONGRAIN_H
