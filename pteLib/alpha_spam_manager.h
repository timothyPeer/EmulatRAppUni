// ============================================================================
// alpha_spam_manager.h - SPAM TLB Shard Manager
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
// Top-level TLB manager for the SPAM (Software Page Address Map) model.
// Organises SPAMBucket instances into a 4-dimensional shard array:
//
//   m_shards[cpuId][realm][sizeClass][bucketIndex]
//
// Provides the full Alpha AXP TLB instruction set:
//   - tlbInsert()                  -- Fill from PTE
//   - tlbLookup() / WithKnownGH() -- Probe (hot-path)
//   - tbchkProbe()                 -- TBCHK instruction
//   - tbisInvalidate()             -- TBIS  (both ITB+DTB, all GH)
//   - tbisdInvalidate()            -- TBISD (DTB only, both banks)
//   - tbisiInvalidate()            -- TBISI (ITB only)
//   - invalidateAllTLBs()          -- TBIA  (nuke one CPU)
//   - invalidateTLBsByASN()        -- TBIAP (epoch bump + optional sweep)
//   - invalidateTLBsByASN_AllCPUs() -- Cross-CPU TBIAP (IPI path)
//   - invalidateNonASM()           -- Context-switch epoch bump
//
// Page-size (GH) validation rules:
//   Rule 2.1/3.1 -- GH is extracted from the PTE (source of truth).
//   Rule 2.2     -- VA and PFN alignment are checked; misaligned
//                    superpage PTEs are degraded to 8KB (GH=0).
//   Rule 4.4     -- Validation may only REDUCE GH, never increase.
//   Rule 4.5     -- Tag uses the validated GH verbatim.
//   Rule 5.1     -- Lookup probes all GH values (largest first) when
//                    the page size is unknown.
//   Rule 5.2     -- Lookup with known GH probes only that size class.
//
// Audit fixes applied in this revision:
//   [B2] tlbInsert() now sets entry.globalGenAtFill before insert.
//        Without this, all non-global entries become lazily dead after
//        the first context switch (globalEpoch goes to 1, entries stay 0).
//   [E2] invalidateNonASM() now delegates to SPAMEpoch::bumpGlobal().
//   [B4] References to Bucket::AssocWays updated to Bucket::kWays.
//
// GH Coverage Bitmap (VA Format Contract optimization):
//   The Alpha VA Format Contract (ASA v6, Memory Management) states:
//     "Do NOT scan VA options on every lookup."
//   While the base page size (8KB, pageShift=13) is fixed at EV6
//   implementation time, GH superpages (64KB/512KB/4MB) are per-PTE
//   attributes that must be probed on lookup.
//
//   The m_ghCoverage[cpu][realm] bitmap encodes 8 distinct (GH, global)
//   pairs in a single byte:
//     Low nibble  [3:0] -- non-global (ASM=0) entries per GH 0..3
//     High nibble [7:4] -- global     (ASM=1) entries per GH 0..3
//
//   Lookups skip pairs whose bit is clear.  Probe reduction:
//     Pure userspace       0b00000001  1 probe  (was 8)
//     Kernel + userspace   0b00010001  2 probes (was 8)
//     Kernel w/largepage   0b01010001  3 probes (was 8)
//     Saturated (all 8)    0b11111111  8 probes + 1 byte overhead
//
//   The bitmap is conservative: bits set on insert, only cleared on
//   TBIA.  False negatives are impossible.  A stale set-bit costs
//   one extra empty-bucket probe (killed by m_occ fast-reject).
//
// ============================================================================

#ifndef ALPHA_SPAM_MANAGER_H
#define ALPHA_SPAM_MANAGER_H

#include <QtCore/QtGlobal>

