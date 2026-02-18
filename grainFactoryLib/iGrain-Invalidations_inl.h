#pragma once
#include <QtGlobal>
#include "iGrain-KeyIdenties.h"
#include "iGrain-DualCache_singleton.h"


// ============================================================================
// Self-Modifying Code
// ============================================================================

void onCodeModified(quint64 pa) {
	// Invalidate by PA (hardware identity)
	PaKey paKey = PaKey::fromPA(pa);
	paDecodeCache().invalidate(paKey);
    pcDecodeCache().invalidateAll();  // conservative but safe
	// Note: PC cache may still have stale entries
	// They'll be detected as mismatches on next PA lookup
}

// ============================================================================
// Page Unmapped
// ============================================================================

void onPageUnmapped(quint64 pfn) {
	// Must invalidate entire page worth of PA entries
	const quint64 pageBase = pfn << 13;  // 8KB page
	const quint64 pageEnd = pageBase + 0x2000;

	for (quint64 pa = pageBase; pa < pageEnd; pa += 4) {
		PaKey paKey = PaKey::fromPA(pa);
		paDecodeCache().invalidate(paKey);
	}
}

// ============================================================================
// Context Switch
// ============================================================================

void onContextSwitch() {
	// PC mappings change - flush PC cache
	pcDecodeCache().invalidateAll();

	// PA cache remains valid (physical identity unchanged)
}
