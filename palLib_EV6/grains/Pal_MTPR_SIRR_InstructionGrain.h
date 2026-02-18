#ifndef PAL_MTPR_SIRR_INSTRUCTIONGRAIN_H
#define PAL_MTPR_SIRR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_SIRR_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_SIRR
//
// Function code:
//   CALL_PAL 0x3B
//
// Summary:
//   Sets software interrupt request bits in the SIRR. This allows PAL or OS
//   code to trigger asynchronous software interrupts inside the processor.
//
// References:
//   - Alpha AXP System Reference Manual, SIRR IPR.
//   - Software interrupt generation rules.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_SIRR_InstructionGrain : public PalInstructionBase<0x3B>
{
public:
    Pal_MTPR_SIRR_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_SIRR"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add cpu.writeIPR_SIRR_Active(id, val).
        //   2) Ensure only software-interrupt bits are writable.
        //   3) Trigger evaluation of pending software interrupts.
        //   4) Provide trace output for debugging SIRR modifications.
        //
        // Example:
        //   cpu.writeIPR_SIRR_Active(id, val);
        //   cpu.checkSoftwareInterrupts(id);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_SIRR_InstructionGrain);

#endif // PAL_MTPR_SIRR_INSTRUCTIONGRAIN_H