#include "alpha_spam_types.h"       // SPAMTag, SPAMEntry, Realm, ASNType
#include "alpha_spam_bucket.h"      // SPAMBucket
#include "alpha_pte_core.h"        // AlphaPTE (low-level PTE access)
#include "AlphaPTE_Core.h"         // AlphaPTE high-level wrapper
#include "Ev6PTETraits.h"          // EV6-specific PTE traits
#include "coreLib/types_core.h"    // CPUIdType, SC_Type, PFNType
#include "TemplatePolicyBase.h"    // SRRIPPolicy, DefaultInvalidationStrategy
#include "SPAMEpoch_inl.h"        // PerCPUEpochTable, SPAMEpoch

// ============================================================================
// SPAMShardManager
// ============================================================================
//
// Template parameters:
//   Traits              -- PTE traits (tag/entry construction, field access).
//   AssocWays           -- Bucket associativity (default 4).
//   MaxASN              -- Max ASN (must be 256 for Alpha EV6).
//   BucketCount         -- Buckets per shard (default 1024, unused here --
//                          BUCKETS_PER_SHARD is the operative constant).
//   ShardBySize         -- Shard by GH size class (default true).
//   VictimPolicy        -- Replacement policy (default SRRIP).
//   InvalidationPolicy  -- Invalidation strategy (default walk-and-match).
// ============================================================================

template<typename Traits,
	unsigned AssocWays = 4,
	unsigned MaxASN = 256,
	unsigned BucketCount = 1024,
	bool     ShardBySize = true,
	typename VictimPolicy = SRRIPPolicy<typename SPAMBucket<Traits, AssocWays, MaxASN>::Entry, AssocWays>,
	typename InvalidationPolicy = DefaultInvalidationStrategy<typename SPAMBucket<Traits, AssocWays, MaxASN>::Entry>
>
class SPAMShardManager
{
public:
	using Tag    = typename Traits::Tag;
	using Entry  = typename Traits::Entry;
	using Bucket = SPAMBucket<Traits>;

	static constexpr int MAX_CPUS          = 4;     // emulated CPU count
	static constexpr int REALMS            = 2;     // D=0, I=1
	static constexpr int SIZE_CLASSES      = 4;     // GH 0..3
	static constexpr int BUCKETS_PER_SHARD = 128;   // buckets per shard

private:
	// 4-D shard array: [cpu][realm][sizeClass][bucket]
	Bucket m_shards[MAX_CPUS][REALMS][SIZE_CLASSES][BUCKETS_PER_SHARD];

	// Per-CPU epoch tables (zero cross-CPU contention on the hot path).
	PerCPUEpochTable m_asnEpochs[MAX_CPUS];

	// --------------------------------------------------------------------
	// GH Coverage Bitmap -- skip empty size class + global combinations
	// --------------------------------------------------------------------
	//
	// Per-CPU, per-realm bitmask tracking which (GH, global) pairs have
	// had at least one insert since the last TBIA.
	//
	// Encoding (fits in quint8, all 8 bits used):
	//   Low nibble  [3:0] -- non-global (ASM=0) entries per GH
	//   High nibble [7:4] -- global     (ASM=1) entries per GH
	//
	//   Bit 0 = GH=0 non-global    Bit 4 = GH=0 global
	//   Bit 1 = GH=1 non-global    Bit 5 = GH=1 global
	//   Bit 2 = GH=2 non-global    Bit 6 = GH=2 global
	//   Bit 3 = GH=3 non-global    Bit 7 = GH=3 global
	//
	// This directly addresses the VA Format Contract recommendation:
	//   "Do NOT scan VA options on every lookup."
	// The base page size (8KB, pageShift=13) is fixed at EV6
	// implementation time, but GH superpages (64KB, 512KB, 4MB) are
	// per-PTE attributes.  The bitmap eliminates probes for (GH, global)
	// pairs that have never been populated.
	//
	// Probe reduction examples:
	//   Pure userspace     -- 0b00000001 -- 1 probe (was 8)
	//   Kernel + userspace -- 0b00010001 -- 2 probes (was 8)
	//   Kernel w/largepage -- 0b01010001 -- 3 probes (was 8)
	//   All 8 pairs filled -- 0b11111111 -- 8 probes + 1 byte overhead
	//
	// Contract:
	//   - CONSERVATIVE (one-directional): bits set on insert, only
	//     cleared on TBIA.  A set bit may outlive the last entry at
	//     that (GH, global) pair.  Cost: one extra empty-bucket probe
	//     (killed by m_occ bitmap in one relaxed load).
	//   - FALSE NEGATIVES ARE IMPOSSIBLE.
	//   - NOT ATOMIC: single-writer (owning CPU insert + TBIA only).
	//
	// Memory: 4 CPUs x 2 realms x 1 byte = 8 bytes total.
	//
	quint8 m_ghCoverage[MAX_CPUS][REALMS]{};

public:
	// ====================================================================
	// Construction
	// ====================================================================

