#pragma once
#include "CPUStateIPRInterface.h"
#include "Global_HWPCBBank_Interface.h"
#include "IPRStorage_core.h"
#include <QtGlobal>
#include "HWPCB_helpers_inline.h"

/*
    RTI – Return From Interrupt

    Architecture:
    - Restore PC and PS from HWPCB.
    - Return to previous privilege mode and IPL.
    - Clear PALmode bit.
    - Resume normal instruction execution at restored PC.
*/
inline void executeRTI_iface(CPUStateIPRInterface* cpuState)
{
    if (!cpuState) {
        return;
    }

    const quint8 cpuId = cpuState->cpuId();

    // Access HWPCB (global)
    auto& pcb = globalHWPCBController()(cpuId);

    // 1. Restore saved PC and PS from HWPCB
    const quint64 restoredPC = globalHWPCBController()(cpuId).getPC();
    const quint64 restoredPS = pcb.getSavedPS();

    // 2. Apply to CPU architectural registers / IPRs
    setPC_Active(restoredPC);
    setPS_Active(restoredPS);
    //     cpuState->setPC(restoredPC);
    //     cpuState->setPS(restoredPS);

        // 3. Leave PALmode
        // Most PAL implementations track PALmode in HWPCB or in IPR.MCES/PS bits.
        // We clear it here, so execution resumes in normal ISA mode.
    cpuState->exitPalMode();

    // Optional (if your implementation uses shadow registers or saved state):
    // pcb.loadSavedState(cpuId);

    // Done: next instruction fetch uses restored PC.

    // todo
    /*
    pcb.clearInProgressAST(cpuId);
    pcb.restoreShadowRegisters(cpuId);
    */
}
