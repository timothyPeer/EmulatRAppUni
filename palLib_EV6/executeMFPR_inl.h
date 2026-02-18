#ifndef EXECUTEMFPR_INL_H
#define EXECUTEMFPR_INL_H
#include "Pal_core_inl.h"
#include "IPRStorageHot.h"
#include "types_core.h"
#include <QtGlobal>

// ============================================================================
// executeMFPR(cpuId, iprNumber)
// Move From Processor Register (PAL-mode privileged read)
// ============================================================================
inline quint64 executeMFPR(CPUIdType cpuId, quint16 iprNumber) noexcept
{
    auto& iprHot  = globalIPRHot(cpuId);

    switch (iprNumber)
    {
    case IPR_CC:
        // Cycle counter
        return iprHot.cc;

    case IPR_CC_CTL:
        // CC control bits
        return iprHot.cc_ctl;

    case IPR_EXC_ADDR:
        // Where the last exception occurred
        return iprHot.exc_addr;

    case IPR_PAL_BASE:
        // Base address for PAL entry points
        return iprHot.pal_base;

    case IPR_MM_STAT:
        return iprHot.mm_stat;

    case IPR_EXC_SUM:
        return iprHot.exc_sum;

    default:
        // Unknown or unimplemented IPR returns 0
        // (Matches many PAL behavior patterns)
        return 0;
    }
}

#endif // EXECUTEMFPR_INL_H