	// Construct the shard manager and attach CPU-local epoch tables to
	// every bucket.  After construction each bucket knows its CPU, realm,
	// and has a pointer to the correct PerCPUEpochTable for lazy
	// invalidation checks.
	//
	// @param cpuCount  Number of CPUs to initialise (unused; all MAX_CPUS
	//                  are always initialised for simplicity).
	SPAMShardManager(int cpuCount = MAX_CPUS) noexcept
	{
		(void)cpuCount;
		for (int alphaCPU = 0; alphaCPU < MAX_CPUS; ++alphaCPU) {
			for (int r = 0; r < REALMS; ++r) {
				for (int sc = 0; sc < SIZE_CLASSES; ++sc) {
					for (int b = 0; b < BUCKETS_PER_SHARD; ++b) {
						m_shards[alphaCPU][r][sc][b].attachEpochTable(
							&m_asnEpochs[alphaCPU],
							(r == 0) ? Realm::D : Realm::I);
					}
				}
			}
		}
	}

	// ====================================================================
	// Epoch table access
	// ====================================================================

	// Return a reference to the per-CPU epoch table.
	// Used by external code that needs to read/bump epochs directly.
	PerCPUEpochTable& getEpochTable(CPUIdType cpuId) noexcept
	{
		Q_ASSERT(cpuId < MAX_CPUS);
		return m_asnEpochs[cpuId];
	}

	// ====================================================================
	// Context-switch / non-ASM invalidation
	// ====================================================================

	// Bump the global epoch for one CPU.  O(1).
	//
	// After this call every non-global (ASM=0) entry on this CPU whose
	// globalGenAtFill does not match the new epoch is lazily dead.
	//
	// [E2] Now delegates to SPAMEpoch::bumpGlobal() so the ordering
	//      contract is defined in one place.
	AXP_HOT inline void invalidateNonASM(CPUIdType cpuId) noexcept
	{
		SPAMEpoch::bumpGlobal(m_asnEpochs[cpuId]);
	}

	// ====================================================================
	// PTE field extraction and validation
	// ====================================================================

	// Rule 2.1, 3.1 -- Extract GH (granularity hint) from PTE.
	// The PTE is the source of truth for page size.
	//
	// @param pte  Raw PTE.
	// @return     GH value [0..3].
	static inline SC_Type extractGH(const AlphaPTE& pte) noexcept
	{
		quint64 raw = pte.raw;
		SC_Type gh = static_cast<SC_Type>((raw >> 5) & 0x3);
		Q_ASSERT(gh <= 3);
		return gh;
	}

