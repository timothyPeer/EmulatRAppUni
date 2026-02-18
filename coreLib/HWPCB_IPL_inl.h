#ifndef _EMULATRAPPUNI_CORELIB_HWPCB_IPL_INL_H
#define _EMULATRAPPUNI_CORELIB_HWPCB_IPL_INL_H


// ========================================================================
    // Interrupt Priority Level (IPL)
    // ========================================================================

#include <qtypes.h>

#include "IRQController.h"
#include "types_core.h"
#include "HWPCB_core.h"

// HWPCB keeps the cache for the hot read path (cache line 0)
AXP_HOT AXP_ALWAYS_INLINE
quint8 getIPL(HWPCB* hwpcb) const noexcept
{
    return hwpcb->ipl;  // fast: cache line 0
}

// ALL writes go through this single function which syncs both
// This is the ONLY way to change IPL — no direct setIPL_Internal

AXP_HOT AXP_ALWAYS_INLINE
void setIPL(quint8 newIPL, IRQController* irq, CPUIdType cpuId, HWPCB* hwpcb) noexcept
{
    const quint8 masked = newIPL & 0x1F;

    // Update PS bits 7:3
    hwpcb->ps = (hwpcb->ps & ~0xF8ULL) | (static_cast<quint64>(masked) << 3);

    // Update hot cache
    hwpcb->ipl = masked;

    // Sync source of truth
    irq->setCPUIpl(cpuId, masked);
}
#endif
