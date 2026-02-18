// ============================================================================
// alpha_spam_bucket.h - SPAM TLB Set-Associative Bucket
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================
//
// N-way set-associative bucket for the SPAM (Software Page Address Map)
// TLB model.  Each bucket holds up to AssocWays entries with concurrent
// insert/lookup/invalidate via a lightweight seqlock protocol.
//
// Two-axis lazy invalidation (see SPAMEpoch_inl.h):
//   Axis 1 - globalEpoch:  context-switch guard (ASM=0 entries only)
//   Axis 2 - per-ASN epoch: TBIAP / selective flush guard
//
// ASM bit semantics (Alpha AXP):
//   ASM=1 (global)  -- valid for all processes; survives context switches
//   ASM=0 (local)   -- valid only when current ASN matches
//
// Concurrency:
//   Writer (insert/invalidate) -- seqlock odd/even protocol on m_ver
//   Reader (find)              -- spin on odd, compare v0==v1 for consistency
//   Slot allocation            -- lock-free CAS on occupancy bitmap (m_occ)
//
// Audit fixes applied in this revision:
//   [B1] find(): e.flags/e.globalGenAtFill corrected to e->  (was dot on ptr)
//   [B3] sweepDeadForASN(): curGen widened from quint16 to quint32
//   [B4] Static constexpr members renamed to kWays/kMaxASN (was shadowing
//        template parameters)
//   [B5] Removed vestigial bucket-level globalGenAtFill (belongs on Entry)
//
// ============================================================================

#ifndef ALPHA_SPAM_BUCKET_H
#define ALPHA_SPAM_BUCKET_H

#include <QtCore/QtGlobal>
#include <QtCore/QAtomicInteger>
#include <QtAlgorithms>
#include <QtCore/qglobal.h>

#include "alpha_pte_core.h"         // AlphaPTE, PFNType
#include "alpha_spam_types.h"       // SPAMTag, SPAMEntry, Realm, ASNType
#include "coreLib/types_core.h"     // CPUIdType, SC_Type
#include "SPAMEpoch_inl.h"         // PerCPUEpochTable, SPAMEpoch

// ============================================================================
// SPAMBucket
// ============================================================================
//
// Template parameters:
//   Traits    -- Policy class providing Tag/Entry types and field access.
//                Must define SPAMTag<Traits>, SPAMEntry<Traits>.
//   AssocWays -- Set associativity (1..64).  Default: 4.
//   MaxASN    -- Maximum ASN count.  Must be 256 (Alpha 8-bit ASN).
// ============================================================================

template<typename Traits, unsigned AssocWays = 4, unsigned MaxASN = 256>
class SPAMBucket {
	static_assert(AssocWays >= 1 && AssocWays <= 64, "WAYS must be 1..64");
	static_assert(MaxASN == 256, "Adjust epoch table if you change MaxASN");

public:
	using Tag   = SPAMTag<Traits>;
	using Entry = SPAMEntry<Traits>;

	// [B4] Renamed from AssocWays/MaxASN to avoid shadowing template params.
	static constexpr unsigned kWays   = AssocWays;
	static constexpr unsigned kMaxASN = MaxASN;

private:
	// --------------------------------------------------------------------
	// Data members
	// --------------------------------------------------------------------

	// Occupancy bitmap.  Bit N = 1 means m_entries[N] is occupied.
	// Only the lowest kWays bits are meaningful.
	QAtomicInteger<quint64> m_occ{ 0 };

	// Seqlock version counter.
	// Even = no write in progress, Odd = write in progress.
	// Writer: fetchAndAddRelease before/after mutation.
	// Reader: loadAcquire, spin while odd, compare v0==v1.
	QAtomicInteger<quint32> m_ver{ 0 };

	// [B5] Removed vestigial bucket-level globalGenAtFill.
	// The field belongs on SPAMEntry (stamped at insert, checked at lookup).

	// Bitmask with the lowest kWays bits set.
	// Special case: kWays==64 uses ~0ULL to avoid UB from (1ULL << 64).
	static constexpr quint64 FULL_MASK =
		(AssocWays == 64) ? ~0ULL : ((1ULL << AssocWays) - 1ULL);

	// Pointer to the per-CPU epoch table.  Assigned once via
	// attachEpochTable() during SPAMShardManager construction.
	// The bucket does not own this pointer.
	const PerCPUEpochTable* m_epochTable = nullptr;