	// Rule 4.4 -- Validate GH, degrading to 8KB if alignment fails.
	//
	// Checks both VA alignment (Rule 2.2) and PFN alignment (Rule 2.2).
	// If either check fails the GH is reduced to 0 (8KB).  The validated
	// GH is NEVER greater than the PTE's original GH.
	//
	// @param pte  Raw PTE.
	// @param va   Virtual address being mapped.
	// @return     Validated GH [0..3], possibly degraded.
	static inline SC_Type validateAndGetPageSize(const AlphaPTE& pte,
	                                             quint64 va) noexcept
	{
		SC_Type gh = extractGH(pte);
		Q_ASSERT(gh <= 3);

		if (gh == 0) {
			return 0;                       // 8KB always valid
		}

		const quint64 shift = PageSizeHelpers::pageShift(gh);
		const quint64 alignMask = (1ULL << shift) - 1;

		// Rule 2.2 -- VA alignment check.
		if ((va & alignMask) != 0) {
			return 0;                       // DEGRADE to 8KB
		}

		// Rule 2.2 -- PFN alignment check.
		PFNType pfn = pte.pfn();
		quint64 pfnAlignMask = (1ULL << (shift - 13)) - 1;
		if ((pfn & pfnAlignMask) != 0) {
			return 0;                       // DEGRADE to 8KB
		}

		return gh;                          // validated
	}

	// ====================================================================
	// TLB Insert -- Rule 4.1, 4.2, 4.5
	// ====================================================================

	// Fill a TLB entry from a PTE.
	//
	// Derives the size class from the PTE (Rule 4.1), validates alignment
	// (Rule 4.4, may degrade), builds the tag with the validated GH
	// (Rule 4.5), and inserts into the appropriate bucket.
	//
	// If the target bucket is full, performs a lazy reclamation sweep
	// across all ASNs and retries once.
	//
	// [B2] Now stamps entry.globalGenAtFill from the current globalEpoch.
	//      Without this, all non-global entries become lazily dead after
	//      the first context switch.
	//
	// @param cpuId  Target CPU.
	// @param realm  Realm::I or Realm::D.
	// @param va     Virtual address.
	// @param asn    Address Space Number.
	// @param pte    Source PTE (source of truth for all fields).
	// @return       true if inserted, false if bucket full after sweep.
	bool tlbInsert(CPUIdType cpuId,
		Realm realm,
		quint64 va,
		ASNType asn,
		const AlphaPTE& pte) noexcept
	{
		if (cpuId >= MAX_CPUS) return false;

		// Rule 4.1 -- Derive sizeClass from PTE.
		SC_Type ghFromPTE = extractGH(pte);

		// Rule 4.4 -- Validate (may reduce, never increase).
		SC_Type validatedGH = validateAndGetPageSize(pte, va);

		// Rule 4.5 -- Tag uses validated GH verbatim.
		const auto tag = Traits::makeTag(va, realm, validatedGH, pte.isGlobal());

		// Extract PTE fields.
		PFNType pfn = Traits::pfn(pte);
		AlphaN_S::PermMask perm = Traits::permMask(pte);
		bool global = pte.isGlobal();

		// Build entry.
		Entry entry;
		entry.tag       = tag;
		entry.pfn       = pfn;
		entry.permMask  = perm;
		entry.asn       = asn;

		// Stamp per-ASN generation based on realm.
		if (realm == Realm::I) {
			entry.asnGenAtFill =
				m_asnEpochs[cpuId].itbEpoch[asn].load(std::memory_order_relaxed);
		}
		else {
			entry.asnGenAtFill =
				m_asnEpochs[cpuId].dtbEpoch[asn].load(std::memory_order_relaxed);
		}

		// [B2] Stamp global generation -- context-switch guard.
		// Without this the field stays 0 and every non-global entry
		// becomes lazily dead after the first invalidateNonASM().
		entry.globalGenAtFill =
			m_asnEpochs[cpuId].globalEpoch.load(std::memory_order_relaxed);

		entry.sizeClass        = validatedGH;
		entry.pteRaw           = pte;
		entry.flags.global     = global;
		entry.flags.valid      = true;
		entry.flags.transitioning = false;
		entry.syncDerivedFromSizeClass();

		// Rule enforcement assertions.
		Q_ASSERT(entry.pfn == Traits::pfn(pte));
		Q_ASSERT(entry.sizeClass == validatedGH);
		Q_ASSERT(validatedGH <= ghFromPTE);
		Q_ASSERT(entry.tag.sizeClass == entry.sizeClass);
		Q_ASSERT(entry.flags.valid == true);

		quint64 expectedVPN = va >> PageSizeHelpers::pageShift(validatedGH);
		Q_ASSERT(entry.tag.vpn == expectedVPN);

		// Route to bucket.
		int realmIdx  = (realm == Realm::D) ? 0 : 1;
		int bucketIdx = static_cast<int>(tag.vpn & (BUCKETS_PER_SHARD - 1));
		auto& buck    = m_shards[cpuId][realmIdx][validatedGH][bucketIdx];

		bool inserted = buck.insert(entry);

		// Lazy reclamation on insert failure: sweep all ASNs and retry.
		if (!inserted) {
			for (ASNType asn_sweep = 0; asn_sweep < 256; ++asn_sweep) {
				quint32 curGen =
					m_asnEpochs[cpuId].itbEpoch[asn_sweep].load(
						std::memory_order_relaxed);
				buck.sweepDeadForASN(asn_sweep, curGen);
			}
			inserted = buck.insert(entry);
		}

		// Mark this (GH, global) pair as populated so that future
		// lookups probe this combination.  Encoding:
		//   non-global: bit = (1 << gh)       -> low nibble
		//   global:     bit = (1 << (gh + 4)) -> high nibble
		if (inserted) {
			const unsigned shift = global ? (validatedGH + 4) : validatedGH;
			m_ghCoverage[cpuId][realmIdx] |= static_cast<quint8>(1u << shift);
		}

		return inserted;
	}

