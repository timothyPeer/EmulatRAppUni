// ============================================================================
// StagedPTECache.h - Check if staged PTE matches given VA and ASN.
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

#ifndef STAGEDPTECACHE_H
#define STAGEDPTECACHE_H

#include "../pteLib/AlphaPTE_Core.h"
#include "../pteLib/Ev6PTETraits.h"
#include "../pteLib/Ev6SiliconTLB_Singleton.h"
#include <QtGlobal>
#include "../pteLib/alpha_pte_core.h"


using namespace AlphaN_S;

// ============================================================================
// Staged PTE Cache
// ============================================================================
// Temporary staging area for PTEs during TLB miss handling.
// Allows fetch -> validate -> commit workflow.
//
// Workflow:
//   1. TLB miss occurs
//   2. Fetch PTE from page table -> m_stagedDTBPTE
//   3. Validate PTE (check permissions, format)
//   4. Build TLB tag -> m_stagedDTBTAG
//   5. commitStagedPTEs() -> Insert into silicon TLB
//   6. reset() -> Clear staging area
// ============================================================================

enum class stagePTEType : quint8 {
	DTBPTE,      // Data TLB PTE
	DTBPTE0,     // DTB bank 0 (if modeling dual-bank)
	DTBPTE1,     // DTB bank 1 (if modeling dual-bank)
	ITBPTE,      // Instruction TLB PTE
	DTBTAG       // DTB tag (VA + ASN)
};

struct stagePTECache {
	// Staged PTEs (temporary holding)
	AlphaPTE m_stagedDTBPTE = AlphaPTE{};
	AlphaPTE m_stagedDTBPTE0 = AlphaPTE{};
	AlphaPTE m_stagedDTBPTE1 = AlphaPTE{};
	AlphaPTE m_stagedITBPTE = AlphaPTE{};

	// Staged tag (VA + ASN for TLB insertion)
	Ev6TLBTag m_stagedDTBTAG = Ev6TLBTag{};

	// Optional: staged metadata
	VAType  m_stagedVA = 0;
	ASNType m_stagedASN = 0;
	SC_Type m_sizeClass = 0;
	PFNType m_pfn = 0;
	quint8 m_perm = 0;

	stagePTECache() = default;
	~stagePTECache() = default;
	// ========================================================================
	// Reset - Clear all staged entries
	// ========================================================================
	AXP_FLATTEN 	void clear() noexcept {
		m_stagedDTBPTE = AlphaPTE{};
		m_stagedDTBPTE0 = AlphaPTE{};
		m_stagedDTBPTE1 = AlphaPTE{};
		m_stagedITBPTE = AlphaPTE{};
		m_stagedDTBTAG = Ev6TLBTag{};
		m_stagedVA = 0;
		m_stagedASN = 0;
	}

	/**
	 * @brief Check if staged PTE matches given VA and ASN.
	 *
	 * Used by pipeline to determine if a retry can use the staged PTE
	 * without going to silicon TLB yet.
	 *
	 * @param va Virtual address to check
	 * @param asn Address Space Number to check
	 * @param realm Realm (D-stream or I-stream)
	 * @return true if staged PTE matches this address
	 */
	AXP_FLATTEN
		bool matches(VAType va, ASNType asn, Realm realm = Realm::D) const noexcept {
		// Check if we have a staged entry for this realm
		const AlphaPTE& pte = (realm == Realm::D) ? m_stagedDTBPTE : m_stagedITBPTE;

		if (!pte.bitV()) {
			return false;  // No valid staged entry
		}

		// Check if VA matches (page-aligned comparison)
		constexpr quint64 PAGE_MASK = ~0x1FFFULL;  // 8KB page mask
		if ((m_stagedVA & PAGE_MASK) != (va & PAGE_MASK)) {
			return false;  // Different page
		}

		// Check ASN (unless global mapping)
		if (!pte.bitASM() && m_stagedASN != asn) {
			return false;  // Different ASN (and not global)
		}

		return true;  // Match!
	}

	// ========================================================================
	// NEW: matches() Overload - Just VA (Assumes Current ASN)
	// ========================================================================
	/**
	 * @brief Check if staged PTE matches given VA.
	 *
	 * Simplified version that assumes caller already checked ASN.
	 *
	 * @param va Virtual address to check
	 * @param realm Realm (D-stream or I-stream)
	 * @return true if staged PTE matches this address
	 */
	AXP_FLATTEN
		bool matches(VAType va, Realm realm = Realm::D) const noexcept {
		const AlphaPTE& pte = (realm == Realm::D) ? m_stagedDTBPTE : m_stagedITBPTE;

		if (!pte.bitV()) {
			return false;
		}

		constexpr quint64 PAGE_MASK = ~0x1FFFULL;
		return (m_stagedVA & PAGE_MASK) == (va & PAGE_MASK);
	}

