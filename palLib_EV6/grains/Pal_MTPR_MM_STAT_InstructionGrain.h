#ifndef PAL_MTPR_MM_STAT_INSTRUCTIONGRAIN_H
#define PAL_MTPR_MM_STAT_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_MM_STAT_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_MM_STAT
//
// Function code:
//   CALL_PAL 0x3D
//
// Summary:
//   Writes the Memory Management Status Register (MM_STAT). Useful for
//   clearing state after handling faults or managing implementation-defined
//   VM conditions.
//
// References:
//   - Alpha AXP System Reference Manual, MM_STAT IPR.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_MM_STAT_InstructionGrain : public PalInstructionBase<0x3D>
{
public:
    Pal_MTPR_MM_STAT_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_MM_STAT"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Implement cpu.writeIPR_MM_STAT_Active(id, val).
        //   2) Ensure that writing MM_STAT does not conflict with
        //      your emulator's trap/fault scheduling code.
        //   3) Add trace logging for VM state transitions.
        //
        // Example:
        //   cpu.writeIPR_MM_STAT_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_MM_STAT_InstructionGrain);

#endif // PAL_MTPR_MM_STAT_INSTRUCTIONGRAIN_H