	// TLB realm (Instruction or Data) this bucket belongs to.
	// Determines which per-ASN epoch array to read.
	Realm m_realm = Realm::D;

public:
	// --------------------------------------------------------------------
	// Construction and configuration
	// --------------------------------------------------------------------

	SPAMBucket() = default;

	// Attach the CPU-local epoch table and set the realm.
	// Called once per bucket during SPAMShardManager construction.
	//
	// @param tbl   Per-CPU epoch table pointer (must outlive the bucket).
	// @param realm Realm::I for instruction-stream, Realm::D for data-stream.
	void attachEpochTable(const PerCPUEpochTable* tbl, Realm realm) noexcept
	{
		m_epochTable = tbl;
		m_realm = realm;
	}

	// Entry storage.  Public for direct access by victim-selection
	// policies (e.g. SRRIP age bits).  Validity is governed by m_occ
	// and flags.valid, not by array membership alone.
	Entry m_entries[AssocWays]{};

private:
	// --------------------------------------------------------------------
	// Private helpers
	// --------------------------------------------------------------------

	// Read the current per-ASN generation for this bucket's realm.
	// Returns 0 if no epoch table is attached (unit-test fallback).
	inline quint32 currentAsnGen(ASNType asn) const noexcept {
		if (!m_epochTable) return 0u;
		return SPAMEpoch::getCurrent(*m_epochTable, m_realm, asn);
	}

	// Lock-free CAS loop to claim the first free slot.
	// Uses count-trailing-zeros to find the lowest clear bit in O(1).
	//
	// @param[out] idx  Slot index [0..kWays-1] on success.
	// @return          true if claimed, false if bucket is full.
	bool tryClaimSlot(unsigned& idx) noexcept {
		for (;;) {
			const quint64 cur = m_occ.loadRelaxed();
			const quint64 used = cur & FULL_MASK;
			if (used == FULL_MASK) return false;
			const quint64 freeBits = (~used) & FULL_MASK;
			const int bit = qCountTrailingZeroBits(freeBits);
			const quint64 want = cur | (1ULL << bit);
			if (m_occ.testAndSetRelaxed(cur, want)) {
				idx = static_cast<unsigned>(bit);
				return true;
			}
		}
	}

	// Seqlock: begin write (EVEN -> ODD).
	inline void beginWrite() noexcept { m_ver.fetchAndAddRelease(1); }

	// Seqlock: end write (ODD -> EVEN).
	inline void endWrite()   noexcept { m_ver.fetchAndAddRelease(1); }

public:
	// --------------------------------------------------------------------
	// Insert
	// --------------------------------------------------------------------

	// Insert a TLB entry into this bucket.
	//
	// Claims a free slot via the occupancy bitmap.  Returns false if the
	// bucket is full (caller may sweep dead entries and retry).
	//
	// The entry is written in a two-phase commit under the seqlock:
	//   1. Copy entry with valid=false.
	//   2. Set valid=true on the slot copy.
	// This ensures find() never sees a partially-written valid entry.
	//
	// Preconditions (enforced by the manager):
	//   - e.globalGenAtFill is set to current globalEpoch.
	//   - e.asnGenAtFill is set to current realm epoch for e.asn.
	//   - e.tag, e.pfn, e.permMask, e.sizeClass, e.flags are populated.
	//   - e.syncDerivedFromSizeClass() has been called.
	//
	// @param e  Entry to insert (by value; caller's copy is not modified).
	// @return   true if inserted, false if full.
	bool insert(Entry e) noexcept {
		unsigned slot;
		if (!tryClaimSlot(slot)) return false;

		beginWrite();
		e.flags.transitioning = false;
		e.flags.valid = false;
		m_entries[slot] = e;
		m_entries[slot].flags.valid = true;
		endWrite();

		return true;
	}

	// --------------------------------------------------------------------
	// Invalidation
	// --------------------------------------------------------------------

	// Invalidate all entries.  Brute-force path used by TBIA.
	void invalidateAll() noexcept {
		for (unsigned i = 0; i < kWays; ++i) {
			invalidate(i);
		}
	}

