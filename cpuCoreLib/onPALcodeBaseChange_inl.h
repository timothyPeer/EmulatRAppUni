#ifndef ONPALCODEBASECHANGE_INL_H
#define ONPALCODEBASECHANGE_INL_H

#include "AlphaCPU.h"
#include <QtGlobal>

inline void onPalCodeBaseChange(CPUIdType cpuId, quint64 oldBase, quint64 newBase)
{
	auto& iprsExt = globalIPRHotExt(cpuId);
	auto& iprs = globalIPRHot64(cpuId);

	DEBUG_LOG(QString("PALcode base changed: 0x%1 -> 0x%2")
		.arg(oldBase, 16, 16, QChar('0'))
		.arg(newBase, 16, 16, QChar('0')));

	// ----------------------------------------------------------------
	// 1. Flush PAL instruction cache
	//    PALcode instructions may have been cached at old address
	// ----------------------------------------------------------------
#ifdef FLUSH_ICACHE_ON_PALBASE_CHANGE
	argCpu->flushICacheRange(oldBase, oldBase + 0x10000);  // 64KB PAL region
#endif

	// ----------------------------------------------------------------
	// 2. Reset PAL execution base pointer
	//    This is used by instruction fetch to know PAL address range
	// ----------------------------------------------------------------
	iprsExt.pal_base = newBase;

	// ----------------------------------------------------------------
	// 3. Reset PAL-mode PC if currently in PAL mode
	//    Prevents executing stale code at old PAL_BASE
	// ----------------------------------------------------------------
	if (iprs.isInPalMode()) {
		// If PC was relative to old PAL_BASE, adjust it
		quint64 currentPC = getPC_Active(cpuId);
		quint64 oldOffset = currentPC - oldBase;

		if (oldOffset < 0x10000ULL) {
			// PC was in old PAL region - relocate to new base
			quint64 newPC = newBase + oldOffset;
			setPC_Active(cpuId, newPC);

			DEBUG_LOG(QString("Relocated PAL PC: 0x%1 -> 0x%2")
				.arg(currentPC, 16, 16, QChar('0'))
				.arg(newPC, 16, 16, QChar('0')));
		}
	}

	// ----------------------------------------------------------------
	// 4. Flush PAL branch prediction
	//    Branch targets may have been learned at old addresses
	// ----------------------------------------------------------------
	// TODO if necessary

	// ----------------------------------------------------------------
	// 5. Invalidate PAL TLB entries (if PAL uses virtual addressing)
	//    Note: Most PALcode runs in physical mode, so this may be N/A
	// ----------------------------------------------------------------

// Invalidate ITB entries in PAL VA range
	for (quint64 va = oldBase; va < oldBase + 0x10000; va += 8192) {
		globalEv6SPAM().invalidateAllTLBs(cpuId);
	}

}
#endif // ONPALCODEBASECHANGE_INL_H