	// ====================================================================
	// TLB Lookup -- Rule 5.1 (probe all GH when unknown)
	// ====================================================================

	// Probe the TLB for a VA, trying all page sizes largest-first.
	//
	// For each GH value [3..0], tries both global and non-global tags.
	// This is the general lookup path used when the page size is not
	// known in advance (e.g. from a hardware walker).
	//
	// @param cpuId        Target CPU.
	// @param realm        Realm::I or Realm::D.
	// @param va           Virtual address to look up.
	// @param currentAsn   Current ASN for non-global matching.
	// @param[out] outPfn       Physical frame number on hit.
	// @param[out] outPermMask  Permission mask on hit.
	// @param[out] outSizeClass GH of the matching entry on hit.
	// @param[out] outPTE       Optional: pointer to cached PTE (COW support).
	// @return true on hit, false on miss.
	bool tlbLookup(CPUIdType cpuId,
		Realm realm,
		quint64 va,
		ASNType currentAsn,
		/*out*/ PFNType& outPfn,
		/*out*/ AlphaN_S::PermMask& outPermMask,
		/*out*/ SC_Type& outSizeClass,
		/*out*/ const AlphaPTE** outPTE = nullptr) noexcept
	{
		if (cpuId >= MAX_CPUS) return false;

		int realmIdx = (realm == Realm::D) ? 0 : 1;

		// GH coverage bitmap: skip (GH, global) pairs that have never
		// been populated.  Low nibble = non-global, high nibble = global.
		// Typical userspace: coverage = 0b00000001 -> 1 probe (was 8).
		const quint8 coverage = m_ghCoverage[cpuId][realmIdx];

		// Rule 5.1.1 -- Probe populated GH values, largest first.
		for (int gh = 3; gh >= 0; --gh)
		{
			// Quick reject: skip GH entirely if neither global type exists.
			if (!(coverage & ((1u << gh) | (1u << (gh + 4))))) continue;

			SC_Type ghVal = static_cast<SC_Type>(gh);

			for (bool global : { true, false })
			{
				// Per-pair filter: skip if this specific (GH, global) is empty.
				const unsigned covBit = global ? (gh + 4) : gh;
				if (((coverage >> covBit) & 1u) == 0u) continue;
				const auto tag = Traits::makeTag(va, realm, ghVal, global);

				int bucketIdx = static_cast<int>(
					tag.vpn & (BUCKETS_PER_SHARD - 1));
				auto& buck = m_shards[cpuId][realmIdx][gh][bucketIdx];

				if (auto* e = buck.find(tag, currentAsn)) {
					Q_ASSERT(e->flags.valid);
					Q_ASSERT(e->sizeClass == ghVal);
					Q_ASSERT(e->tag.sizeClass == ghVal);
					Q_ASSERT(e->pfn != 0);

					outPfn       = e->pfn;
					outPermMask  = e->permMask;
					outSizeClass = e->sizeClass;
					if (outPTE) {
						*outPTE = &e->pteRaw;
					}
					return true;
				}
			}
		}

		return false;       // miss
	}

