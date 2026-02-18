#ifndef PAL_MFPR_MCES_INSTRUCTIONGRAIN_H
#define PAL_MFPR_MCES_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_MCES_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_MCES
//
// Function code:
//   CALL_PAL 0x56
//
// Summary:
//   Reads the Machine Check Error Summary (MCES) register. Software uses this
//   to determine which machine check conditions have occurred or are enabled.
//
// References:
//   - Alpha AXP System Reference Manual, MCES IPR.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_MCES_InstructionGrain : public PalInstructionBase<0x56>
{
public:
    Pal_MFPR_MCES_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_MCES"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide an accessor:
        //        quint64 mces = cpu.readIPR_MCES_Active(id);
        //   2) Write MCES into the PAL return register (e.g., R0 or v0).
        //   3) Log MCES reads during machine check handling for diagnostics.
        //
        // Example:
        //   quint64 mces = cpu.readIPR_MCES_Active(id);
        //   cpu.writeIntReg(retReg, mces);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_MCES_InstructionGrain);

#endif // PAL_MFPR_MCES_INSTRUCTIONGRAIN_H
