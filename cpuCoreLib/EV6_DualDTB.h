// ============================================================================
// EV6_DualDTB.h - EV6 Dual DTB Array Management
// ============================================================================
// Part of Phase 5: EV6 Support - Advanced Implementation
// 
// This file implements dual data TLB (DTB) array management for EV6.
// Unlike EV4/EV5 which have a single DTB, EV6 has TWO DTB arrays:
//   - DTB0: Primary DTB array (128 entries)
//   - DTB1: Secondary DTB array (128 entries)
// 
// Design Philosophy:
//   - Separate DTB0 and DTB1 management
//   - Split TLB invalidation operations (can target DTB0 or DTB1)
//   - Dual lookup for maximum TLB hit rate
//   - Separate DTB0/DTB1 miss handlers
// 
// Key Operations:
//   - DTB0_FLUSH: Flush DTB0 only
//   - DTB1_FLUSH: Flush DTB1 only
//   - DTB0_ASN_FLUSH: Flush DTB0 entries for ASN
//   - DTB1_ASN_FLUSH: Flush DTB1 entries for ASN
//   - Dual lookup: Check both DTB0 and DTB1
// // EV6 has TWO DTB banks:
// desc_DTB_IA0(0x23)  // Bank 0 only (MBOX0, 0L pipe)
// desc_DTB_IA1(0xA3)  // Bank 1 only (MBOX1, 1L pipe)
// desc_DTB_IAP(0x22)  // BOTH banks
// 
// Integration with Phase 4 TLBManager:
//   - Extends TLBManager to support dual arrays
//   - DTB0 = primary array, DTB1 = secondary array
//   - Invalidation operations can target specific array
// ============================================================================
// 
// #pragma  once
// 
// #include <cstdint>
// #include <QtGlobal>
// 
// 
// 
// 			// ============================================================================
// 			// Dual DTB Manager (EV6 Extension)
// 			// ============================================================================
// 			// Extends TLBManager from Phase 4 to support dual DTB arrays.
// 			//
// 			// Design:
// 			//   - DTB0: Primary array (128 entries on EV6)
// 			//   - DTB1: Secondary array (128 entries on EV6)
// 			//   - ITB: Single array (128 entries on EV6)
// 			//   - Separate invalidation operations per array
// 			//   - Dual lookup (check both arrays)
// 			// ============================================================================
// 
// class DualDTBManager : public TLBManager {
// public:
// 	// ========================================================================
// 	// CONSTRUCTION
// 	// ========================================================================
// 	DualDTBManager(qsizetype itbSize = 128, qsizetype dtb0Size = 128, qsizetype dtb1Size = 128)
// 		: TLBManager(itbSize, dtb0Size),  // Base TLBManager manages ITB + DTB0
// 		m_dtb1(dtb1Size)
// 	{
// 		reset();
// 	}
// 
// 	// ========================================================================
// 	// RESET (Override)
// 	// ========================================================================
// 	void reset() override {
// 		TLBManager::reset();  // Reset ITB and DTB0
// 		for (auto& entry : m_dtb1) entry.invalidate();
// 	}
// 
// 	// ========================================================================
// 	// DTB0 OPERATIONS (Use base class DTB as DTB0)
// 	// ========================================================================
// 
// 	// Invalidate all DTB0 entries
// 	void invalidateDTB0All() {
// 		// Base class m_dtb is DTB0
// 		for (auto& entry : m_dtb) {
// 			entry.invalidate();
// 		}
// 		m_stats.invalidateAllCount++;
// 	}
// 
// 	// Invalidate DTB0 entries for ASN
// 	void invalidateDTB0Process(quint8 asn) {
// 		for (auto& entry : m_dtb) {
// 			if (entry.valid && entry.asn == asn && entry.asm_field == 0) {
// 				entry.invalidate();
// 			}
// 		}
// 		m_stats.invalidateProcessCount++;
// 	}
// 
// 	// Invalidate single DTB0 entry by VA
// 	void invalidateDTB0Single(quint64 va, quint8 currentASN) {
// 		quint64 vpn = va >> 13;
// 
// 		for (auto& entry : m_dtb) {
// 			if (entry.valid && entry.vpn == vpn) {
// 				if (entry.asm_field == 1 || entry.asn == currentASN) {
// 					entry.invalidate();
// 					m_stats.invalidateSingleCount++;
// 					return;
// 				}
// 			}
// 		}
// 	}
// 
// 	// ========================================================================
// 	// DTB1 OPERATIONS (New for EV6)
// 	// ========================================================================
// 
// 	// Invalidate all DTB1 entries
// 	void invalidateDTB1All() {
// 		for (auto& entry : m_dtb1) {
// 			entry.invalidate();
// 		}
// 		m_statsDTB1.invalidateAllCount++;
// 	}
// 
// 	// Invalidate DTB1 entries for ASN
// 	void invalidateDTB1Process(quint8 asn) {
// 		for (auto& entry : m_dtb1) {
// 			if (entry.valid && entry.asn == asn && entry.asm_field == 0) {
// 				entry.invalidate();
// 			}
// 		}
// 		m_statsDTB1.invalidateProcessCount++;
// 	}
// 
// 	// Invalidate single DTB1 entry by VA
// 	void invalidateDTB1Single(quint64 va, quint8 currentASN) {
// 		quint64 vpn = va >> 13;
// 
// 		for (auto& entry : m_dtb1) {
// 			if (entry.valid && entry.vpn == vpn) {
// 				if (entry.asm_field == 1 || entry.asn == currentASN) {
// 					entry.invalidate();
// 					m_statsDTB1.invalidateSingleCount++;
// 					return;
// 				}
// 			}
// 		}
// 	}
// 
// 	// ========================================================================
// 	// COMBINED OPERATIONS (Invalidate Both DTB0 and DTB1)
// 	// ========================================================================
// 
// 	// Override base class invalidateAll() to handle both DTBs
// 	void invalidateAll() override {
// 		TLBManager::invalidateAll();  // Invalidate ITB + DTB0
// 		invalidateDTB1All();           // Invalidate DTB1
// 	}
// 
// 	// Override base class invalidateProcess() to handle both DTBs
// 	void invalidateProcess(quint8 asn) override {
// 		TLBManager::invalidateProcess(asn);  // Invalidate ITB + DTB0
// 		invalidateDTB1Process(asn);          // Invalidate DTB1
// 	}
// 
// 	// Override base class invalidateSingle() to handle both DTBs
// 	void invalidateSingle(quint64 va, quint8 currentASN) override {
// 		TLBManager::invalidateSingle(va, currentASN);  // Invalidate ITB + DTB0
// 		invalidateDTB1Single(va, currentASN);          // Invalidate DTB1
// 	}
// 
// 	// ========================================================================
// 	// DUAL DTB LOOKUP (Check Both DTB0 and DTB1)
// 	// ========================================================================
// 
// 	// Lookup DTB (checks both DTB0 and DTB1)
// 	// Returns: DTB0 entry (preferred), or DTB1 entry, or nullptr
// 	TLBEntry* lookupDTBDual(quint64 va, quint8 currentASN) {
// 		// Try DTB0 first (primary array)
// 		TLBEntry* entry0 = lookupDTB(va, currentASN);
// 		if (entry0) {
// 			m_statsDTB0Hits++;
// 			return entry0;
// 		}
// 
// 		// Try DTB1 (secondary array)
// 		for (auto& entry : m_dtb1) {
// 			if (entry.matches(va, currentASN)) {
// 				entry.lastUsed = m_cycleCounter++;
// 				m_statsDTB1Hits++;
// 				return &entry;
// 			}
// 		}
// 
// 		// TLB miss
// 		m_statsDTBMisses++;
// 		return nullptr;
// 	}
// 
// 	// ========================================================================
// 	// DTB1 INSERTION (For TLB Fills)
// 	// ========================================================================
// 
// 	// Insert DTB1 entry (after DTB miss handler)
// 	void insertDTB1(quint64 vpn, quint8 asn, quint64 pfn, quint8 prot,
// 		quint8 asm_field = 0, quint8 gh = 0) {
// 		// Find victim entry (LRU)
// 		TLBEntry* victim = findLRUEntry(m_dtb1);
// 
// 		// Fill entry
// 		victim->valid = true;
// 		victim->vpn = vpn;
// 		victim->asn = asn;
// 		victim->pfn = pfn;
// 		victim->prot = prot;
// 		victim->asm_field = asm_field;
// 		victim->gh = gh;
// 		victim->lastUsed = m_cycleCounter++;
// 
// 		m_statsDTB1Fills++;
// 	}
// 
// 	// ========================================================================
// 	// STATISTICS (Extended for DTB1)
// 	// ========================================================================
// 
// 	struct DualDTBStatistics {
// 		// ITB (inherited from base)
// 		quint64 itbHits;
// 		quint64 itbMisses;
// 
// 		// DTB0 (primary)
// 		quint64 dtb0Hits;
// 		quint64 dtb0Misses;
// 		quint64 dtb0Fills;
// 
// 		// DTB1 (secondary)
// 		quint64 dtb1Hits;
// 		quint64 dtb1Misses;
// 		quint64 dtb1Fills;
// 
// 		// Combined
// 		quint64 dtbHitsCombined;
// 		quint64 dtbMissesCombined;
// 
// 		// Invalidations
// 		quint64 invalidateAllCount;
// 		quint64 invalidateProcessCount;
// 		quint64 invalidateSingleCount;
// 
// 		DualDTBStatistics() {
// 			itbHits = itbMisses = 0;
// 			dtb0Hits = dtb0Misses = dtb0Fills = 0;
// 			dtb1Hits = dtb1Misses = dtb1Fills = 0;
// 			dtbHitsCombined = dtbMissesCombined = 0;
// 			invalidateAllCount = invalidateProcessCount = invalidateSingleCount = 0;
// 		}
// 	};
// 
// 	DualDTBStatistics getDualStatistics() const {
// 		DualDTBStatistics stats;
// 
// 		// ITB stats (from base)
// 		const auto& baseStats = getStatistics();
// 		stats.itbHits = baseStats.itbHits;
// 		stats.itbMisses = baseStats.itbMisses;
// 
// 		// DTB0 stats (from base, but we track separately)
// 		stats.dtb0Hits = m_statsDTB0Hits;
// 		stats.dtb0Misses = baseStats.dtbMisses - m_statsDTB1Hits - m_statsDTBMisses;
// 		stats.dtb0Fills = baseStats.dtbFills;
// 
// 		// DTB1 stats (our own tracking)
// 		stats.dtb1Hits = m_statsDTB1Hits;
// 		stats.dtb1Misses = m_statsDTB1.dtbMisses;
// 		stats.dtb1Fills = m_statsDTB1Fills;
// 
// 		// Combined DTB stats
// 		stats.dtbHitsCombined = stats.dtb0Hits + stats.dtb1Hits;
// 		stats.dtbMissesCombined = m_statsDTBMisses;
// 
// 		// Invalidations
// 		stats.invalidateAllCount = baseStats.invalidateAllCount + m_statsDTB1.invalidateAllCount;
// 		stats.invalidateProcessCount = baseStats.invalidateProcessCount + m_statsDTB1.invalidateProcessCount;
// 		stats.invalidateSingleCount = baseStats.invalidateSingleCount + m_statsDTB1.invalidateSingleCount;
// 
// 		return stats;
// 	}
// 
// 	void resetDualStatistics() {
// 		resetStatistics();  // Base class stats
// 		m_statsDTB1 = TLBManager::Statistics();
// 		m_statsDTB0Hits = m_statsDTB1Hits = m_statsDTBMisses = 0;
// 		m_statsDTB1Fills = 0;
// 	}
// 
// 	// ========================================================================
// 	// ACCESSORS (Extended for DTB1)
// 	// ========================================================================
// 
// 	const std::vector<TLBEntry>& getDTB0() const { return m_dtb; }  // Base class DTB
// 	const std::vector<TLBEntry>& getDTB1() const { return m_dtb1; }
// 
// 	qsizetype getDTB0Size() const { return m_dtb.size(); }
// 	qsizetype getDTB1Size() const { return m_dtb1.size(); }
// 
// 	qsizetype countValidDTB0() const {
// 		return countValidDTB();  // Base class method
// 	}
// 
// 	qsizetype countValidDTB1() const {
// 		qsizetype count = 0;
// 		for (const auto& entry : m_dtb1) {
// 			if (entry.valid) count++;
// 		}
// 		return count;
// 	}
// 
// private:
// 	// ========================================================================
// 	// INTERNAL STATE
// 	// ========================================================================
// 	std::vector<TLBEntry> m_dtb1;       // Secondary DTB array (DTB1)
// 
// 	// DTB1-specific statistics
// 	TLBManager::Statistics m_statsDTB1;
// 
// 	// Combined statistics tracking
// 	quint64 m_statsDTB0Hits = 0;
// 	quint64 m_statsDTB1Hits = 0;
// 	quint64 m_statsDTBMisses = 0;
// 	quint64 m_statsDTB1Fills = 0;
// 
// 	// ========================================================================
// 	// HELPERS
// 	// ========================================================================
// 
// 	// Find LRU entry in DTB1
// 	TLBEntry* findLRUEntry(std::vector<TLBEntry>& tlb) {
// 		TLBEntry* lru = &tlb[0];
// 		quint64 oldestTime = lru->lastUsed;
// 
// 		for (auto& entry : tlb) {
// 			if (!entry.valid) {
// 				return &entry;  // Use invalid entry first
// 			}
// 			if (entry.lastUsed < oldestTime) {
// 				lru = &entry;
// 				oldestTime = entry.lastUsed;
// 			}
// 		}
// 
// 		return lru;
// 	}
// };
// 
// ============================================================================
// EV6 IPR Side Effects (Dual DTB)
// ============================================================================
// These functions are called from EV6 descriptor hooks to perform
// dual DTB invalidation operations.
// ============================================================================
// 
// namespace EV6DualDTB {
// 
// 	// DTB0_FLUSH: Flush DTB0 only
// 	inline void onDTB0_FLUSHWrite(CPUStateIPRInterface* cpu, quint64 oldValue, quint64 newValue) {
// 		(void)oldValue;
// 		(void)newValue;
// 		(void)cpu;
// 
// 		printf("[CPU] DTB0: Flushing all entries\n");
// 	}
// 
// 	// DTB1_FLUSH: Flush DTB1 only
// 	inline void onDTB1_FLUSHWrite(CPUStateIPRInterface* cpu, quint64 oldValue, quint64 newValue) {
// 		(void)oldValue;
// 		(void)newValue;
// 		(void)cpu;
// 
// 		printf("[CPU] DTB1: Flushing all entries\n");
// 	}
// 
// 	// DTB0_ASN_FLUSH: Flush DTB0 entries for ASN
// 	inline void onDTB0_ASN_FLUSHWrite(CPUStateIPRInterface* cpu, quint64 oldValue, quint64 newValue) {
// 		(void)oldValue;
// 
// 		quint8 asn = newValue & 0xFF;
// 		(void)cpu;
// 		(void)asn;
// 
// 		printf("[CPU] DTB0: Flushing entries for ASN=%u\n", asn);
// 	}
// 
// 	// DTB1_ASN_FLUSH: Flush DTB1 entries for ASN
// 	inline void onDTB1_ASN_FLUSHWrite(CPUStateIPRInterface* cpu, quint64 oldValue, quint64 newValue) {
// 		(void)oldValue;
// 
// 		quint8 asn = newValue & 0xFF;
// 		(void)cpu;
// 		(void)asn;
// 
// 		printf("[CPU] DTB1: Flushing entries for ASN=%u\n", asn);
// 	}
// 
