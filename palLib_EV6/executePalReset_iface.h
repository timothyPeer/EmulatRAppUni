// #pragma once
// #include <QtGlobal>
// #include "pendingEvent_struct.h"
// #include "Global_IPRInterface.h"
// #include "../cpuCoreLib/AlphaCPU.h"
// 
// // ============================================================================
// // executePALReset
// //
// // Description:
// //   Perform a cold reset of the processor into PALcode context. This models
// //   the architectural power-up behavior described in the Alpha AXP System
// //   Reference Manual and the PAL_BASE internal processor register.
// //
// //   On a cold reset:
// //
// //     - The PAL_BASE IPR is initialized to 0.
// //       (See Alpha AXP System Reference Manual, Version 6 (1994),
// //        Appendix F, "PAL_BASE Internal Processor Register" and
// //        Figure F-9.)
// //
// //     - The hardware vectors into PALcode at an offset from PAL_BASE.
// //       The reset vector is the PALcode initialization vector and must
// //       begin at offset 0 within the PAL image.
// //       (See Chapter 4, "Exceptions, Interrupts, and Machine Checks",
// //        Table 4-1 "PALcode Entry Points".)
// //
// //   In this emulator, executePALReset:
// //
// //     1. Calls IPRStorage::reset() to reinitialize all IPRs to their
// //        architectural power-on defaults.
// //     2. Explicitly sets PAL_BASE to 0 (matching the ARM/ASA description).
// //     3. Initializes PS to a known reset value (all bits clear). Any OS-
// //        level bootstrap PS (e.g., IPL=31, CM=kernel) should be set later
// //        via initializeBootstrapPS() once PAL hands control to the OS.
// //     4. Sets the PC to PAL_BASE + offset 0, which corresponds to the
// //        PAL reset entry point.
// //     5. Marks the CPU as "in PAL mode" so that the main run loop will
// //        not attempt to deliver asynchronous interrupts until PAL exits
// //        via REI.
// //     6. Clears any cached pending interrupt/AST/SWI state in the
// //        CPUStateIPRInterface view.
// //
// // Notes:
// //   - This routine models a *cold* reset. A warm restart via RESTART
// //     would instead use the restart block and RESTART_ADDRESS handling
// //     described in the firmware chapter and is not covered here.
// //   - OS-visible bootstrap PS semantics (IPL=31, CM=kernel, VMM=0,
// //     IP=0, SW=0) are typically established by PAL and/or the OS
// //     loader after PAL initialization, not by the raw hardware reset.
// // ============================================================================
// 
// inline void executePALReset_iface(AlphaCPU& argCpu) noexcept
// {
// 	quint8 cpuId = argCpu.cpuId();
// 	// 1. Reset architectural IPR state to power-on defaults.
// 	//
// 	//    This clears PS, PC, IER, AST state, TLB tags, etc., according to
// 	//    your IPRStorage::reset() implementation.
// 	auto& iprs = globalIPRHot(cpuId);
// 	iprs.reset();
// 
// 	// 2. Per ASA/ARM: PAL_BASE = 0 at power-up.
// 	//    (High-order bits of PC come from PAL_BASE; low 14 bits from
// 	//     the entry offset within the PAL image.)
// 	iprs.pal_base = 0;
// 
// 	// 3. Initialize PS to a clean reset state.
// 	//
// 	//    Here we leave PS = 0 (kernel mode, IPL=0, IP=0, VMM=0, SW=0).
// 	//    If you want OS-level bootstrap semantics (IPL=31, CM=kernel),
// 	//    that should be done by calling initializeBootstrapPS() at the
// 	//    appropriate point in your PAL/OS boot path, not in the raw reset.
// 	setPS_Active(cpuId, 0);
// 
// 	// Keep any cached CM/IPL mirrors in sync with the PS register.
// 
// 	argCpu.updateCachedIPLFromPS();
// 	argCpu.updateCachedModeFromPS();
// 
// 	// 4. Set the PC to the PAL reset entry point.
// 	//
// 	//    Architecturally, the reset vector is at PAL_BASE + offset 0
// 	//    (Table 4-1). Since PAL_BASE has just been set to 0, this is
// 	//    simply 0. If you later support non-zero PAL_BASE values (e.g.,
// 	//    multiple PAL images), this line should be:
// 	//
// 	//        iprs.pc = (iprs.pal_base & PAL_BASE_MASK) | PAL_RESET_OFFSET;
// 	//
// 	//    where PAL_RESET_OFFSET == 0x0.
// 	setPC_Active(cpuId,iprs.pal_base);  // PAL reset vector: offset 0
// 
// 	// 5. Enter PAL mode: from this point forward, the CPU is executing
// 	//    PALcode until a REI (or equivalent firmware transfer) returns
// 	//    to non-PAL execution.
// 	argCpu.enterPalMode();
// 
// 	// 6. Clear any CPU-side pending interrupt/event state.
// 	//
// 	//    The IRQController may have its own notion of pending vectors;
// 	//    system-level reset should clear those via a separate controller
// 	//    reset, but from the CPU’s perspective we start with a clean slate.
// 	argCpu.setEventPendingState(PendingEventKind::None);
// 	argCpu.setEventPending(false);
// 	argCpu.setInterruptPending(false);
// 	argCpu.setEntryOffset(0x0000);		// PAL Base
// 	argCpu.setPendingSoftware(false); // SWI Pending
// 	argCpu.setPendingAST(false);
// 	setFaultedVA(0);
// 	argCpu.setTrapPending(false);
// 
// 	// If your AST/SWI enable mirrors are cached (m_enableAST, m_sienMask,
// 	// etc.), they were already reset by iprs.reset() + recalculate any
// 	// priorities the next time IER/CM is written.
// }