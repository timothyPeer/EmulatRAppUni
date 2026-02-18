#pragma once
#include <QtGlobal>
#include <QString>
#include "types_core.h"
#include "alpha_pte_core.h"
#include "AlphaPTE_Core.h"
#include "enum_header.h"
#include "define_helpers.h"
#include "HWPCB_helpers_inline.h"
#include "Ev6PTETraits.h"



// Pipeline and TLB Prefetch

	/// \brief Pre-warm (speculatively probe/fill) the TLB for a virtual address.
	/// \param va The virtual address to prewarm.
	/// \details 
	/// - Calls tlbLookup() and, on miss, triggers a page table walk.
	/// - For functional emulation, may be a no-op.  
	/// - For cycle/timing-accurate emulation, logs or enqueues an event.
	/// - No architectural state is changed unless TLB fill is modeled.
	/// \see Alpha AXP System Reference Manual, Ch. 4
// inline void prewarmTLB(quint16 cpuId, Realm realm, quint8 sizeClass, quint64 va) noexcept {
// 	quint32 pfn;
// 	PermMask perm;
// 
// 
// 	auto& silicon = Ev6SiliconTLB_Singleton().instance();
// 
// 	// Get reference to SPAMShardManager (TLB hardware implementation)
// 	auto& mgr = silicon.spam();
// 
// 	const quint8 asn = getASN_Active(cpuId);
// 	bool hit = mgr.tlbLookup(cpuId, realm, sizeClass, va, asn, pfn, perm);
// 
// 	if (!hit) {
// 		DEBUG_LOG("TLB_PREFETCH_MISS", "VA: 0x" << std::hex << va);
// 
// 		// 2) Optional: perform a page-table walk and insert the entry now.
// 		//    This is the part where you "influence prefetch behavior".
// 		//
// 		//    ASA ref: Alpha AXP System Reference Manual,
// 		//    Section 3.7 / 3.8 (Translation Buffer and TBMISS handling).
// 		//
// 		AlphaPTE pte = walkPageTable_EV6(cpuId, va, asn);
// 		mgr.tlbInsert(cpuId, realm, sizeClass, va, asn, /*global=*/false, pte);
// 	}
// 	else {
// 		DEBUG_LOG("TLB_PREFETCH_HIT", "VA: 0x" << std::hex << va);
// 	}
// }

inline quint64 tagToVA(const Ev6TLBTag& tag) noexcept {
	const quint64 shift = Ev6PTETraits::pageShiftForClass(tag.sizeClass);
	return tag.vpn << shift;
}



/// \brief Simulate a speculative prefetch or fill of the TLB for the given VA.
///
/// This stub does nothing in the base implementation. On a real Alpha CPU,
/// the instruction fetch, decode, or prefetch logic may attempt to pre-load
/// the TLB in anticipation of a likely upcoming reference, to reduce latency.
///
/// - In a timing-accurate emulator, you may want to enqueue a TLB lookup or
///   prefill action, or simulate a TLB probe latency.
/// - For functional emulators, this is typically a no-op.
///
/// \param va Virtual address for which the TLB pre-warming is simulated.
/// \see Alpha AXP System Reference Manual, Ch. 4, p. 4-14 (TLB lookups, prefetch)
///
inline void prewarmTLB(VAType va) noexcept
{
	Q_UNUSED(va);
	// TODO: Implement TLB prefetch, speculative fill, or logging if desired.
	// For now, this stub does nothing.
	//
	// In a detailed simulator, you might:
	// - Schedule a TLB lookup event for 'va'
	// - Log the speculative prefetch for tracing
	// - Prime a software TLB cache
	//
	// Example:
	// speculativeTLBProbe(va);
	// traceLog("prewarmTLB: va=%016llx", va);
}


AXP_FLATTEN constexpr SC_Type decodePTEPageSize(const AlphaPTE& pte) noexcept
{
	// PTE<6:5> is GH (granularity hint) and is the authoritative sizeClass for TB entry fill.
	// Ref: SRM v6.0, PTE bits 6-5 = GH.
	return static_cast<SC_Type>((raw() >> 5) & 0x3ULL);
}

