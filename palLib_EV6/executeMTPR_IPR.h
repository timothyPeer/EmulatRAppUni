#ifndef executeMTPR_IPR_h__
#define executeMTPR_IPR_h__

#include "Pal_core_inl.h"
#include "IPRStorageHot.h"
#include "types_core.h"
#include <QtGlobal>


// ============================================================================
// executeMTPR(cpuId, iprNumber, value)
// Move To Processor Register (PAL-mode privileged write)
// ============================================================================
inline void executeMTPR(CPUIdType cpuId, quint16 iprNumber, quint64 value) noexcept
{
    auto& iprHot  = globalIPRHot(cpuId);


    switch (iprNumber)
    {
    case IPR_CC:
        // Software can reset or set the cycle counter
        iprHot.cc = value;
        break;

    case IPR_CC_CTL:
        // Only allow valid bits (ENABLE, FREEZE_PAL)
        iprHot.cc_ctl = (value & (0x1 | 0x2));
        break;

    case IPR_EXC_ADDR:
        iprHot.exc_addr = value;
        break;

    case IPR_PAL_BASE:
        // PAL base may be written in privileged PAL state
        iprHot.pal_base = value;
        break;

    case IPR_MM_STAT:
        iprHot.mm_stat = value;
        break;

    case IPR_EXC_SUM:
        // OS/VMS clear bits by writing new summary
        iprHot.exc_sum = value;
        break;

    default:
        // Unknown/unimplemented IPR → ignored (per Alpha PAL rules)
        break;
    }
}
#endif // executeMTPR_IPR_h__
