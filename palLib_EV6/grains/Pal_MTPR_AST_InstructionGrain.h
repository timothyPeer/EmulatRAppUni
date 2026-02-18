#ifndef PAL_MTPR_AST_INSTRUCTIONGRAIN_H
#define PAL_MTPR_AST_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_AST_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_AST
//
// Function code:
//   CALL_PAL 0x09
//
// Summary:
//   Writes the AST (Asynchronous System Trap) level into the processor AST
//   register. OS kernels use this to raise or lower AST levels when switching
//   context or preparing exits from kernel to user mode.
//
// References:
//   - Alpha AXP System Reference Manual, AST definitions.
//   - VMS and Tru64 context switching rules.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_AST_InstructionGrain : public PalInstructionBase<0x09>
{
public:
    Pal_MTPR_AST_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_AST"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc   = getPC(cpu);
        const quint64 val  = cpu.readIntReg(di.ra);
        const int id       = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add writeIPR_AST_Active(id, val) to AlphaProcessorContext.
        //   2) Validate AST range: usually small integer modes.
        //   3) If AST lowering triggers pending traps, schedule them.
        //   4) Provide trace output for debugging.
        //
        // Example:
        //   cpu.writeIPR_AST_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_AST_InstructionGrain);

#endif // PAL_MTPR_AST_INSTRUCTIONGRAIN_H