AXP_FLATTEN
static bool isGlobalPTE(const AlphaPTE& pte) noexcept
{
	// ASA: ASM=1 -> matches all ASNs (global mapping)
	return pte.isGlobal();
}

// In permissions_helper.h (or a similar central location)

inline bool ev6HasPermission(const AlphaPTE& pte,
	AccessKind access,
	Mode_Privilege mode) noexcept
{
	// Fault-on-* bits
	const bool for_ = pte.bitFOR(); //  (AlphaPTE::PTE_BIT_FOR);
	const bool fow = pte.bitFOW(); //  (AlphaPTE::PTE_BIT_FOW);

	// Basic access type checks
	switch (access)
	{
	case AccessKind::EXECUTE:
// 		if (foe) return false;  // foe is not a bit member of the DTB-PTE
// 		break;
	case AccessKind::READ:
		if (for_) return false;
		break;
	case AccessKind::WRITE:
		if (fow) return false;
		break;
	default:
		break;
	}

	// Mode-specific read/write enables
	const bool kre = pte.bitKRE(); // (AlphaPTE::PTE_BIT_KRE);
	const bool ere = pte.bitERE(); // (AlphaPTE::PTE_BIT_ERE);
	const bool sre = pte.bitSRE(); //  (AlphaPTE::PTE_BIT_SRE);
	const bool ure = pte.bitURE(); //  (AlphaPTE::PTE_BIT_URE);

	const bool kwe = pte.bitKWE(); //  (AlphaPTE::PTE_BIT_KWE);
	const bool ewe = pte.bitEWE(); //  (AlphaPTE::PTE_BIT_EWE);
	const bool swe = pte.bitSWE(); //  (AlphaPTE::PTE_BIT_SWE);
	const bool uwe = pte.bitUWE(); //  (AlphaPTE::PTE_BIT_UWE);

	bool canRead = false;
	bool canWrite = false;

	switch (mode)
	{
	case Mode_Privilege::Kernel:
		// Compress E/S + K for now (your emulator already treats E/S as K)
		canRead = kre || ere || sre;
		canWrite = kwe || ewe || swe;
		break;
	case Mode_Privilege::Executive:
		canRead = ere || kre;
		canWrite = ewe || kwe;
		break;
	case Mode_Privilege::Supervisor:
		canRead = sre || ere || kre;
		canWrite = swe || ewe || kwe;
		break;
	case Mode_Privilege::User:
		canRead = ure;
		canWrite = uwe;
		break;
	default:
		// If mode is unknown, be conservative
		return false;
	}

	switch (access)
	{
	case AccessKind::EXECUTE:
		// Treat EXEC as "needs read permission" plus FOE check above
		return canRead;
	case AccessKind::READ:
		return canRead;
	case AccessKind::WRITE:
		return canWrite;
	default:
		return false;
	}
}

inline bool ev6HasPermission(AlphaN_S::PermMask perm,
                             AccessKind access,
                             Mode_Privilege mode) noexcept
{
    // Extract bits directly from the 8-bit perm mask
    const bool kre = AlphaN_S::kre(perm);
    const bool ere = AlphaN_S::ere(perm);
    const bool sre = AlphaN_S::sre(perm);
    const bool ure = AlphaN_S::ure(perm);

    const bool kwe = AlphaN_S::kwe(perm);
    const bool ewe = AlphaN_S::ewe(perm);
    const bool swe = AlphaN_S::swe(perm);
    const bool uwe = AlphaN_S::uwe(perm);

    // Alpha treats EXEC as READ + FOE check (FOE is handled earlier)
    bool canRead  = false;
    bool canWrite = false;

    switch (mode)
    {
    case Mode_Privilege::Kernel:
        canRead  = kre || ere || sre;
        canWrite = kwe || ewe || swe;
        break;

    case Mode_Privilege::Executive:
        canRead  = ere || kre;
        canWrite = ewe || kwe;
        break;

    case Mode_Privilege::Supervisor:
        canRead  = sre || ere || kre;
        canWrite = swe || ewe || kwe;
        break;

    case Mode_Privilege::User:
        canRead  = ure;
        canWrite = uwe;
        break;

    default:
        return false;
    }

    switch (access)
    {
    case AccessKind::EXECUTE:
        return canRead;  // FOE already handled in caller
    case AccessKind::READ:
        return canRead;
    case AccessKind::WRITE:
        return canWrite;
    default:
        return false;
    }
}




