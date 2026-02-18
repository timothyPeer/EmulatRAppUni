#pragma once
#include <QtGlobal>
#include "AlphaPTE_Core.h"
#include "alpha_pte_core.h"
#include "Alpha_spam_types.h"



// Forward declare to break cycle
class Ev6SiliconConfig;

// Forward declare SPAM types (don't include Alpha_spam_types.h yet)
// template<typename Traits>
// struct SPAMTag;

template<typename Traits>
struct SPAMEntry;

/*using Tag = SPAMTag<Ev6PTETraits>;*/
// ============================================================================
// EV6 PTETraits -  Concrete traits type for EV6
// ============================================================================

struct Ev6PTETraits
{
	using PTE = AlphaPTE;
	using Realm = ::Realm;

	// Forward-declare our own Tag/Entry (will be defined in Alpha_spam_types.h)
	using Tag = SPAMTag<Ev6PTETraits>;
	using Entry = SPAMEntry<Ev6PTETraits>;

	// Page shift helper (GH -> pageShift).
	// Source ref: ASA/SRM PTE.GH definition (TB block hint).
	static inline quint64 pageShiftFromGH(SC_Type gh) noexcept
	{
		return PageSizeHelpers::pageShift(gh); // GH 0..3
	}

	// Tag construction helper (canonical SPAMTag)
	static inline Tag makeTag(VAType va, Realm realm, SC_Type gh, bool global) noexcept
	{
		// Validate inputs
		Q_ASSERT(gh <= 3);  // GH must be 0-3
		Q_ASSERT(realm == Realm::D || realm == Realm::I);
		Tag t{};
		const quint64 shift = PageSizeHelpers::pageShift(gh);

		// SPAMTag stores vpn = va >> pageShift, sizeClass = GH, realm = D/I
		t.vpn = (va >> shift);
		t.sizeClass = gh;
		t.realm = realm;
		t.matchAllASNs = global;
		// Assert: VPN was computed correctly
		Q_ASSERT(t.sizeClass == gh);
		Q_ASSERT((t.vpn << shift) <= va);  // VPN << shift is page base

		return t;
	}

	// PFN extraction
	static inline PFNType pfn(const AlphaPTE& pte) noexcept {
		return pte.pfn();
	}

	// Permission mask extraction
	static inline AlphaN_S::PermMask permMask(const AlphaPTE& pte) noexcept {
		// Build compact permission mask from PTE
		AlphaN_S::PermMask mask = 0;
		bool kre, ere, sre, ure;
		bool kwe, ewe, swe, uwe;

		pte.getReadPermissions(kre, ere, sre, ure);
		pte.getWritePermissions(kwe, ewe, swe, uwe);

		// Pack into mask (customize to your layout)
		if (ure) mask |= (1 << 0);
		if (uwe) mask |= (1 << 1);
		if (kre) mask |= (1 << 2);
		if (kwe) mask |= (1 << 3);

		return mask;
	}
};


// EV6-specific front-end tag with gentle compatibility glue to SPAMTag<Ev6PTETraits>.
struct Ev6TLBTag
{
	VPNType vpn;        // Virtual Page Number
	quint8  realm;      // 0 = DTB, 1 = ITB (must match Traits::Realm)
	SC_Type  sizeClass;  // GH block size class (0..3)
	quint8  bank;       // reserved for dual-bank support (currently ignored by SPAM)
	bool    matchAllASNs; // match all ASNs when true
	inline bool isValid() const noexcept {
		return vpn != 0 && (realm == 0 || realm == 1);
	}

	inline void clear() noexcept
	{
		vpn = 0;
		realm = 0;
		sizeClass = 0;
		bank = 0;
		matchAllASNs = false;
	}

	using Traits = Ev6PTETraits;
	using SPTag = SPAMTag<Traits>;

	// Default
	Ev6TLBTag() = default;

	// Construct from a SPAMTag (useful if SPAM code ever hands tags back)
	explicit Ev6TLBTag(const SPTag& t) noexcept
		: vpn(t.vpn),
		realm(static_cast<quint8>(t.realm)),
		sizeClass(t.sizeClass),
		bank(0),
		matchAllASNs(t.matchAllASNs)
	{
	}

	// Implicit conversion TO SPAMTag so SPAMShardManager/Bucket can accept Ev6TLBTag.
	operator SPTag() const noexcept
	{
		SPTag t{};
		t.vpn = vpn;
		t.realm = static_cast<typename Traits::Realm>(realm);
		t.sizeClass = sizeClass;
		t.matchAllASNs = matchAllASNs;
		return t;
	}
};