	// ========================================================================
	// Commit Staged PTEs to Silicon TLB
	// ========================================================================
	// Atomically insert all valid staged PTEs into the hardware TLB.
	// Returns true if successful, false if TLB insertion failed.
	//
	// This is the single commit point - ensures we don't partially update
	// the TLB if something goes wrong during PTE fetch/validation.
	// ========================================================================
	AXP_HOT AXP_ALWAYS_INLINE bool commitStagedPTEs(CPUIdType cpuId) noexcept {

		auto& silicon = globalEv6Silicon();
		bool success = true;

		// Commit DTB PTE (data translation)
		if (m_stagedDTBPTE.valid()) {
			success &= silicon.spam().tlbInsert(
				cpuId,
				Realm::D,              // Data realm
				m_stagedVA,
				m_stagedASN,
				m_stagedDTBPTE
			);
		}

		// Commit ITB PTE (instruction translation)
		if (m_stagedITBPTE.valid()) {
			success &= silicon.spam().tlbInsert(
				cpuId,
				Realm::I,              // Instruction realm
				m_stagedVA,
				m_stagedASN,
				m_stagedITBPTE
			);
		}

		// Clear staged entries after successful commit
		if (success) {
			clear();
		}

		return success;
	}

	/**
	 * @brief Get staged PTE for given realm.
	 *
	 * @param realm Realm (D-stream or I-stream)
	 * @return Reference to staged PTE
	 */
	AXP_FLATTEN
		const AlphaPTE& getStagedPTE(Realm realm = Realm::D) const noexcept {
		return (realm == Realm::D) ? m_stagedDTBPTE : m_stagedITBPTE;
	}

	AXP_FLATTEN void setPFN(PFNType pfn, Realm realm = Realm::D) noexcept { 
	
		switch (realm) {
		case Realm::D:
			m_stagedDTBPTE.setPFN(pfn);
			break;
		case Realm::I:
			m_stagedITBPTE.setPFN(pfn);
			break;
		default:
			break;

		}
	}
	
	AXP_FLATTEN void setPermMask(PermMask pMask, Realm realm = Realm::D) noexcept {

		switch (realm) {
		case Realm::D:
			m_stagedDTBPTE.setPermMask(pMask);
			break;
		case Realm::I:
			m_stagedITBPTE.setPermMask(pMask);
			break;
		default:
			break;

		}
	}
	AXP_FLATTEN void setSizeClass(SC_Type sc, Realm realm = Realm::D) noexcept {

		switch (realm) {
		case Realm::D:
			m_stagedDTBPTE.setGH(sc);
			break;
		case Realm::I:
			m_stagedITBPTE.setGH(sc);
			break;
		default:
			break;

		}
	}
	/**
	 * @brief Translate VA using staged PTE.
	 *
	 * Used by pipeline to avoid silicon TLB lookup on retry.
	 *
	 * @param va Virtual address to translate
	 * @param[out] pa Output physical address
	 * @param realm Realm (D-stream or I-stream)
	 * @return true if translation succeeded
	 */
	AXP_FLATTEN
		bool translateWithStagedPTE(VAType va, quint64& pa, Realm realm = Realm::D) const noexcept {
		const AlphaPTE& pte = getStagedPTE(realm);

		if (!pte.bitV()) {
			return false;  // No valid staged entry
		}

		if (!matches(va, realm)) {
			return false;  // VA doesn't match staged entry
		}

		// Translate: PA = (PFN << PAGE_SHIFT) | page_offset
		constexpr quint64 PAGE_SHIFT_inl = 13;  // 8KB pages
		constexpr quint64 PAGE_OFFSET_MASK_inl = 0x1FFF;

		pa = (static_cast<quint64>(pte.pfn()) << PAGE_SHIFT_inl) | (va & PAGE_OFFSET_MASK_inl);

		return true;
	}

	// ========================================================================
	// Stage DTB Entry
	// ========================================================================
	// Prepare a DTB entry for commit.
	// Called during DTB miss handling after PTE fetch.
	// ========================================================================
	AXP_FLATTEN
		void stageDTBEntry(VAType va, ASNType asn, const AlphaPTE& pte) noexcept {
		m_stagedVA = va;
		m_stagedASN = asn;
		m_stagedDTBPTE = pte;

		// Build TLB tag
// 		m_stagedDTBTAG.va = va;
// 		m_stagedDTBTAG.asn = asn;
	}

	// ========================================================================
	// Stage ITB Entry
	// ========================================================================
	// Prepare an ITB entry for commit.
	// Called during ITB miss handling after PTE fetch.
	// ========================================================================
	AXP_FLATTEN
		void stageITBEntry(VAType va, ASNType asn, const AlphaPTE& pte) noexcept {
		m_stagedVA = va;
		m_stagedASN = asn;
		m_stagedITBPTE = pte;
	}

	// ========================================================================
	// Query Staged State
	// ========================================================================
	AXP_FLATTEN
		bool hasStagedDTB() const noexcept {
		return m_stagedDTBPTE.valid();
	}

	AXP_FLATTEN
		bool hasStagedITB() const noexcept {
		return m_stagedITBPTE.valid();
	}
};

#endif // STAGEDPTECACHE_H
