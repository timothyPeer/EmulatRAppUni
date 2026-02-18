#ifndef ONVAWRITE_INL_H
#define ONVAWRITE_INL_H
#include "AlphaCPU.h"
#include <QtGlobal>

inline void onVAWrite(AlphaCPU* argCpu, quint64 oldValue, quint64 newValue)
{
    CPUIdType cpuId = argCpu->cpuId();
    auto& iprs = globalIPRBank()[cpuId];

    // ----------------------------------------------------------------
    // 1. Update architectural state (ALWAYS required)
    // ----------------------------------------------------------------
    iprs.hot.va = newValue;

    // ----------------------------------------------------------------
    // 2. Logging (helpful for debugging)
    // ----------------------------------------------------------------
    TRACE_LOG(QString("VAWrite - VA : oldValue %1, newValue %2").arg(oldValue).arg(newValue));

#ifdef VALIDATE_VA_WRITES
    // ----------------------------------------------------------------
    // 3. Validate VA in current ASN context
    // ----------------------------------------------------------------
    quint8 currentASN = getASN_Active(cpuId);
    validateVirtualAddressInASN(argCpu, newValue, currentASN);

    // ----------------------------------------------------------------
    // 4. Analyze VA attributes and detect violations
    // ----------------------------------------------------------------
    analyzeVirtualAddressAttributes(argCpu, newValue);
#endif

#ifdef TRACK_VA_STATISTICS
    // ----------------------------------------------------------------
    // 5. Performance tracking
    // ----------------------------------------------------------------
    trackVirtualAddressWrite(argCpu, oldValue, newValue);
#endif

#ifdef DETAILED_SPECULATION_CONTROL
    // ----------------------------------------------------------------
    // 6. Adjust speculation (cycle-accurate emulation only)
    // ----------------------------------------------------------------
    adjustSpeculativeExecutionForVA(argCpu, newValue);
#endif

#ifdef PREWARM_TLB_ON_VA_WRITE
    // ----------------------------------------------------------------
    // 7. Optional: Pre-warm TLB for the faulting address
    //    (helps reduce subsequent fault latency)
    // ----------------------------------------------------------------
    VAType va = newValue;
    prepareForVATranslation(cpuId, Realm::D, 0, va);
#endif
}

#endif // ONVAWRITE_INL_H