	// Invalidate a single slot by index.
	//
	// Marks the entry invalid and clears its occupancy bit under the
	// seqlock.  The occupancy-bit clear is a CAS loop because another
	// thread may be claiming/clearing a different slot concurrently.
	//
	// @param slot  Way index [0..kWays-1].  Out-of-range is ignored.
	void invalidate(unsigned slot) noexcept {
		if (slot >= kWays) return;

		beginWrite();
		m_entries[slot].flags.valid = false;

		quint64 cur;
		const quint64 mask = ~(1ULL << slot);
		do {
			cur = m_occ.loadRelaxed();
		} while (!m_occ.testAndSetRelaxed(cur, cur & mask));

		endWrite();
	}

	// Invalidate the first entry whose tag matches, respecting ASN/epoch.
	//
	// A tag-matched entry is considered a match if:
	//   - It is global (ASM=1), OR
	//   - Its ASN matches AND its asnGenAtFill matches the current epoch.
	//
	// Only the first match is invalidated (tags are unique in a correctly
	// operating system).
	//
	// Used by the manager for TBIS / TBISD / TBISI.
	//
	// @param tag  Tag to match.
	// @param asn  Current Address Space Number.
	// @return     true if an entry was invalidated.
	bool invalidateMatching(const Tag& tag, ASNType asn) noexcept {
		const quint64 used = m_occ.loadRelaxed() & FULL_MASK;

		for (unsigned i = 0; i < kWays; ++i) {
			if (((used >> i) & 1ULL) == 0ULL) continue;

			Entry* e = &m_entries[i];
			if (!e->flags.valid) continue;

			if (e->tag == tag) {
				const bool match =
					e->flags.global ||
					(e->asn == asn &&
					 e->asnGenAtFill == currentAsnGen(asn));

				if (match) {
					invalidate(i);
					return true;
				}
			}
		}
		return false;
	}

	// --------------------------------------------------------------------
	// Eager sweep (optional slot reclamation)
	// --------------------------------------------------------------------

	// Reclaim slots holding stale entries for a specific ASN.
	//
	// After the manager bumps the epoch for an ASN, entries filled under
	// the old generation are lazily dead but still occupy slots.  This
	// method explicitly invalidates them so they can be reused.
	//
	// Sweep is OPTIONAL for correctness.  Called by the manager:
	//   - After invalidateTLBsByASN() as proactive cleanup.
	//   - After a failed insert() as last-resort reclamation.
	//
	// [B3] curGen is quint32 to match epoch counter width.
	//      Was quint16 -- silently truncated after 65536 bumps.
	//
	// @param asn     ASN whose stale entries should be swept.
	// @param curGen  Current generation for this ASN (after bump).
	void sweepDeadForASN(quint8 asn, quint32 curGen) noexcept {
		const quint64 used = m_occ.loadRelaxed() & FULL_MASK;

		for (unsigned i = 0; i < kWays; ++i) {
			if (((used >> i) & 1ULL) == 0ULL)
				continue;

			Entry* e = &m_entries[i];

			if (!e->flags.valid || e->flags.transitioning)
				continue;
			if (e->flags.global)
				continue;
			if (e->asn != asn)
				continue;

			if (e->asnGenAtFill != curGen) {
				invalidate(i);
			}
		}
	}

	// --------------------------------------------------------------------
	// Probe (boolean existence check -- TBCHK fast path)
	// --------------------------------------------------------------------

