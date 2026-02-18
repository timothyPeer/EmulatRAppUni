#ifndef PAL_MTPR_PTBR_INSTRUCTIONGRAIN_H
#define PAL_MTPR_PTBR_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_MTPR_PTBR_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: MTPR_PTBR
//
// Function code:
//   CALL_PAL 0x35
//
// Summary:
//   Writes the Page Table Base Register (PTBR). Changing this affects all
//   virtual memory mappings and requires TLB invalidation.
//
// References:
//   - Alpha AXP System Reference Manual, PTBR semantics.
//   - OS PALcode (VMS/OSF) for paging structures.
// ============================================================================

#include "PalInstructionBase.h"
#include "grainFactoryLib/GrainRegistrationCore.h"

class Pal_MTPR_PTBR_InstructionGrain : public PalInstructionBase<0x35>
{
public:
    Pal_MTPR_PTBR_InstructionGrain() noexcept = default;

    QString mnemonic() const override { return "MTPR_PTBR"; }

protected:
    void executePAL(PipelineSlot& slot) const 
    {
        const quint64 pc  = getPC_Active(slot.cpuId);
        const quint64 val = slot.readIntReg(slot.di.ra);
        const int id      = slot.cpuId;
        Q_UNUSED(pc)

        // TODO:
        //   1) Add cpu.writeIPR_PTBR_Active(id, val).
        //   2) Perform TBIA (invalidate all TLB entries) or the equivalent
        //      of a new ASN epoch if required.
        //   3) Update paging state in the emulator (if PTBR changes the
        //      root page table physical address).
        //   4) Add trace logging for PTBR changes.
        //
        // Example:
        //   cpu.writeIPR_PTBR_Active(id, val);
        //   cpu.invalidateTLB_All(id);

        Q_UNUSED(val)
        Q_UNUSED(id)
    }
};

REGISTER_GRAIN(Pal_MTPR_PTBR_InstructionGrain);

#endif // PAL_MTPR_PTBR_INSTRUCTIONGRAIN_H