inline AccessResult checkAccess(CPUIdType cpuId, VAType va, AccessIntent intent, quint8 currentMode, quint8 permMask   // AccessBits mask from PTE (USER/KERN R/W/X)
) noexcept
{
	Mode_Privilege privMode = static_cast<Mode_Privilege>(currentMode);
	return ::checkAccess(cpuId, va, intent, privMode, permMask);
}

inline AccessResult checkAccess(VAType va, AccessIntent intent, Mode_Privilege currentMode, quint8 permMask) noexcept // AccessBits mask from PTE (USER/KERN R/W/X)
{
	// ---------------------------------------------------------------------
	// 1. Translation Failure / No Page?  (permMask = 0 means unmapped)
	// ---------------------------------------------------------------------
	if (permMask == 0) {
		return AccessResult::Fault_NoPage;
	}

	// ---------------------------------------------------------------------
	// 2. Determine required bits based on access type AND processor mode
	// ---------------------------------------------------------------------
	quint8 required = 0;

	switch (intent)
	{
	case AccessIntent::Read:
		required = (currentMode == Mode_Privilege::Kernel)
			? KERNEL_READ
			: USER_READ;
		break;

	case AccessIntent::Write:
		required = (currentMode == Mode_Privilege::Kernel)
			? KERNEL_WRITE
			: USER_WRITE;
		break;

	case AccessIntent::Execute:
		required = (currentMode == Mode_Privilege::Kernel)
			? KERNEL_EXEC
			: USER_EXEC;
		break;

	default:
		return AccessResult::Fault_Unknown;
	}

	// ---------------------------------------------------------------------
	// 3. Test whether PTE permission bits allow this access
	// ---------------------------------------------------------------------
	if ((permMask & required) == 0) {
		// If execute denied:
		if (intent == AccessIntent::Execute)
			return AccessResult::Fault_Execution;

		// If write denied:
		if (intent == AccessIntent::Write)
			return AccessResult::Fault_Write;

		// Otherwise Read denied:
		return AccessResult::Fault_Permission;
	}

	// ---------------------------------------------------------------------
	// 4. Optional: FEN (Floating Point Enable) enforcement
	//    If intent involves FP register memory access or FP instruction fetch,
	//    check fenEnabled() here.
	// ---------------------------------------------------------------------
	// if (intent == AccessIntent::Execute && isFPInstruction && !fenEnabled())
	//     return AccessResult::Fault_FEN;

	// ---------------------------------------------------------------------
	// 5. Optional: Alignment checks (Alpha faults on misalignment)
	//    Only if your emulator performs alignment checks at this layer.
	// ---------------------------------------------------------------------
	// if ((va & alignmentMask) != 0)
	//     return AccessResult::Fault_Alignment;

	// ---------------------------------------------------------------------
	// Access Allowed
	// ---------------------------------------------------------------------
	return AccessResult::Allowed;
}
inline AccessResult checkAccess(CPUIdType cpuId, VAType va, AccessIntent intent, quint8 permMask    // produced by TLB/PTE translation
)  noexcept
{
	quint8 cm = getCM_Active(cpuId);
	return checkAccess(cpuId, va, intent, cm, permMask);
}

inline static AlphaPTE fromPFNAndPerm(PFNType pfn, AlphaN_S::PermMask perm) noexcept
{
	AlphaPTE pte;
	pte.setPFN(pfn);
	pte.setPermMask(perm);
	pte.setValid(true);
	return pte;
}