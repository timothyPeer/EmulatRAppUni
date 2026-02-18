#ifndef PAL_WR_PS_INSTRUCTIONGRAIN_H
#define PAL_WR_PS_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_WR_PS_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: WR_PS (Write Processor Status)
//
// Function code:
//   CALL_PAL 0x46
//
// Summary:
//   Writes the PS (Processor Status) register. This modifies mode bits,
//   interrupt-enable state, IPL, and other critical execution context flags.
//
// References:
//   - Alpha AXP System Reference Manual, PS IPR.
//   - Exception return, mode switching semantics.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_WR_PS_InstructionGrain : public PalInstructionBase<0x46>
{
public:
    Pal_WR_PS_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "WR_PS"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide accessor in AlphaProcessorContext:
        //        cpu.writeIPR_PS_Active(id, val)
        //   2) Validate allowed PS bit transitions.
        //   3) If mode transition occurs (kernel <-> user), update:
        //        - MMU/translation state
        //        - interrupt behavior
        //   4) If IPL lowered, evaluate pending interrupts.
        //   5) Debug trace: old/new PS.
        //
        // Example:
        //   cpu.writeIPR_PS_Active(id, val);
        //   cpu.evaluateInterruptState(id);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_WR_PS_InstructionGrain);

#endif // PAL_WR_PS_INSTRUCTIONGRAIN_H
