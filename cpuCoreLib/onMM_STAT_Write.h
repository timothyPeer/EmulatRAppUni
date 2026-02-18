#ifndef ONMM_STAT_WRITE_H
#define ONMM_STAT_WRITE_H
#include "AlphaCPU.h"
#include <QtGlobal>

inline void onMM_STATWrite(AlphaCPU* argCpu, quint64 oldValue, quint64 newValue)
{
    if (!argCpu) return;

    const CPUIdType cpuId = argCpu->cpuId();
    auto& iprs = globalIPRBank()[cpuId];

    // ----------------------------------------------------------------
    // 1. Log the write
    // ----------------------------------------------------------------
    /*argCpu->logIPRWrite("MM_STAT", oldValue, newValue);*/
    TRACE_LOG(QString("MM_STAT:: oldvalue: %1 - newvalue: %2").arg(oldValue).arg(newValue));

    // ----------------------------------------------------------------
    // 2. Decode MM_STAT register layout
    // ----------------------------------------------------------------

    // Fault address: bits [63:16] (page-aligned VA)
    const quint64 faultAddress = (newValue & 0xFFFFFFFFFFFF0000ULL);

    // Fault type: bits [10:8]
    const quint8 faultTypeBits = (newValue >> 8) & 0x7u;

    // Write bit: bit [0]
    const bool isWrite = (newValue & 0x1u) != 0;

    // Access type: bits [7:4] (implementation-specific flags)
    const quint8 accessFlags = (newValue >> 4) & 0xFu;

    // Additional fault info: bits [3:1]
    const quint8 AdditionalfaultInfo = (newValue >> 1) & 0x7u;

    // ----------------------------------------------------------------
    // 3. Decode to your MemoryFaultType enum
    // ----------------------------------------------------------------
    const MemoryFaultType faultType = decodeMMStatFaultType(faultTypeBits, isWrite);

    // ----------------------------------------------------------------
    // 4. Store architectural state
    // ----------------------------------------------------------------
    iprs.mm_stat = newValue;

    // ----------------------------------------------------------------
    // 5. Build complete fault information structure
    // ----------------------------------------------------------------
    MemoryFaultInfo faultInfo;
    faultInfo.faultType = faultType;
    faultInfo.faultAddress = faultAddress;
    faultInfo.faultingPC = iprs.exc_addr;  // PC saved in EXC_ADDR
    faultInfo.isWrite = isWrite;
    faultInfo.accessTypeEx = isWrite ? MemoryAccessType::WRITE : MemoryAccessType::READ;
    faultInfo.accessSizeEx = MemoryAccessSize::QUADWORD;  // Default assumption
    faultInfo.tbDomain = TB_ExceptionDomain::DTB;
    faultInfo.inPALmode = argCpu->isInPalMode();
    faultInfo.currentMode = getCM_Active(cpuId);
    faultInfo.translationValid = false;  // No PA yet on MM fault

    // ----------------------------------------------------------------
    // 6. Validate this is an actual fault (not diagnostic write)
    // ----------------------------------------------------------------
    if (!isMemoryManagementFault(faultType)) {
        DEBUG_LOG(QString("MM_STAT write: non-fault type %1").arg(faultTypeBits));
        return;
    }

    // ----------------------------------------------------------------
    // 7. Handle the fault based on type
    // ----------------------------------------------------------------
    handleMemoryManagementFault(argCpu, faultInfo);

    // ----------------------------------------------------------------
    // 8. Update fault statistics
    // ----------------------------------------------------------------
    //trackMemoryFault(argCpu, faultType, faultAddress);
}

#endif // ONMM_STAT_WRITE_H
