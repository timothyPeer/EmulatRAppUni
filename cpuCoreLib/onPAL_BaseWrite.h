#ifndef ONPAL_BASEWRITE_H
#define ONPAL_BASEWRITE_H
#include "Global_IPRInterface.h"
#include "../coreLib/LoggingMacros.h"
#include "types_core.h"
#include <QtGlobal>
#include "AlphaCPU.h"

inline void onPAL_BASEWrite(AlphaCPU* cpuState, quint64 oldValue, quint64 newValue)
{
    // EV6 PALcode Base Address Write Hook

    // 1. Logging and Tracing
    const CPUIdType cpuId = cpuState->cpuId();
    TRACE_LOG(QString("PAL_BASE:: oldvalue: %1 - newvalue: %2").arg(oldValue).arg(newValue));

    // 2. Validate PALcode Base Address
    // EV6 specific: Lower 4 bits must be zero (0xFFFFFFFFFFFFFFF0 mask)
    quint64 validatedValue = newValue & 0xFFFFFFFFFFFFFFF0ULL; // PAL_BASE must be 16-byte aligned, bottom bits are architecturally reserve

    // 3. Update PALcode Base Address in Storage
    auto& iprs = globalIPRBank()[cpuId];

    //auto& iprs = cpuState->getIPRStorage();
    iprs.hot.pal_base = validatedValue;			// the authoritative value

    // 4. PALcode Transition Handling
    // Detect if PALcode base has substantially changed
    if ((oldValue ^ validatedValue) & 0xFFFFFFFFFFFFFFF0ULL) {
        // Significant PALcode base change detected
        // Clear PAL instruction cache (optional)
        // Reset internal palExecBase = pal_base
        // Reset PALmode PC if necessary
        onPALcodeBaseChange(cpuState, oldValue, validatedValue); //TODO
    }

    // 5. Memory Management Synchronization
    // Update page table base and virtual address translation
    // physical range: [PAL_BASE, PAL_BASE + 64 KB)
    //  Register the PALcode region as a MMU - bypass region
    // 	Ensure SafeMemory::readPhysical is used for PAL - mode fetch
    // 	Install a memory - mapping descriptor
    // 	Allow PAL loads / stores to bypass DTB

    onSyncPALcodeMemoryMapping(cpuState, validatedValue); //TODO

    // 6. Privilege and Security Validation
    /*
        Address must be RAM or ROM
        Not overlapping MMIO windows
        Not > physical memory limit
        Must not conflict with CPU scratch areas
    */
    if (!onValidatePALcodeBaseAddress(cpuState, validatedValue)) { //TODO
        // Invalid PALcode base address
        onTriggerPrivilegeViolation(cpuState, validatedValue); //TODO
        return;
    }

    // 7. Interrupt and Exception Vector Reconfiguration
    /* ALL FAULT VECTORS LIVE IN PALcode
        Arithmetic traps
        Machine checks
        Opcode faults
        TLB misses
        Interrupt dispatch entry points
        Bugcheck handlers
    */
    // PAL_BASE + offset_for_vector
    onReconfigureInterruptVectors(cpuState, validatedValue); //TODO

    // 8. PALcode State Reset
    // Potentially reset certain PALcode-related state
    /*
        Reset PAL private registers
        Reset HWPCB PAL scratch registers
        Flush PAL's internal return registers (e.g., scratch IPRs)
        Clear any lingering PALmode execution flags
    */
    // Without this, SRM returns into garbage after REI
    resetPALcodeState(cpuState, validatedValue); //TODO
}


#endif // ONPAL_BASEWRITE_H