	// Lightweight existence check: "does a live entry with this tag exist?"
	//
	// Functionally equivalent to (find(tag, asn) != nullptr) but avoids:
	//   - Returning an Entry* the caller does not need.
	//   - Debug assertions that validate PFN/sizeClass on the hit path.
	//   - Any output-parameter writes.
	//
	// Uses the occupancy bitmap as a level-0 fast reject: if the bucket
	// is empty (m_occ == 0) the call returns false without entering the
	// seqlock at all.  This matters because tbchkProbe() scans up to
	// 16 buckets (4 GH x 2 global x 2 realms) and most will be misses.
	//
	// The seqlock and both epoch axes are still checked for correctness.
	//
	// @param tag  Tag to probe for.
	// @param asn  Current Address Space Number.
	// @return     true if a live entry exists, false otherwise.
	bool probe(const Tag& tag, quint8 asn) const noexcept {
		// Level 0: bitmap fast-reject.
		// Single relaxed load -- if no slots are occupied, nothing
		// can possibly match.  Avoids the seqlock entirely.
		if ((m_occ.loadRelaxed() & FULL_MASK) == 0ULL)
			return false;

		// Level 1: seqlock-protected scan with epoch checks.
		for (;;) {
			const quint32 v0 = m_ver.loadAcquire();
			if (v0 & 1u) continue;          // write in progress -- spin

			const quint64 used = m_occ.loadRelaxed() & FULL_MASK;

			for (unsigned i = 0; i < kWays; ++i) {
				if (((used >> i) & 1ULL) == 0ULL) continue;

				const Entry* e = &m_entries[i];

				if (!e->flags.valid || e->flags.transitioning) continue;
				if (!(e->tag == tag)) continue;

				// Axis 1: Global epoch -- context-switch guard.
				if (!e->flags.global &&
					e->globalGenAtFill !=
						m_epochTable->globalEpoch.load(
							std::memory_order_relaxed))
				{
					return false;           // lazily dead (tag is unique)
				}

				// Axis 2: Per-ASN epoch -- TBIAP guard.
				const bool ok = e->flags.global ||
					(e->asn == asn &&
					 e->asnGenAtFill == currentAsnGen(asn));
				if (!ok) continue;

				// Seqlock consistency check.
				const quint32 v1 = m_ver.loadAcquire();
				if (v0 == v1)
					return true;            // consistent hit
				break;                      // torn read -- restart
			}

			const quint32 v1 = m_ver.loadAcquire();
			if (v0 == v1) return false;     // consistent miss
		}
	}

	// --------------------------------------------------------------------
	// Lookup
	// --------------------------------------------------------------------

	// Look up an entry by tag and ASN.  This is the TLB hot-path.
	//
	// Algorithm:
	//   1. Acquire seqlock snapshot (v0).  Spin while odd.
	//   2. For each occupied slot:
	//      a. Skip invalid / transitioning entries.
	//      b. Skip tag mismatches.
	//      c. Axis 1 -- Global epoch check:
	//         Non-global (ASM=0) entries with stale globalGenAtFill are
	//         dead.  Return nullptr (the tag matched, so no other entry
	//         can match -- tags are unique).
	//      d. Axis 2 -- Per-ASN epoch check:
	//         Global entries pass.  Non-global entries must have matching
	//         ASN and asnGenAtFill.
	//      e. Seqlock consistency check (v1 == v0).
	//         Consistent: return hit.  Torn: restart scan.
	//   3. After a consistent full scan with no match, return nullptr.
	//
	// @param tag  Tag to search for (VPN + sizeClass + realm + global).
	// @param asn  Current Address Space Number.
	// @return     Pointer to the matching entry, or nullptr on miss.
	//             Valid only until the next insert/invalidate on this bucket.
	//
	// [B1] Fixed: e.flags/e.globalGenAtFill corrected to e-> (pointer).
	Entry* find(const Tag& tag, quint8 asn) noexcept {
		for (;;) {
			const quint32 v0 = m_ver.loadAcquire();
			if (v0 & 1u) continue;

			const quint64 used = m_occ.loadRelaxed() & FULL_MASK;

			for (unsigned i = 0; i < kWays; ++i) {
				if (((used >> i) & 1ULL) == 0ULL) continue;

				Entry* e = &m_entries[i];

				if (!e->flags.valid || e->flags.transitioning) continue;
				if (!(e->tag == tag)) continue;

				// Axis 1: Global epoch -- context-switch guard.
				// [B1] Fixed: was e.flags.global / e.globalGenAtFill
				if (!e->flags.global &&
					e->globalGenAtFill !=
						m_epochTable->globalEpoch.load(
							std::memory_order_relaxed))
				{
					return nullptr;     // lazily dead
				}

				// Axis 2: Per-ASN epoch -- TBIAP guard.
				const bool ok = e->flags.global ||
					(e->asn == asn &&
					 e->asnGenAtFill == currentAsnGen(asn));
				if (!ok) continue;

				// Seqlock consistency check.
				const quint32 v1 = m_ver.loadAcquire();
				if (v0 == v1) {
					Q_ASSERT(e->tag.sizeClass == e->sizeClass);
					Q_ASSERT(e->flags.valid);
					return e;           // consistent hit
				}
				break;                  // torn read -- restart
			}

			const quint32 v1 = m_ver.loadAcquire();
			if (v0 == v1) return nullptr;   // consistent miss
		}
	}
};

#endif // ALPHA_SPAM_BUCKET_H
