#ifndef IPR_IC_FLUSH_INL_H
#define IPR_IC_FLUSH_INL_H

#include "TraceHelpers.h"
#include "IPRStorageHot.h"
#include "types_core.h"
#include "../cpuCoreLib/Prefetch_inl_helper.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "IPRStorage_IBox.h"


// ============================================================================
// I_CTL write semantics (architectural)
// ----------------------------------------------------------------------------
// Enforces RO/RW masking rules and preserves hardware-defined bits.
// ============================================================================

AXP_FLATTEN
quint64 applyIctlWriteSemantics(quint64 oldRaw, quint64 newRaw) noexcept
{
	// Preserve RO fields:
	//  - CHIP_ID   bits 29:24
	//  - BIST_FAIL bit 23
	//  - SL_RCV    bit 14
	const quint64 preserved =
		(oldRaw & ((1ULL << 29) - (1ULL << 24))) | // CHIP_ID
		(oldRaw & (1ULL << 23)) |                  // BIST_FAIL
		(oldRaw & (1ULL << 14));                   // SL_RCV

	// Mask out writes to RO fields
	quint64 masked = newRaw;
	masked &= ~(0x3FULL << 24); // CHIP_ID
	masked &= ~(1ULL << 23);    // BIST_FAIL
	masked &= ~(1ULL << 14);    // SL_RCV

	return masked | preserved;
}

// ============================================================================
// Instruction frontend invalidation (I-box effects)
// ============================================================================
AXP_FLATTEN
void invalidateInstructionFrontend(AlphaProcessorContext* ctx) noexcept
{
	CPUIdType cpuId = ctx.cpuId();
	auto& iprs = globalIPRIbox(cpuId);
	
	// Architectural I-cache invalidation
// 	iprs.ic_flush;          // architectural side effect marker
// 	iprs.flushICache(ctx);  // emulator cache model reset
}

// ============================================================================
// Speculation / prefetch reset (microarchitectural)
// ============================================================================
AXP_FLATTEN
void resetInstructionSpeculation(AlphaProcessorContext& cpuState) noexcept
{
	cpuState.clearPrefetchState();
}


AXP_HOT
void writeIPR_ICTL(AlphaProcessorContext* ctx, quint64 newval) noexcept
{
	CPUIdType cpuId = ctx.cpuId();
	auto& iprs = globalIPRIbox(cpuId);

	// ---------------------------------------------------------------------
	// 1) Apply architectural write semantics to I_CTL
	// ---------------------------------------------------------------------
	iprs.i_ctl.raw =
		applyIctlWriteSemantics(iprs.i_ctl.raw, newval);

	// ---------------------------------------------------------------------
	// 2) Invalidate instruction frontend (I-cache / I-box)
	// ---------------------------------------------------------------------
	invalidateInstructionFrontend(ctx);

	// ---------------------------------------------------------------------
	// 3) Reset speculative frontend state
	// ---------------------------------------------------------------------
	resetInstructionSpeculation(ctx);

	// ---------------------------------------------------------------------
	// 4) Optional: ordering fence (modeling hook)
	// ---------------------------------------------------------------------
	// MEM_BARRIER();  // enable later if required by SMP modeling
}


#endif // IPR_IC_FLUSH_INL_H
