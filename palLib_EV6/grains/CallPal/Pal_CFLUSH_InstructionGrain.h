#ifndef PAL_CFLUSH_INSTRUCTIONGRAIN_H
#define PAL_CFLUSH_INSTRUCTIONGRAIN_H

// ============================================================================
// Pal_CFLUSH_InstructionGrain.h
// ============================================================================
// Alpha AXP PALcode: CFLUSH
//
// Function code:
//   CALL_PAL 0x01  (CFLUSH)
//
// Architectural summary:
//   CFLUSH requests that the implementation flush or invalidate caches.
//   The exact scope (instruction cache, data cache, both) is
//   implementation-dependent, but the general intent is to ensure that
//   subsequent instruction and data accesses observe updated memory contents.
//
// References:
//   - Alpha AXP System Reference Manual, Version 6 (1994),
//     PALcode chapter, CALL_PAL CFLUSH.
//   - Alpha 21164 and later Hardware Reference Manuals, cache control.
// ============================================================================
#include "PalInstructionBase.h"
#include "../../cpuCoreLib/AlphaProcessorContext.h"

#include "../MBoxLib_EV6/MBoxBase.h"
#include "../../grainFactoryLib/PipeLineSlot.h"
#include "../../grainFactoryLib/GrainRegistrationCore.h"

// CFLUSH: CALL_PAL 0x01
class Pal_CFLUSH_InstructionGrain : public PalInstructionBase<0x01>
{
public:
    Pal_CFLUSH_InstructionGrain() noexcept = default;

    QString mnemonic() const override
    {
        return "CFLUSH";
    }

    AXP_HOT  void execute(PipelineSlot& slot,  AlphaProcessorContext* ctx) const noexcept override
    {
        // TODO: CFLUSH implementation details
        //   1) Define a cache model interface in your emulator:
        //        - Instruction cache model
        //        - Data cache model
        //   2) Implement a method to flush or invalidate all cache lines,
        //        for example:
        //        - cpu.flushAllCaches()
        //        - or cacheController.flushAll()
        //   3) Ensure that any subsequent memory fetches go to SafeMemory
        //      or backing store, and not to stale cache lines.
        //   4) Decide whether CFLUSH is a full barrier in your emulator:
        //        - If yes, integrate with existing memory barrier helpers
        //          such as MB/WMB and your SMP/ReservationManager model.
        //   5) Provide logging or tracing hooks via your logging macros so
        //      that CFLUSH events can be observed during debugging.
        //
        // Source:
        //   - Alpha AXP System Reference Manual, PALcode CFLUSH definition,
        //     CALL_PAL 0x01.
		// Route to MBox -
		// MBox complete side effects
		// Update pipeline stage_WB as slot.needsWriteback = false;
        ctx.getMBox()->executeCFLUSH(slot);
    }
};

REGISTER_GRAIN(Pal_CFLUSH_InstructionGrain);

#endif // PAL_CFLUSH_INSTRUCTIONGRAIN_H
