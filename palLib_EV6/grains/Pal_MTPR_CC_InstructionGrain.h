#ifndef PAL_MTPR_CC_INSTRUCTIONGRAIN_H
#define PAL_MTPR_CC_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_CC_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_CC
//
// Function code:
//   CALL_PAL 0x53   (EV5, EV56, EV6)
//
// Summary:
//   Writes the cycle counter. Only privileged PALcode or OS kernel code
//   typically manipulates this.
//
// References:
//   - Alpha EV5/EV56/EV6 Hardware Reference Manuals.
//   - Performance counter architecture.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_CC_InstructionGrain : public PalInstructionBase<0x53>
{
public:
    Pal_MTPR_CC_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_CC"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Implement:
        //        cpu.writeIPR_CC_Active(id, val)
        //   2) Must ensure only privileged code writes CC.
        //   3) Resetting CC may affect profiling tools or timekeeping.
        //   4) Add trace output for counter writes.
        //
        // Example:
        //   cpu.writeIPR_CC_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_CC_InstructionGrain);

#endif // PAL_MTPR_CC_INSTRUCTIONGRAIN_H