	// ====================================================================
	// TLB Lookup with known GH -- Rule 5.2
	// ====================================================================

	// Probe the TLB for a VA when the page size is already known
	// (e.g. from ITB_TAG / DTB_TAG registers).
	//
	// Probes both global and non-global entries for the given GH.
	//
	// @param cpuId        Target CPU.
	// @param realm        Realm::I or Realm::D.
	// @param va           Virtual address.
	// @param knownGH      Known GH from tag register.
	// @param currentAsn   Current ASN.
	// @param[out] outPfn       PFN on hit.
	// @param[out] outPermMask  Permission mask on hit.
	// @param[out] outPTE       Optional cached PTE pointer.
	// @return true on hit, false on miss.
	bool tlbLookupWithKnownGH(CPUIdType cpuId,
		Realm realm,
		quint64 va,
		SC_Type knownGH,
		ASNType currentAsn,
		/*out*/ PFNType& outPfn,
		/*out*/ AlphaN_S::PermMask& outPermMask,
		/*out*/ const AlphaPTE** outPTE = nullptr) noexcept
	{
		if (cpuId >= MAX_CPUS || knownGH >= SIZE_CLASSES) return false;

		int realmIdx = (realm == Realm::D) ? 0 : 1;

		for (bool global : { true, false })
		{
			const auto tag = Traits::makeTag(va, realm, knownGH, global);
			int bucketIdx = static_cast<int>(
				tag.vpn & (BUCKETS_PER_SHARD - 1));
			auto& buck = m_shards[cpuId][realmIdx][knownGH][bucketIdx];

			if (auto* e = buck.find(tag, currentAsn)) {
				outPfn      = e->pfn;
				outPermMask = e->permMask;
				if (outPTE) {
					*outPTE = &e->pteRaw;
				}
				return true;
			}
		}

		return false;       // miss
	}

	// ====================================================================
	// TBCHK -- Translation Buffer Check
	// ====================================================================

	// High-performance boolean existence check for one realm.
	//
	// Uses the GH coverage bitmap to skip unpopulated (GH, global)
	// pairs, then probes remaining ones (largest GH first) via the
	// bucket's lightweight probe() method.
	//
	// Cost breakdown:
	//   - 1 byte load (coverage bitmap) for level-0 pair filter.
	//   - Per populated pair: 1 probe() call.
	//   - Per probe(): 1 relaxed load for bitmap fast-reject if empty,
	//     else seqlock + epoch checks returning bool.
	//
	// Typical userspace (GH=0 non-global only):
	//   1 byte load + 1 probe() call.
	// Typical kernel + userspace (GH=0 global + GH=0 non-global):
	//   1 byte load + 2 probe() calls.
	// Worst case (all 8 pairs populated):
	//   1 byte load + 8 probe() calls + 8 bit checks.
	//
	// @param cpuId  Target CPU.
	// @param realm  Realm::I or Realm::D.
	// @param va     Virtual address to check.
	// @param asn    Current ASN.
	// @return       true if a live entry exists for this VA.
	AXP_HOT bool hasValidEntry(CPUIdType cpuId, Realm realm,
		quint64 va, quint8 asn) const noexcept
	{
		if (cpuId >= MAX_CPUS) return false;

		int realmIdx = (realm == Realm::D) ? 0 : 1;

		// GH coverage bitmap: skip (GH, global) pairs with no entries.
		const quint8 coverage = m_ghCoverage[cpuId][realmIdx];

		for (int gh = 3; gh >= 0; --gh)
		{
			// Quick reject: skip GH entirely if neither global type exists.
			if (!(coverage & ((1u << gh) | (1u << (gh + 4))))) continue;

			SC_Type ghVal = static_cast<SC_Type>(gh);

			for (bool global : { true, false })
			{
				// Per-pair filter.
				const unsigned covBit = global ? (gh + 4) : gh;
				if (((coverage >> covBit) & 1u) == 0u) continue;
				const auto tag = Traits::makeTag(va, realm, ghVal, global);

				int bucketIdx = static_cast<int>(
					tag.vpn & (BUCKETS_PER_SHARD - 1));
				const auto& buck = m_shards[cpuId][realmIdx][gh][bucketIdx];

				if (buck.probe(tag, asn))
					return true;
			}
		}

		return false;
	}

