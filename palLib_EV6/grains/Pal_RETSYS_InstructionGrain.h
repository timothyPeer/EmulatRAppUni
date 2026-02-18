#ifndef PAL_RETSYS_INSTRUCTIONGRAIN_H
#define PAL_RETSYS_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_RETSYS_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: RETSYS (Return from System Call)
//
// NOTE: PalFunc value below is a placeholder. Replace 0xFFF5 with the
//       correct RETSYS PAL function code for your PAL profile.
//
// Function code (placeholder):
//   CALL_PAL 0xFFF5  // TODO: replace with real RETSYS PAL function code
//
// Summary:
//   Returns from a system call, restoring PS, mode bits, and PC from
//   the syscall exception frame created at CALLSYS entry.
//
// References:
//   - Alpha Architecture Reference Manual, CALL_PAL REI/RTI/RETSYS.
//   - OS-specific PALcode (VMS / Tru64) syscall exit sequences.
// ============================================================================

#include "PalInstructionBase.h"

class CPUStateIPRInterface;
void executePALRetsys_iface(CPUStateIPRInterface* cpuState);

class Pal_RETSYS_InstructionGrain : public PalInstructionBase<0xFFF5> // TODO: fix PalFunc
{
public:
    Pal_RETSYS_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "RETSYS"; }

protected:
    void executePAL(const DecodedInstruction& di,
                    AlphaProcessorContext& cpu) const override
    {
        Q_UNUSED(di);

        CPUStateIPRInterface* iface = static_cast<CPUStateIPRInterface*>(&cpu);
        executePALRetsys_iface(iface);

        // TODO:
        //   1) Complete executePALRetsys_iface:
        //        - Restore PS from syscall frame.
        //        - Restore return PC to user or kernel-mode caller.
        //        - Restore argument and preserved registers per PAL ABI.
        //   2) Ensure any pending traps or signals are processed after return.
    }
};

REGISTER_GRAIN(Pal_RETSYS_InstructionGrain);

#endif // PAL_RETSYS_INSTRUCTIONGRAIN_H
