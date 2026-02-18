#pragma once
// ============================================================================
// PCTX_helpers.h  (header-only, corrected for IPRStorage_Hot)
// ============================================================================

#include <QtGlobal>
#include "Axp_Attributes_core.h"
#include "globalIPR_hot_cold_impl.h"

// ============================================================================
// Bit masks (EV6 / 21264)
// ============================================================================

static constexpr quint64 PCTX_ASN_MASK = 0x0007F80000000000ULL; // 46:39
static constexpr quint64 PCTX_ASTER_MASK = 0x00000000000001E0ULL; // 8:5
static constexpr quint64 PCTX_ASTRR_MASK = 0x0000000000001E00ULL; // 12:9
static constexpr quint64 PCTX_FPE_MASK = 0x0000000000000004ULL; // bit 2
static constexpr quint64 PCTX_PPCE_MASK = 0x0000000000000002ULL; // bit 1

static constexpr int PCTX_ASN_SHIFT = 39;
static constexpr int PCTX_ASTRR_SHIFT = 9;
static constexpr int PCTX_ASTER_SHIFT = 5;

// Writable architectural bits
static constexpr quint64 PCTX_DEFINED_MASK =
PCTX_ASN_MASK | PCTX_ASTER_MASK | PCTX_ASTRR_MASK |
PCTX_FPE_MASK | PCTX_PPCE_MASK;

// ============================================================================
// Storage binding (HOT)
// ============================================================================
AXP_FLATTEN inline quint64& pctxRef(CPUIdType cpuId) noexcept
{
	return globalIPRHot(cpuId).pctx;
}

// ============================================================================
// Decode helpers
// ============================================================================
AXP_FLATTEN quint8 decodeASN(quint64 p) { return (p & PCTX_ASN_MASK) >> PCTX_ASN_SHIFT; }
AXP_FLATTEN quint8 decodeASTER(quint64 p) { return (p & PCTX_ASTER_MASK) >> PCTX_ASTER_SHIFT; }
AXP_FLATTEN quint8 decodeASTRR(quint64 p) { return (p & PCTX_ASTRR_MASK) >> PCTX_ASTRR_SHIFT; }
AXP_FLATTEN bool   decodeFPE(quint64 p) { return (p & PCTX_FPE_MASK) != 0; }
AXP_FLATTEN bool   decodePPCE(quint64 p) { return (p & PCTX_PPCE_MASK) != 0; }

// ============================================================================
// Register accessors
// ============================================================================
AXP_FLATTEN quint64 getPCTX(CPUIdType cpuId) noexcept
{
	return pctxRef(cpuId);
}

AXP_FLATTEN void setPCTX(CPUIdType cpuId, quint64 v) noexcept
{
	pctxRef(cpuId) = (v & PCTX_DEFINED_MASK);
}

// ============================================================================
// Side-effect handlers (REALISTIC Alpha semantics)
// ============================================================================

AXP_FLATTEN void pctxOnAsnChange(CPUIdType cpuId,
	quint8 oldAsn,
	quint8 newAsn) noexcept
{
	auto& iprs = globalIPRHot(cpuId);

	// Update hot ASN mirror (used on every TLB lookup)
	iprs.asn = newAsn;

	// Architectural intent: invalidate non-global translations
	iprs.dtb_zap = 1;
	iprs.itb_zap = 1;
}

AXP_FLATTEN void pctxOnAstChange(CPUIdType cpuId,
	quint8 aster,
	quint8 astrr) noexcept
{
	auto& iprs = globalIPRHot(cpuId);

	iprs.aster = aster;
	iprs.astrr = astrr;

	// Recompute AST pending bits
	iprs.ast = aster & astrr;
}

AXP_FLATTEN void pctxOnFpeChange(CPUIdType cpuId,
	bool fpe) noexcept
{
	auto& iprs = globalIPRHot(cpuId);

	// Gate FP execution
	if (fpe)
		iprs.iccsr |= (1ULL << 2);  // FPE enable bit
	else
		iprs.iccsr &= ~(1ULL << 2);  // disable
}

AXP_FLATTEN void pctxOnPpceChange(CPUIdType cpuId,
	bool ppce) noexcept
{
	auto& iprs = globalIPRHot(cpuId);

	// Enable / disable per-process perf counters
	if (!ppce) {
		iprs.perf_cnt0 = 0;
		iprs.perf_cnt1 = 0;
		iprs.perf_cnt2 = 0;
	}
}

// ============================================================================
// HW_MTPR PCTX write (field-select semantics)
// ============================================================================
AXP_FLATTEN
void pctx_hw_mtpr_write(CPUIdType cpuId,
	quint8 fieldSelect,
	quint64 newValue) noexcept
{
	quint64 oldP = getPCTX(cpuId);
	quint64 p = oldP;

	if (fieldSelect & 0x01) p = (p & ~PCTX_ASN_MASK) | (newValue & PCTX_ASN_MASK);
	if (fieldSelect & 0x02) p = (p & ~PCTX_ASTER_MASK) | (newValue & PCTX_ASTER_MASK);
	if (fieldSelect & 0x04) p = (p & ~PCTX_ASTRR_MASK) | (newValue & PCTX_ASTRR_MASK);
	if (fieldSelect & 0x08) p = (p & ~PCTX_PPCE_MASK) | (newValue & PCTX_PPCE_MASK);
	if (fieldSelect & 0x10) p = (p & ~PCTX_FPE_MASK) | (newValue & PCTX_FPE_MASK);

	p &= PCTX_DEFINED_MASK;

	if (p == oldP)
		return;

	setPCTX(cpuId, p);

	// Dispatch side effects
	if ((oldP ^ p) & PCTX_ASN_MASK)
		pctxOnAsnChange(cpuId, decodeASN(oldP), decodeASN(p));

	if ((oldP ^ p) & (PCTX_ASTER_MASK | PCTX_ASTRR_MASK))
		pctxOnAstChange(cpuId, decodeASTER(p), decodeASTRR(p));

	if ((oldP ^ p) & PCTX_FPE_MASK)
		pctxOnFpeChange(cpuId, decodeFPE(p));

	if ((oldP ^ p) & PCTX_PPCE_MASK)
		pctxOnPpceChange(cpuId, decodePPCE(p));
}
