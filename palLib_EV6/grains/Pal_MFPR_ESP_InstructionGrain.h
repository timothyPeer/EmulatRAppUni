#ifndef PAL_MFPR_ESP_INSTRUCTIONGRAIN_H
#define PAL_MFPR_ESP_INSTRUCTIONGRAIN_H
// ============================================================================
// MTPR_AESP - Move To Processor Register (ESP)
// CALL_PAL 0x07 (Privileged)
//
// Grain Responsibility: Route to MBOX
// MBox Responsibility: Write ESP to IPR, coordinate side effects via MBox
// ============================================================================

#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

class Pal_MFPR_ESP_InstructionGrain : public PalInstructionBase<0x01E>
{
public:
    Pal_MFPR_ESP_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "MFPR_ESP";
    }
    AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const noexcept override
    {
        // Route to MBox -
        // MBox complete side effects
        // Update pipeline stage_WB as slot.needsWriteback = false;
        ctx.getMBox()->executeMFPR_ESP(slot);
    }
};

REGISTER_GRAIN(Pal_MFPR_ESP_InstructionGrain);
#endif // PAL_MFPR_ESP_INSTRUCTIONGRAIN_H
