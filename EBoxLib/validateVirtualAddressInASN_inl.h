#pragma once
#include "VA_core.h"
#include "../coreLib/types_core.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "../coreLib/Global_IPRInterface.h"
#include "VA_types.h"
#include "../coreLib/HWPCB_helpers_inline.h"

/*
VA is within valid ranges for the current ASN
No ASN/VA aliasing conflicts (advanced multi-process emulation)
Address space hasn't been torn down while VA is in use
*/
inline void validateVirtualAddressInASN(AlphaProcessorContext* ctx, VAType va, ASNType asn)
{
	quint8 cpuId = ctx.cpuId();
	auto& iprs = globalIPRBank()[cpuId];

	// ----------------------------------------------------------------
	// 1. Check if ASN is valid (0-255)
	// ----------------------------------------------------------------
	if (!isValidASN(asn)) {
		WARN_LOG(QString("Invalid ASN: %1 on CPU %2").arg(asn).arg(cpuId));
		return;
	}

	// ----------------------------------------------------------------
	// 2. Check if VA is within architectural limits
	// ----------------------------------------------------------------
	if (!isValidAddressInASN(va, asn)) {
		qWarning() << "VA" << Qt::hex << va << "out of range for ASN" << asn;
		return;
	}

	// ----------------------------------------------------------------
	// 3. Check if VA is canonical (not in reserved hole)
	// ----------------------------------------------------------------
	if (!isCanonicalVA(va, iprs.hot.va_ctl)) {
		DEBUG_LOG(QString("Non-canonical VA: 0x%1 (VA_CTL=0x%2)")
			.arg(va, 16, 16, QChar('0'))
			.arg(iprs.hot.va_ctl, 16, 16, QChar('0')));
	}

	// Optional: Additional per-ASN range checks
	// (for OS emulation that tracks process VA regions)
}
/*
Is this a kernel VA (bit 63 set) vs user VA?
Does this hit a superpage region (consult VA_CTL)?
Is this in a reserved or invalid VA range?
Does the VA format match VA_CTL settings (43-bit vs 48-bit)?
*/

/// \brief Analyze VA characteristics and detect privilege violations
/// \details Decodes VA format and checks against current privilege level
inline void analyzeVirtualAddressAttributes(AlphaProcessorContext& argCpu, VAType va)
{
	quint8 cpuId = argCpu.cpuId();
	auto& iprs = globalIPRBank()[cpuId];

	// ----------------------------------------------------------------
	// 1. Classify the virtual address (User/Kernel/Unknown)
	// ----------------------------------------------------------------
	AddressClass vaClass = classifyVA(va, iprs.hot.va_ctl);

	// ----------------------------------------------------------------
	// 2. Get current privilege mode (from CM or PS)
	// ----------------------------------------------------------------
	quint8 cm = getCM_Active(cpuId);
	bool isUserMode = (cm == 3);      // CM=3 is user mode
	bool isKernelMode = (cm == 0);    // CM=0 is kernel mode

	// ----------------------------------------------------------------
	// 3. Detect privilege violations
	// ----------------------------------------------------------------
	if (vaClass == AddressClass::Kernel && isUserMode) {
		// User mode accessed kernel VA -> privilege violation
		DEBUG_LOG(QString("Privilege violation: user mode (CM=%1) accessing kernel VA 0x%2")
			.arg(cm)
			.arg(va, 16, 16, QChar('0')));
	}

	if (vaClass == AddressClass::User && isKernelMode) {
		// Kernel accessing user VA (legal, but track for statistics)
		DEBUG_LOG(QString("Kernel->user VA access: 0x%1").arg(va, 16, 16, QChar('0')));
	}

	if (vaClass == AddressClass::Unknown) {
		// VA in reserved region (non-canonical)
		qWarning() << "Access to non-canonical VA:" << Qt::hex << va;
	}

	// ----------------------------------------------------------------
	// 4. Check VA mode (43-bit vs 48-bit)
	// ----------------------------------------------------------------
	bool is48bit = isVA48(iprs.hot.va_ctl);
	DEBUG_LOG(QString("VA mode: %1-bit, class: %2")
		.arg(is48bit ? 48 : 43)
		.arg(vaClass == AddressClass::User ? "User" :
			vaClass == AddressClass::Kernel ? "Kernel" : "Unknown"));

	// ----------------------------------------------------------------
	// 5. Optional: Check alignment for the access type
	// ----------------------------------------------------------------
	// Note: VA register doesn't tell us the access type, but you can
	// track the most recent memory operation if needed
#ifdef CHECK_VA_ALIGNMENT
	if (!ev6CheckAlignment(va, AccessKind::READ)) {
		DEBUG_LOG(QString("Potential alignment fault: VA 0x%1")
			.arg(va, 16, 16, QChar('0')));
	}
#endif
}



