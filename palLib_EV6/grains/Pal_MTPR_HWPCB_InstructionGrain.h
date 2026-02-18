#ifndef PAL_MTPR_HWPCB_INSTRUCTIONGRAIN_H
#define PAL_MTPR_HWPCB_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_HWPCB_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_HWPCB
//
// Function code:
//   CALL_PAL 0x3F
//
// Summary:
//   Writes the Hardware PCB pointer (HWPCB). Used heavily in context
//   switching and exception return to replace kernel context structures.
//
// References:
//   - Alpha AXP System Reference Manual, HWPCB register.
//   - PALcode context switching semantics.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_HWPCB_InstructionGrain : public PalInstructionBase<0x3F>
{
public:
    Pal_MTPR_HWPCB_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_HWPCB"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add cpu.writeIPR_HWPCB_Active(id, val).
        //   2) Updating HWPCB triggers context switch:
        //        - Load new PTBR, KSP, USP, ASN, PS, etc.
        //   3) Must interact with your TLB invalidation model.
        //   4) Ensure your AlphaProcessorContext fetch/decode loop sees updated context.
        //   5) Add detailed debug logs for HWPCB transitions.
        //
        // Example:
        //   cpu.writeIPR_HWPCB_Active(id, val);
        //   cpu.contextSwitchFromHWPCB(id);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_HWPCB_InstructionGrain);

#endif // PAL_MTPR_HWPCB_INSTRUCTIONGRAIN_H
