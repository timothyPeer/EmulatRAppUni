#ifndef PAL_MFPR_WHAMI_INSTRUCTIONGRAIN_H
#define PAL_MFPR_WHAMI_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_WHAMI_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_WHAMI
//
// Function code:
//   CALL_PAL 0x54  (implementation-specific, EV6-style identity)
//
// Summary:
//   Returns the identity of the executing CPU, typically used for SMP-aware
//   software to distinguish processors.
//
// References:
//   - Alpha AXP System Reference Manual, WHAMI / CPU identity registers.
//   - EV6 family implementation notes.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_WHAMI_InstructionGrain : public PalInstructionBase<0x54>
{
public:
    Pal_MFPR_WHAMI_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_WHAMI"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide a WHAMI accessor on AlphaProcessorContext, for example:
        //        quint64 who = cpu.readIPR_WHAMI_Active(id);
        //      or just use cpuId() if your architectural model equates them.
        //   2) Write the WHAMI value into the PAL return register (e.g. R0 or v0).
        //   3) Add trace logging to help debug SMP topology and CPU identity use.
        //
        // Example:
        //   quint64 who = cpu.readIPR_WHAMI_Active(id);
        //   cpu.writeIntReg(retReg, who);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_WHAMI_InstructionGrain);

#endif // PAL_MFPR_WHAMI_INSTRUCTIONGRAIN_H
