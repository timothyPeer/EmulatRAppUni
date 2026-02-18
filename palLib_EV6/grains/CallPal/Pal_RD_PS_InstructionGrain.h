#ifndef PAL_RD_PS_INSTRUCTIONGRAIN_H
#define PAL_RD_PS_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_RD_PS_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: RD_PS  (Read Processor Status)
//
// Function code:
//   CALL_PAL 0x45
//
// Summary:
//   Reads the processor status (PS) register. This register contains system
//   mode bits, interrupt enable state, IPL, and other key condition flags.
//
// References:
//   - Alpha AXP System Reference Manual, PS IPR.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_RD_PS_InstructionGrain : public PalInstructionBase<0x45>
{
public:
    Pal_RD_PS_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "RD_PS"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const quint64 ps = getPS(cpu);
        Q_UNUSED(pc);

        // TODO:
        //   1) Determine PAL return register (likely R0 or call ABI return).
        //   2) Write 'ps' value into integer register.
        //   3) Add trace logging for mode/interrupt state debugging.
        //
        // Example:
        //   cpu.writeIntReg(retReg, ps);

        Q_UNUSED(ps);
    }
};

REGISTER_GRAIN(Pal_RD_PS_InstructionGrain);

#endif // PAL_RD_PS_INSTRUCTIONGRAIN_H
