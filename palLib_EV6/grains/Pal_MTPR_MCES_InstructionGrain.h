#ifndef PAL_MTPR_MCES_INSTRUCTIONGRAIN_H
#define PAL_MTPR_MCES_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_MCES_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_MCES
//
// Function code:
//   CALL_PAL 0x55
//
// Summary:
//   Writes the Machine Check Error Summary (MCES) register. Used to control
//   and clear machine check reporting and enable bits.
//
// References:
//   - Alpha AXP System Reference Manual, MCES IPR.
//   - Machine check handling and reporting rules.
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_MCES_InstructionGrain : public PalInstructionBase<0x55>
{
public:
    Pal_MTPR_MCES_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_MCES"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Provide an accessor:
        //        cpu.writeIPR_MCES_Active(id, val);
        //   2) Mask writable bits according to the MCES definition so reserved
        //      bits remain unchanged.
        //   3) If MCES bits clear machine check status, ensure your machine
        //      check controller or IRQController is updated.
        //   4) Add debug logging for MCES writes (old/new values).
        //
        // Example:
        //   cpu.writeIPR_MCES_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_MCES_InstructionGrain);

#endif // PAL_MTPR_MCES_INSTRUCTIONGRAIN_H