	// Probe both ITB and DTB for a VA.
	// Returns a bitmask: bit 0 = DTB hit, bit 1 = ITB hit.
	AXP_HOT AXP_ALWAYS_INLINE quint64 tbchkProbe(
		CPUIdType cpuId, quint64 va, quint8 asn) const noexcept
	{
		quint64 result = 0;
		if (hasValidEntry(cpuId, Realm::D, va, asn)) result |= 1;
		if (hasValidEntry(cpuId, Realm::I, va, asn)) result |= 2;
		return result;
	}

	// ====================================================================
	// Single-entry invalidation instructions
	// ====================================================================

	// TBIS -- Invalidate Single (both ITB and DTB, all GH, both global).
	void tbisInvalidate(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;
		invalidateTLBEntry(cpuId, Realm::I, va, asn);
		invalidateDTBBothBanks(cpuId, va, asn);
	}

	// TBISD -- Invalidate Single Data-stream only (DTB both banks).
	void tbisdInvalidate(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;
		invalidateDTBBothBanks(cpuId, va, asn);
	}

	// TBISI -- Invalidate Single Instruction-stream only (ITB).
	void tbisiInvalidate(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;
		invalidateTLBEntry(cpuId, Realm::I, va, asn);
	}

	// ====================================================================
	// Bulk invalidation
	// ====================================================================

	// TBIA -- Invalidate all TLB entries for one CPU.
	//
	// Walks every bucket and invalidates every slot.  This is the
	// brute-force path; for most cases prefer invalidateNonASM() or
	// invalidateTLBsByASN().
	//
	// [B4] Uses Bucket::kWays (renamed from Bucket::AssocWays).
	void invalidateAllTLBs(CPUIdType cpuId) noexcept
	{
		if (cpuId >= MAX_CPUS) return;

		for (int r = 0; r < REALMS; ++r) {
			for (int sc = 0; sc < SIZE_CLASSES; ++sc) {
				for (int b = 0; b < BUCKETS_PER_SHARD; ++b) {
					auto& buck = m_shards[cpuId][r][sc][b];
					for (unsigned i = 0; i < Bucket::kWays; ++i) {
						buck.invalidate(i);
					}
				}
			}
			// Reset coverage bitmap -- no entries remain for any GH.
			m_ghCoverage[cpuId][r] = 0;
		}
	}

