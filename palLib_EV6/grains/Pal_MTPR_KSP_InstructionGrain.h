#ifndef PAL_MTPR_KSP_INSTRUCTIONGRAIN_H
#define PAL_MTPR_KSP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_KSP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_KSP
//
// Function code:
//   CALL_PAL 0x31
//
// Summary:
//   Writes Kernel Stack Pointer (KSP) to the processor KSP register.
//   Part of context switching, interrupt entry, and privileged stack
//   manipulation.
//
// References:
//   - Alpha AXP System Reference Manual (1994), KSP register.
//   - OSF/VMS PALcode documentation for kernel stack handling.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_KSP_InstructionGrain : public PalInstructionBase<0x31>
{
public:
    Pal_MTPR_KSP_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_KSP"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add cpu.writeIPR_KSP_Active(id, val).
        //   2) Possibly update SP in AlphaProcessorContext execution context if KSP
        //      directly participates in SP visible to software.
        //   3) Update trap entry/exit logic to use new KSP as required.
        //   4) Add debug tracing.
        //
        // Example:
        //   cpu.writeIPR_KSP_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_KSP_InstructionGrain);

#endif // PAL_MTPR_KSP_INSTRUCTIONGRAIN_H
