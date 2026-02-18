#ifndef PAL_SWPIPL_INSTRUCTIONGRAIN_H
#define PAL_SWPIPL_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_SWPIPL_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: SWPIPL (Swap Interrupt Priority Level)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFE2 with the
//       correct CALL_PAL function code for SWPIPL.
//
// Function code (placeholder):
//   CALL_PAL 0xFFE2  // TODO: replace with real SWPIPL PAL function code
//
// Summary:
//   Atomically swaps the current IPL (interrupt priority level) with a
//   new one supplied in a register, returning the old IPL to the caller.
//   Often used by kernels to raise or lower IPL while preserving the
//   previous level for restoration.
//
// References:
//   - Alpha AXP System Reference Manual, IPL and SPL macros.
//   - OS PALcode specifications for SWPIPL.
// ============================================================================

#include "PalInstructionBase.h"

class Pal_SWPIPL_InstructionGrain : public PalInstructionBase<0xFFE2> // TODO: fix PalFunc
{
public:
    Pal_SWPIPL_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "SWPIPL";
    }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        const quint64 pc      = getPC(cpu);
        const quint64 newIpl  = cpu.readIntReg(di.ra); // proposed new IPL
        const int id          = cpu.cpuId();
        Q_UNUSED(pc);

        // TODO:
        //   1) Add accessors:
        //        quint64 oldIpl = cpu.readIPR_IPLR_Active(id);
        //        cpu.writeIPR_IPLR_Active(id, newIpl);
        //   2) Return old IPL in the PAL ABI result register (often R0/v0).
        //   3) When lowering IPL, check for pending interrupts that were
        //      previously masked and may now be deliverable.
        //   4) When raising IPL, update interrupt masks in your IRQController
        //      model.
        //   5) Trace logs:
        //        - oldIpl, newIpl, CPU id, and resulting pending events.
        //
        // Example:
        //   quint64 oldIpl = cpu.readIPR_IPLR_Active(id);
        //   cpu.writeIPR_IPLR_Active(id, newIpl);
        //   cpu.writeIntReg(retReg, oldIpl);
        //   cpu.checkAndDispatchPendingInterrupts(id);

        Q_UNUSED(newIpl);
        Q_UNUSED(id);
    }
};

REGISTER_GRAIN(Pal_SWPIPL_InstructionGrain);

#endif // PAL_SWPIPL_INSTRUCTIONGRAIN_H