	// TBIAP -- Invalidate all entries for one ASN on one CPU.
	//
	// Bumps the per-ASN epoch (O(1) lazy kill), then performs an eager
	// sweep to reclaim occupied slots.
	void invalidateTLBsByASN(CPUIdType cpuId, ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;

		// Bump CPU-local epoch for this ASN.
		SPAMEpoch::bumpBoth(m_asnEpochs[cpuId], asn);

		// Get the new epoch for the sweep.
		quint32 newGen =
			m_asnEpochs[cpuId].itbEpoch[asn].load(std::memory_order_relaxed);

		// Eager sweep: reclaim slots on this CPU only.
		for (int r = 0; r < REALMS; ++r) {
			for (int sc = 0; sc < SIZE_CLASSES; ++sc) {
				for (int b = 0; b < BUCKETS_PER_SHARD; ++b) {
					m_shards[cpuId][r][sc][b].sweepDeadForASN(asn, newGen);
				}
			}
		}
	}

	// Cross-CPU TBIAP -- Invalidate one ASN on all CPUs.
	// This is the IPI (inter-processor interrupt) slow path.
	void invalidateTLBsByASN_AllCPUs(ASNType asn) noexcept
	{
		for (int cpu = 0; cpu < MAX_CPUS; ++cpu) {
			invalidateTLBsByASN(cpu, asn);
		}
	}

	// ====================================================================
	// Convenience wrappers
	// ====================================================================

	// Invalidate one DTB entry (both banks, all GH).
	void invalidateDTBEntry(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
	{
		invalidateDTBBothBanks(cpuId, va, asn);
	}

	// Invalidate one ITB entry (all GH).
	void invalidateITBEntry(CPUIdType cpuId, quint64 va, ASNType asn) noexcept
	{
		invalidateTLBEntry(cpuId, Realm::I, va, asn);
	}

	// ====================================================================
	// Internal helpers
	// ====================================================================

	// Invalidate a single VA in one realm (all GH, both global/non-global).
	//
	// Iterates over all 4 size classes and both global flags, building
	// the tag for each combination and calling invalidateMatching() on
	// the target bucket.
	void invalidateTLBEntry(CPUIdType cpuId,
		Realm realm,
		quint64 va,
		ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;

		int realmIdx = (realm == Realm::D) ? 0 : 1;

		for (SC_Type gh = 0; gh < SIZE_CLASSES; ++gh) {
			for (bool global : { true, false }) {
				const auto tag = Traits::makeTag(va, realm, gh, global);

				int bucketIdx = static_cast<int>(
					tag.vpn & (BUCKETS_PER_SHARD - 1));
				auto& buck = m_shards[cpuId][realmIdx][gh][bucketIdx];

				buck.invalidateMatching(tag, asn);
			}
		}
	}

	// Invalidate a single VA in DTB, both banks (all GH, both global).
	//
	// The EV6 DTB uses two banks selected by a bit in the VA.  To ensure
	// correctness we invalidate both possible bank-selected addresses
	// for each GH/global combination.
	void invalidateDTBBothBanks(CPUIdType cpuId, quint64 va,
	                            ASNType asn) noexcept
	{
		if (cpuId >= MAX_CPUS) return;

		for (SC_Type gh = 0; gh < SIZE_CLASSES; ++gh) {
			const quint64 shift   = PageSizeHelpers::pageShift(gh);
			const quint64 bankBit = (1ULL << (shift - 1));

			for (bool global : { true, false }) {
				// Bank 0 -- clear bank bit.
				quint64 va0 = va & ~bankBit;
				const auto tag0 = Traits::makeTag(va0, Realm::D, gh, global);
				int bucket0 = static_cast<int>(
					tag0.vpn & (BUCKETS_PER_SHARD - 1));
				m_shards[cpuId][0][gh][bucket0].invalidateMatching(tag0, asn);

				// Bank 1 -- set bank bit.
				quint64 va1 = va | bankBit;
				const auto tag1 = Traits::makeTag(va1, Realm::D, gh, global);
				int bucket1 = static_cast<int>(
					tag1.vpn & (BUCKETS_PER_SHARD - 1));
				m_shards[cpuId][0][gh][bucket1].invalidateMatching(tag1, asn);
			}
		}
	}
};

#endif // ALPHA_SPAM_MANAGER_H
