#ifndef PAL_MFPR_SCBB_INSTRUCTIONGRAIN_H
#define PAL_MFPR_SCBB_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_SCBB_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_SCBB
//
// Function code:
//   CALL_PAL 0x36
//
// Summary:
//   Reads the System Control Block Base (SCBB), which points to interrupt
//   vector tables and system exception handlers.
//
// References:
//   - Alpha AXP System Reference Manual, SCBB IPR.
//   - VMS/OSF interrupt vector table base rules.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_SCBB_InstructionGrain : public PalInstructionBase<0x36>
{
public:
    Pal_MFPR_SCBB_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_SCBB"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add accessor cpu.readIPR_SCBB_Active(id).
        //   2) Write SCBB value to PAL return register.
        //   3) Trace logging for vector table base reads.
        //
        // Example:
        //   quint64 scbb = cpu.readIPR_SCBB_Active(id);
        //   cpu.writeIntReg(dest, scbb);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_SCBB_InstructionGrain);

#endif // PAL_MFPR_SCBB_INSTRUCTIONGRAIN_H