/*
Histogram of VA ranges accessed (kernel vs user)
Frequency of VA changes (indicates fault rate)
Statistics for memory profiling tools
*/
// Minimal implementation (optional):
/// \brief Track VA writes for performance analysis
/// \details Collects statistics on VA patterns and fault frequencies
inline void trackVirtualAddressWrite(AlphaProcessorContext* ctx, quint64 oldValue, quint64 newValue)
{
	quint8 cpuId = ctx.cpuId();
	auto& iprs = globalIPRBank()[cpuId];

#ifdef TRACK_VA_STATISTICS
	// ----------------------------------------------------------------
	// 1. Track VA change frequency (indicates fault rate)
	// ----------------------------------------------------------------
	if (oldValue != newValue) {
		ctx.incrementPerfCounter(PerfEvent::VA_CHANGES);
	}

	// ----------------------------------------------------------------
	// 2. Track VA range distribution
	// ----------------------------------------------------------------
	AddressClass vaClass = classifyVA(newValue, iprs.va_ctl);

	switch (vaClass) {
	case AddressClass::User:
		incrementPerfCounter(PerfEvent::USER_VA_ACCESSES);
		break;
	case AddressClass::Kernel:
		incrementPerfCounter(PerfEvent::KERNEL_VA_ACCESSES);
		break;
	case AddressClass::Unknown:
		incrementPerfCounter(PerfEvent::INVALID_VA_ACCESSES);
		break;
	}

	// ----------------------------------------------------------------
	// 3. Track TLB bank selection patterns (for dual-bank analysis)
	// ----------------------------------------------------------------
	TLB_BANK bank = selectTLBBank(newValue);
	if (bank == TLB_BANK::BANK1) {
		incrementPerfCounter(PerfEvent::TLB_BANK1_ACCESSES);
	}

	// ----------------------------------------------------------------
	// 4. Optional: Histogram VA distribution
	// ----------------------------------------------------------------
	// cpu.recordVAHistogram(newValue);

#else
	Q_UNUSED(ctx);
	Q_UNUSED(oldValue);
	Q_UNUSED(newValue);
#endif
}


/// \brief Adjust speculative execution based on VA characteristics
/// \details Controls prefetching and speculation for fault-prone regions
inline void adjustSpeculativeExecutionForVA(AlphaProcessorContext* ctx, VAType va)
{
#ifdef DETAILED_SPECULATION_CONTROL
	quint8 cpuId = ctx.cpuId();
	auto& iprs = globalIPRBank()[cpuId];

	// ----------------------------------------------------------------
	// 1. Check if VA is in a fault-prone region
	// ----------------------------------------------------------------
	if (!isCanonicalVA(va, iprs.va_ctl)) {
		// Non-canonical VA likely to fault -> suppress speculation
		ctx.suppressSpeculation();
		DEBUG_LOG("Suppressing speculation for non-canonical VA");
		return;
	}

	// ----------------------------------------------------------------
	// 2. Check for page boundary crossings
	// ----------------------------------------------------------------
	const quint64 PAGE_SIZE = 8192; // 8KB pages
	quint64 pageOffset = va & (PAGE_SIZE - 1);

	if (pageOffset >= (PAGE_SIZE - 64)) {
		// Near page boundary -> may cross to unmapped page
		ctx.reduceSpeculationDepth();
		DEBUG_LOG("Reducing speculation near page boundary");
	}

	// ----------------------------------------------------------------
	// 3. Check for privilege transitions
	// ----------------------------------------------------------------
	quint8 cm = getCM_Active(cpuId);
	AddressClass vaClass = classifyVA(va, iprs.va_ctl);

	if ((cm == 3 && vaClass == AddressClass::Kernel) ||
		(cm == 0 && vaClass == AddressClass::User)) {
		// Privilege mismatch -> likely to fault
		ctx.suppressSpeculation();
		DEBUG_LOG("Suppressing speculation for privilege mismatch");
	}

	// ----------------------------------------------------------------
	// 4. Disable prefetching for non-cacheable regions
	// ----------------------------------------------------------------
	// Note: You'd need to check the PTE's caching attributes here
	// For now, just a placeholder
	// if (isNonCacheable(va)) {
	//     cpu.disablePrefetch();
	// }

#else
	Q_UNUSED(ctx);
	Q_UNUSED(va);
	// Functional emulator: no-op
#endif
}