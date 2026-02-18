#ifndef PAL_RD_UNIQ_INSTRUCTIONGRAIN_H
#define PAL_RD_UNIQ_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_RD_UNIQ_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: RD_UNIQ (Read Unique Value)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFE0 with the
//       correct CALL_PAL function code for RD_UNIQ in your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFE0  // TODO: replace with real RD_UNIQ PAL function code
//
// Summary:
//   Returns a per-CPU or per-context "unique" value. Operating systems
//   often use this field to point to per-thread or per-process data
//   structures (e.g., a kernel-mode TCB pointer).
//
// References:
//   - Alpha AXP System Reference Manual, PALcode unique value usage.
//   - OSF/1 and OpenVMS PALcode documentation for RD_UNIQ.
// ============================================================================

#include "PalInstructionBase.h"

class Pal_RD_UNIQ_InstructionGrain : public PalInstructionBase<0xFFE0> // TODO: fix PalFunc
{
public:
    Pal_RD_UNIQ_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "RD_UNIQ";
    }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        const quint64 pc = getPC(cpu);
        const int id     = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add an accessor on AlphaProcessorContext or IPRStorage for the "unique"
        //      value, for example:
        //        quint64 uniq = cpu.readIPR_UNIQUE_Active(id);
        //   2) Decide which architectural integer register receives the
        //      result (PAL ABI: often R0 or v0).
        //   3) Store the unique value to that register:
        //        cpu.writeIntReg(destReg, uniq);
        //   4) Provide trace logging to correlate per-thread data lookup
        //      with UNIQ reads during debugging.
        //
        // Example:
        //   quint64 uniq = cpu.readIPR_UNIQUE_Active(id);
        //   cpu.writeIntReg(retReg, uniq);

        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_RD_UNIQ_InstructionGrain);

#endif // PAL_RD_UNIQ_INSTRUCTIONGRAIN_H
