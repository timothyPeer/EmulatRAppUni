#ifndef PAL_MTPR_USP_INSTRUCTIONGRAIN_H
#define PAL_MTPR_USP_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_USP_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_USP
//
// Function code:
//   CALL_PAL 0x33
//
// Summary:
//   Writes the User Stack Pointer (USP) into the processor IPR state.
//
// References:
//   - Alpha AXP System Reference Manual, USP IPR
//   - PALcode rules for kernel-to-user transitions
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_USP_InstructionGrain : public PalInstructionBase<0x33>
{
public:
    Pal_MTPR_USP_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_USP"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add setter: cpu.writeIPR_USP_Active(id, val).
        //   2) Ensure new USP is valid in user mode.
        //   3) May need to interact with exception return logic (REI).
        //   4) Add debug logging for USP modification.
        //
        // Example:
        //   cpu.writeIPR_USP_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_USP_InstructionGrain);

#endif // PAL_MTPR_USP_INSTRUCTIONGRAIN_H
