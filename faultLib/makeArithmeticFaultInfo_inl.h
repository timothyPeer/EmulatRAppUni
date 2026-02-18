#ifndef MAKEARITHMETICFAULTINFO_INL_H
#define MAKEARITHMETICFAULTINFO_INL_H
#include <QtGlobal>
#include "MemoryFaultInfo.h"
#include "../cpuCoreLib/AlphaCPU.h"
#include "fault_core.h"
#include "../coreLib/HWPCB_helpers_inline.h"
#include "../coreLib/globalIPR_hot_cold_impl.h"

inline MemoryFaultInfo makeArithmeticFaultInfo(AlphaCPU* argCpu, ArithmeticTrapKind kind) noexcept
{
    const CPUIdType cpuId = argCpu->cpuId();
    auto& iprs = globalIPRHot(cpuId);

    MemoryFaultInfo info;

    info.faultType = MemoryFaultType::NONE;  // Arithmetic -> not memory
    info.faultingVA = 0;
    info.physicalAddress = 0;
    info.accessSize = 0;
    info.isWrite = false;
    info.isExecute = false;

    info.faultingPC = getPC_Active(cpuId);
    info.instruction = argCpu->lastInstrRaw();  // <-- you have this in your decode stage

    // Extended tail
    info.arithmeticKind = kind;
    info.translationValid = true;
    info.inPALmode = iprs.isInPalMode();
    info.currentMode = getCM_Active(cpuId);

    return info;
}
#endif // MAKEARITHMETICFAULTINFO_INL_H
