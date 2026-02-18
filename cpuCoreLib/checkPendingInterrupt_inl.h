#ifndef CHECKPENDINGINTERRUPT_INL_H
#define CHECKPENDINGINTERRUPT_INL_H
#include <QtGlobal>
#include "../faultLib/GlobalFaultDispatcherBank.h"
#include "clearInterruptEligibilityDirty_inl.h"
#include "global_IRQController.h"
#include "types_core.h"
#include "../faultLib/PendingEvent.h"
#include "../exceptionLib/Exception_Core.h"
#include "../coreLib/PAL_core.h"
#include "../coreLib/BitUtils.h"

inline bool checkPendingInterrupts(CPUIdType cpuId)
{
    auto& disp = globalFaultDispatcher(cpuId);
    auto& iprs = globalIPRBank()[cpuId];
    auto& irqCtrl = global_IRQController();

    // Clear the dirty flag
    clearInterruptEligibilityDirty(cpuId);

    // ----------------------------------------------------------------
    // Priority order (highest to lowest):
    // 1. Machine checks (always taken)
    // 2. Hardware interrupts (IPL-based)
    // 3. Software interrupts (SIRR/SIER)
    // 4. AST (mode-based)
    // ----------------------------------------------------------------

    // ================================================================
    // 1. Check for machine check
    // ================================================================
    if (irqCtrl.hasPendingMachineCheck(cpuId)) {
        PendingEvent ev{};
        ev.kind = PendingEventKind::MachineCheck;
        ev.exceptionClass = ExceptionClass::MCHK;
        ev.palVectorId = PalVectorId::MCHK;
        ev.mcReason = irqCtrl.getMachineCheckReason(cpuId);

        disp.setPendingEvent(ev);
        return true;
    }

    // ================================================================
    // 2. Check for hardware interrupts (external IRQ)
    // ================================================================
    const quint8 currentIPL = irqCtrl.getCPUIpl(cpuId);

    if (irqCtrl.hasPendingInterrupt(cpuId, currentIPL)) {
        quint8 irqLevel = irqCtrl.getHighestPendingIRQ(cpuId);

        if (irqLevel > currentIPL) {
            PendingEvent ev{};
            ev.kind = PendingEventKind::HardwareIRQ;
            ev.exceptionClass = ExceptionClass::Interrupt;
            ev.palVectorId = PalVectorId::INTERRUPT;
            ev.hwIPL = irqLevel;
            ev.hwVector = irqCtrl.getIRQVector(cpuId, irqLevel);

            disp.setPendingEvent(ev);
            return true;
        }
    }

    // ================================================================
    // 3. Check for software interrupts (SIRR)
    // ================================================================
    auto& irq = global_IRQController();
    quint8 swiPending = irq.getPendingSoftwareInterruptMask(cpuId); //  ch.hot.sirr & 0xFFFE;  // SWI levels 1-15
    if (swiPending != 0) {
        quint8 swiLevel = BitUtils::highestSetBit(swiPending);

        PendingEvent ev{};
        ev.kind = PendingEventKind::SoftwareIRQ;
        ev.exceptionClass = ExceptionClass::SWI;
        ev.palVectorId = PalVectorId::SWI;
        ev.swiLevel = swiLevel;

        disp.setPendingEvent(ev);
        return true;
    }

    // ================================================================
    // 4. Check for AST (Asynchronous System Trap)
    // ================================================================
    disp.checkAST();  //  Uses your existing checkAST() in FaultDispatcher

    if (disp.eventPending()) {
        return true;
    }

    return false;
}
#endif // CHECKPENDINGINTERRUPT_INL_H
