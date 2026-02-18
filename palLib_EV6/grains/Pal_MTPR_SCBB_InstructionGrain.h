#ifndef PAL_MTPR_SCBB_INSTRUCTIONGRAIN_H
#define PAL_MTPR_SCBB_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_SCBB_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_SCBB
//
// Function code:
//   CALL_PAL 0x37
//
// Summary:
//   Writes the System Control Block Base (SCBB), which defines the base
//   address of the system's interrupt vector table and exception handlers.
//
// References:
//   - Alpha AXP System Reference Manual, SCBB register.
//   - OS PALcode (OpenVMS, OSF/Tru64).
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_SCBB_InstructionGrain : public PalInstructionBase<0x37>
{
public:
    Pal_MTPR_SCBB_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_SCBB"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc  = getPC(cpu);
        const quint64 val = cpu.readIntReg(di.ra);
        const int id      = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add cpu.writeIPR_SCBB_Active(id, val).
        //   2) Validate alignment: SCBB must be aligned according to
        //      system vector table requirements.
        //   3) If emulator models interrupt dispatch tables, update
        //      the vector base in the interrupt subsystem.
        //   4) Provide trace logging for SCBB updates.
        //
        // Example:
        //   cpu.writeIPR_SCBB_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_MTPR_SCBB_InstructionGrain);

#endif // PAL_MTPR_SCBB_INSTRUCTIONGRAIN_H
