#ifndef PAL_WR_UNIQ_INSTRUCTIONGRAIN_H
#define PAL_WR_UNIQ_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_WR_UNIQ_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: WR_UNIQ (Write Unique Value)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFE1 with the
//       correct CALL_PAL function code for WR_UNIQ in your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFE1  // TODO: replace with real WR_UNIQ PAL function code
//
// Summary:
//   Writes the per-CPU or per-context "unique" value used by the OS to
//   index thread-local or process-local data structures.
//
// References:
//   - Alpha AXP System Reference Manual, unique value semantics.
//   - OS PALcode descriptions of WR_UNIQ.
// ============================================================================

#include "PalInstructionBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../MBoxLib_EV6/MBoxBase.h"

class Pal_WR_UNIQ_InstructionGrain : public PalInstructionBase<0xFFE1> // TODO: fix PalFunc
{
public:
    Pal_WR_UNIQ_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "WR_UNIQ";
    }

protected:
    void executePAL(PipelineSlot& slot,
                    AlphaProcessorContext* ctx) const override
    {
        // PAL calling convention: new UNIQ value typically in di.ra.
//         const quint64 pc  = getPC(ctx);
//         const quint64 val = ctx.readIntReg(slot.di.ra);
//         const int id      = ctx.cpuId();
        Q_UNUSED(pc);


        ctx.getMBox()->executeWRUNIQUE(slot);
        // TODO:
        //   1) Add a setter:
        //        cpu.writeIPR_UNIQUE_Active(id, val);
        //   2) Decide whether UNIQ is per-CPU, per-HWPCB, or per-thread,
        //      and connect it to your scheduler/task control block model.
        //   3) When UNIQ changes, ensure any cached pointers derived from
        //      the old value are invalidated.
        //   4) Provide trace logging for UNIQ writes to help debug
        //      thread-local storage behavior.
        //
        // Example:
        //   cpu.writeIPR_UNIQUE_Active(id, val);

        Q_UNUSED(val);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_WR_UNIQ_InstructionGrain);

#endif // PAL_WR_UNIQ_INSTRUCTIONGRAIN_H
