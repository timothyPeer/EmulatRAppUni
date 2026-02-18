#ifndef PAL_MFPR_PTBR_INSTRUCTIONGRAIN_H
#define PAL_MFPR_PTBR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MFPR_PTBR_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MFPR_PTBR
//
// Function code:
//   CALL_PAL 0x34
//
// Summary:
//   Reads the Page Table Base Register (PTBR), which defines the base of the
//   page table tree for virtual address translation.
//
// References:
//   - Alpha AXP System Reference Manual, PTBR register.
//   - TLB refill, VA-to-PA conversion, memory management rules.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_PTBR_InstructionGrain : public PalInstructionBase<0x34>
{
public:
    Pal_MFPR_PTBR_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MFPR_PTBR"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add accessor: cpu.readIPR_PTBR_Active(id).
        //   2) Choose PAL return integer register.
        //   3) Write PTBR value to integer register.
        //   4) Useful trace data for debugging VA translation behavior.
        //
        // Example:
        //   quint64 ptbr = cpu.readIPR_PTBR_Active(id);
        //   cpu.writeIntReg(dest, ptbr);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MFPR_PTBR_InstructionGrain);

#endif // PAL_MFPR_PTBR_INSTRUCTIONGRAIN_H
