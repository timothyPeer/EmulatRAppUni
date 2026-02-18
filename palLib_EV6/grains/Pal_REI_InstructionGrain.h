#ifndef PAL_REI_INSTRUCTIONGRAIN_H
#define PAL_REI_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_REI_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: REI  (Return From Exception)
//
// Function code:
//   CALL_PAL 0x40
//
// Summary:
//   REI restores processor state from the exception frame, leaving PALmode
//   and resuming execution at the previously saved PC.
//
//   REI is one of the core PAL instructions used for:
//     * Returning from interrupts
//     * Returning from faults and traps
//     * Transitioning between kernel and user modes
//
// References:
//   - Alpha AXP System Reference Manual, CALL_PAL REI description.
//   - VMS and OSF PALcode documentation (exception return).
//   - IPR layout for PS, exception frame, and mode bits.
// ============================================================================
#include <QtGlobal>
#include <QString>
#include "PalInstructionBase.h"
#include "../../grainFactoryLib/iGrain-Decode-Meta.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_REI_InstructionGrain : public PalInstructionBase<0x40>
{
public:
    Pal_REI_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "REI"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext* ctx) const override
    {
        Q_UNUSED(di);

        const quint64 pc  = getPC(ctx);
        const quint64 ps  = getPS(ctx);
        const quint64 hwp = getHWPCB(ctx);
        const int cpuId      = ctx.cpuId();
        Q_UNUSED(pc);
        Q_UNUSED(ps);
        Q_UNUSED(hwp);

        // TODO: REI Implementation (Major)
        //
        //   1) Retrieve the exception frame:
        //        - Contains saved PC, PS, GP, other registers depending on PAL.
        //        - Some PALcode uses HWPCB to locate exception frame fields.
        //
        //   2) Restore these fields into CPU architectural state:
        //        - Restore PS (mode bits: kernel/user, IPL, interrupt enable).
        //        - Restore preserved integer registers from the frame.
        //        - Restore PC to the saved return address.
        //
        //   3) Exit PALmode:
        //        - Clear PALmode flag in AlphaProcessorContext.
        //        - Ensure address translations and protection rules are now
        //          user- or kernel-mode appropriate (depends on PS bits).
        //
        //   4) TLB / MMU interaction:
        //        - Depending on OS, REI may require ASID/ASN validation.
        //        - If needed, perform deferred TLB invalidations.
        //
        //   5) Resume execution:
        //        - Set cpu.nextPC to restored PC.
        //        - Mark trap return complete so no trap re-entry occurs.
        //
        //   6) Provide extensive tracing:
        //        - Old and new PC
        //        - Old and new PS
        //        - Mode change (PALmode -> kernel/user)
        //        - Any TLB or IPR changes
        //
        // Example stub steps (commented until full context model implemented):
        //
        //   ExceptionFrame ex = cpu.readExceptionFrame(id);
        //   cpu.writePS_Active(id, ex.savedPS);
        //   cpu.writePC_Active(id, ex.savedPC);
        //   cpu.exitPALMode(id);
        //
        //   cpu.restoreRegistersFromFrame(ex);
        //   cpu.resumeExecution(id);


		quint64 retPC = getEXC_ADDR_Active(cpuId);

		// Architecturally: clear PALmode
        globalHWPCBController().forceUserPC(cpuId,retPC);
    }
};

REGISTER_GRAIN(Pal_REI_InstructionGrain);

#endif // PAL_REI_INSTRUCTIONGRAIN_H
