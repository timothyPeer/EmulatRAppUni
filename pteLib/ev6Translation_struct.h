// ============================================================================
// ev6Translation_struct.h - Translate virtual address for data access (load/store)
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

// ReSharper disable All
#ifndef EV6TRANSLATOR_H
#define EV6TRANSLATOR_H


#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/VA_types.h"
#include "memoryLib/GuestMemory.h"
#include "memoryLib/global_GuestMemory.h"


#include "../coreLib/VA_core.h"
#include "../pteLib/AlphaPTE_Core.h"
#include "../pteLib/alpha_pte_core.h"
#include "../coreLib/types_core.h"


#include <QtGlobal>

#include "cpuCoreLib/ReservationManager.h"
#include "exceptionLib/ExceptionFactory.h"
#include "faultLib/FaultDispatcher.h"
#include "faultLib/GlobalFaultDispatcherBank.h"
#include "faultLib/raiseTranslationFault_inl.h"
#include "machineLib/PipeLineSlot_inl.h"
#include "pteLib/global_Ev6TLB_Singleton.h"
#include <QMutexLocker>
#include <QMutex>

#include "Ev6SiliconTypes.h"

struct alignas(64) Ev6Translator
{
    // Injected once at construction - never changes
    CPUIdType				m_cpuId;

    HWPCB* m_hwpcb;         // ptbr, asn, pc
    GuestMemory*			m_guestMemory;   // page walk reads
    Ev6SPAMShardManager*	m_tlb;           // TLB lookup/insert
	FaultDispatcher*		m_fault_dispatcher{ nullptr };
	ReservationManager*		m_reservationManager{ nullptr };
	CPUStateView* m_iprGlobalMaster{ nullptr };

    Ev6Translator(CPUIdType cpuId)
    :   m_cpuId(cpuId)
    ,   m_hwpcb(&globalHWPCBController(cpuId))
    ,   m_tlb(&globalSPAM(cpuId))
    ,	m_guestMemory(&global_GuestMemory())
    ,	m_fault_dispatcher(&globalFaultDispatcher(cpuId))
	,	m_reservationManager(&globalReservationManager())
		, m_iprGlobalMaster(getCPUStateView(cpuId))
    {
		
    }
    ~Ev6Translator() = default;


#pragma region EV6TranslateFastVA

	// ---------------------------------------------------------------------------
	// ev6TranslateFastVA
	//
	// Fast-path VA -> PA translation:
	//
	//  - Uses TLB only (no page walk).
	//  - Returns 'true' on TLB hit with a valid mapping, fills pa_out.
	//  - Returns 'false' on miss or if TLB entry is not usable.
	//  - Does NOT do page walking, permission traps, or fault classification.
	//    That is left to PAL / trap handlers or ev6TranslateFullVA.
	// ---------------------------------------------------------------------------


	AXP_HOT inline TranslationResult ev6TranslateFastVA(
		VAType va,
		AccessKind access,
		Mode_Privilege mode,
		PAType& pa_out,
		AlphaPTE* outPte = nullptr) const noexcept
	{
		DEBUG_LOG(QString("ev6TranslateFastVA VA: 0x%1").arg(va, 16, 16));

		const ASNType asn = static_cast<ASNType>(m_hwpcb->asn);
		const VAType va_ctl = m_iprGlobalMaster->x->va_ctl;//   // Get directly via global accessor

		// Canonical check
		if (!isCanonicalVA(va, va_ctl))
			return TranslationResult::NonCanonical;

		// Kseg fast path - no TLB, no page walk
		TranslationResult ksegResult = tryKsegTranslate(va, va_ctl, mode, pa_out);
		if (ksegResult != TranslationResult::NotKseg)
			return ksegResult;   // Success or AccessViolation

		// Map access type to realm
		Realm realm = (access == AccessKind::EXECUTE) ? Realm::I : Realm::D;

		PFNType pfn = 0;
		SC_Type sizeClass = 0;
		AlphaN_S::PermMask perm = {};

		if (!m_tlb->tlbLookup(m_cpuId, realm, va, asn, pfn, perm, sizeClass))
			return TranslationResult::TlbMiss;

		const quint64 offset = extractOffset(va);
		pa_out = (static_cast<PFNType>(pfn) << PAGE_SHIFT) | offset;

		if (outPte) {
			AlphaPTE tmp;
			tmp.setPFN(pfn);
			tmp.setPermMask(perm);
			tmp.setValid(true);
			*outPte = tmp;
		}
		return TranslationResult::Success;
	}

#pragma endregion EV6TranslateFastVA

#pragma region Full Translation (Page Walk + Checks)

	// ---------------------------------------------------------------------------
	// ev6TranslateFullVA
	//
	// Full VA -> PA translation with EV6 semantics:
	//  1) Canonical VA check
	//  2) Page table walk (3-level)
	//  3) PTE valid bit check
	//  4) Permission check
	//  5) TLB fill
	//  6) Return PA and PTE
	// ---------------------------------------------------------------------------

	AXP_HOT inline TranslationResult ev6TranslateFullVA(
		VAType va,
		AccessKind access,
		Mode_Privilege mode,
		PAType& pa_out,
		AlphaPTE& outPte)  noexcept
	{
		// 1. Canonical check
		const VAType va_ctl = m_iprGlobalMaster->x->va_ctl; // globalIPRHotExt(cpuId).va_ctl;
		if (!isCanonicalVA(va, va_ctl)) {
			return TranslationResult::NonCanonical;
		}

		// 2. Kseg fast path - no TLB, no page walk
		TranslationResult ksegResult = tryKsegTranslate(va, va_ctl, mode, pa_out);
		if (ksegResult != TranslationResult::NotKseg)
			return ksegResult;   // Success or AccessViolation

		// 3. Page walk
		const quint64 ptbr = m_hwpcb->ptbr; //  getPTBR_Active(cpuId);

		auto walkResult = walkPageTable_EV6(va, ptbr, mode, access);

		// 4. Convert walk result to TranslationResult
		if (!walkResult.success) {
			return toTranslationResult(walkResult);
		}

		// 4. Fill TLB
		Realm realm = (access == AccessKind::EXECUTE) ? Realm::I : Realm::D;
		ASNType asn = static_cast<ASNType>(m_hwpcb->asn); //  getASN_Active(cpuId);

		// 5. Build PermMask from PTE permission bits
		AlphaN_S::PermMask perm = walkResult.pte.protection8();

		// Extract permission bits from PTE and pack into byte
		// Assuming PermMask encoding (check your alpha_pte_core.h for exact layout):
		//   bit 0: kernel_read  (KRE)
		//   bit 1: kernel_write (KWE)
		//   bit 2: user_read    (URE)
		//   bit 3: user_write   (UWE)
		//   bit 4-7: unused or FOE/FOR/FOW


		m_tlb->tlbInsert(m_cpuId, realm, va, asn, walkResult.pte);

		// 6. Compute PA
		pa_out = (walkResult.pte.pfn() << PAGE_SHIFT) | extractOffset(va);
		outPte = walkResult.pte;

		return TranslationResult::Success;
	}

#pragma endregion Full Translation (Page Walk + Checks)


#pragma region EV6 Walk PageTable 


	enum class WalkStatus {

		Success,
		InvalidPTE,
		PageNotPresent,
		AccessViolation,
		BusError,
		FaultOnWrite,
		FaultOnRead
	};

	struct WalkResultEV6
	{
		bool success;
		AlphaPTE pte;
		quint64 pte_pa;
		WalkStatus status;

		enum FaultType {
			None,
			TNV,
			FOW,
			FOR_,
			FOE,
			ACV,
			BUS
		} fault;
	};


	/// \brief EV6 Page Table Walker (Layer-2 only)
	/// \param va      The virtual address to translate
	/// \param ptbr    The PTBR (from IPR)
	/// \param mode    Processor mode (Kernel/User/Exec); used for access bits
	/// \param access  Read / Write / Exec
	/// \param read64  Callback that returns 64-bit data from physical memory
	///
	/// \details
	/// This performs a full 3-level EV6 page table walk.
	/// It returns an AlphaPTE and fault information.
	/// TLB refill logic (insert into SPAM) happens ABOVE this layer.
	///
	// FIXED: Proper template syntax

	AXP_HOT inline WalkResultEV6 walkPageTable_EV6(
		VAType va,
		quint64 ptbr,
		Mode_Privilege mode,
		AccessKind access
		) noexcept
	{
		WalkResultEV6 R{};
		R.success = false;
		R.fault = WalkResultEV6::TNV;

		// --------------------------------------------------------
		// 1. Extract indices for 8KB page mode (EV6 default)
		// --------------------------------------------------------
		constexpr quint64 PAGE_SHIFT = 13;  // 8K pages
		constexpr quint64 L3_BITS = 10;
		constexpr quint64 L2_BITS = 12;
		constexpr quint64 L1_BITS = 8;

		const quint64 vpn = va >> PAGE_SHIFT;

		const quint64 idx_l1 = (vpn >> (L2_BITS + L3_BITS)) & ((1ULL << L1_BITS) - 1);
		const quint64 idx_l2 = (vpn >> L3_BITS) & ((1ULL << L2_BITS) - 1);
		const quint64 idx_l3 = vpn & ((1ULL << L3_BITS) - 1);

		// Each level entry is 8 bytes
		constexpr quint64 L1_ENTRY_SIZE = 8;
		constexpr quint64 L2_ENTRY_SIZE = 8;
		constexpr quint64 L3_ENTRY_SIZE = 8;

		// --------------------------------------------------------
		// 2. L1 lookup: PTE pointer = PTBR + idx*8
		// --------------------------------------------------------
		quint64  l1_raw = 0;
		const quint64 l1_pa = ptbr + idx_l1 * L1_ENTRY_SIZE;
		MEM_STATUS memSt1  =  m_guestMemory->read64(l1_pa, l1_raw);

		if (memSt1 != MEM_STATUS::Ok) {
			R.fault = WalkResultEV6::BUS;
			return R;
		}

		if (!(l1_raw & 0x1)) { // Valid bit #0?
			R.fault = WalkResultEV6::TNV;
			return R;
		}

		AlphaPTE l1_pte = AlphaPTE::fromRaw(l1_raw);

		// --------------------------------------------------------
		// 3. L2 lookup
		// --------------------------------------------------------

		quint64  l2_raw = 0;
		const quint64 l2_pa = (l1_pte.pfn() << PAGE_SHIFT) + idx_l2 * L2_ENTRY_SIZE;
		MEM_STATUS memSt2 = m_guestMemory->read64(l2_pa, l2_raw);

		if (memSt2 != MEM_STATUS::Ok) {
			R.fault = WalkResultEV6::BUS;
			return R;
		}

		if (!(l2_raw & 0x1)) {
			R.fault = WalkResultEV6::TNV;
			return R;
		}

		AlphaPTE l2_pte = AlphaPTE::fromRaw(l2_raw);

		// --------------------------------------------------------
		// 4. L3 lookup -> final PTE
		// --------------------------------------------------------
		quint64  l3_raw = 0;
		const quint64 l3_pa = (l2_pte.pfn() << PAGE_SHIFT) + idx_l3 * L3_ENTRY_SIZE;
		MEM_STATUS memSt3 = m_guestMemory->read64(l3_pa, l3_raw);
		if (memSt3 != MEM_STATUS::Ok) {
			R.fault = WalkResultEV6::BUS;
			return R;
		}

		if (!(l3_raw & 0x1)) {
			R.fault = WalkResultEV6::TNV;
			return R;
		}

		AlphaPTE final_pte = AlphaPTE::fromRaw(l3_raw);

		// --------------------------------------------------------
		// 5. Check access rights
		// --------------------------------------------------------

		switch (access) {
		case AccessKind::WRITE:
			if (final_pte.faultOnWrite()) { R.fault = WalkResultEV6::FOW; return R; }
			break;

		case AccessKind::READ:
			if (final_pte.faultOnRead()) { R.fault = WalkResultEV6::FOR_; return R; }
			break;

		case AccessKind::EXECUTE:
			if (final_pte.faultOnExec()) { R.fault = WalkResultEV6::FOE; return R; }
			break;

		default:
			break;
		}


		// (Optional) Check mode-specific rules
		// (depends on your AlphaPTE_Core implementation)

		// --------------------------------------------------------
		// 6. Success
		// --------------------------------------------------------
		R.success = true;
		R.fault = WalkResultEV6::None;
		R.pte = final_pte;
		R.pte_pa = l3_pa;
		return R;
	}


	// ============================================================================
	// 2. WalkStatus - INTERNAL to walkPageTable_EV6 (convert to TranslationResult)
	// ============================================================================
	// Keep WalkStatus internal, but add converter:

	static AXP_HOT inline  TranslationResult toTranslationResult(const WalkResultEV6& walk) {
		if (walk.success) return TranslationResult::Success;

		switch (walk.fault) {
		case WalkResultEV6::TNV:
			return TranslationResult::PageNotPresent;
		case WalkResultEV6::FOW:
			return TranslationResult::FaultOnWrite;
		case WalkResultEV6::FOR_:
			return TranslationResult::FaultOnRead;
		case WalkResultEV6::FOE:
			return TranslationResult::FaultOnExecute;
		case WalkResultEV6::ACV:
			return TranslationResult::AccessViolation;
		case WalkResultEV6::BUS:
			return TranslationResult::BusError;
		default:
			return TranslationResult::PageNotPresent;
		}
	}

#pragma endregion 


#pragma region SPAM Translation Helpers

	// ============================================================================
// translateVA_Data - Data TLB Translation (Hot Path)
// ============================================================================
/**
 * @brief Translate virtual address for data access (load/store)
 *
 * This is the PRIMARY translation function for memory operations.
 * Uses SPAMShardManager::tlbLookup() and handles all faults.
 *
 * @param cpuId CPU identifier
 * @param va Virtual address to translate
 * @param isWrite true for store, false for load
 * @param pa [out] Physical address (valid only if returns Success)
 * @return TranslationResult indicating success or fault type
 *
 * Side effects:
 * - On failure, queues PendingEvent in FaultDispatcher
 * - Events are delivered at normal architectural recognition point
 */
	AXP_HOT AXP_ALWAYS_INLINE TranslationResult translateVA_Data(
			quint64 va,
			quint64 pc,
			bool isWrite,
			/*out*/ quint64& pa) noexcept
	{

		// CHECK 1: PAL mode
		if (m_iprGlobalMaster->isInPalMode()) {
			pa = va;
			return TranslationResult::Success;
		}

		Mode_Privilege  mode = static_cast<Mode_Privilege>(m_hwpcb->cm);

		// CHECK 2: Physical mode
		const quint64 vaCtl = m_iprGlobalMaster->x->va_ctl;
		if ((vaCtl & 0x2) == 0) {
			pa = va;
			return TranslationResult::Success;
		}



		// CHECK 3: KSEG
		PAType kseg_pa;
		TranslationResult ksegResult = tryKsegTranslate(va, vaCtl, mode, kseg_pa);
		if (ksegResult == TranslationResult::Success) {
			pa = kseg_pa;
			return TranslationResult::Success;
		}
		if (ksegResult == TranslationResult::AccessViolation) {
			return ksegResult;
		}
		// Get current ASN
		const ASNType asn = static_cast<ASNType>(m_hwpcb->asn); 

		// TLB lookup
		PFNType pfn;
		AlphaN_S::PermMask perm;
		SC_Type sizeClass;
		const AlphaPTE* pte = nullptr;

		if (!m_tlb->tlbLookup(m_cpuId, Realm::D, va, asn, pfn, perm, sizeClass, &pte)) {
			// DTB miss - queue exception
			PendingEvent ev = makeDTBMissSingleEvent(
				m_cpuId, va, asn, pc, isWrite);
			m_fault_dispatcher->setPendingEvent(ev);
			return TranslationResult::DlbMiss;
		}

		// ========================================================================
		  // USE AlphaPTE's canWrite/canRead methods (includes FOW/FOR checks!)
		  // ========================================================================

		bool allowed = false;
		if (isWrite) {
			allowed = pte->canWrite(mode);  //  Checks FOW automatically!
		}
		else {
			allowed = pte->canRead(mode);   //  Checks FOR automatically!
		}

		if (!allowed) {
			// Determine fault type from PTE
			if (isWrite && pte->bitFOW()) {
				// Fault-on-Write (COW page)
				PendingEvent ev = makeFaultOnWriteEvent(m_cpuId, va);
				m_fault_dispatcher->setPendingEvent(ev);
				return TranslationResult::FaultOnWrite;
			}
			else if (!isWrite && pte->bitFOR()) {
				// Fault-on-Read
				PendingEvent ev = makeFaultOnReadEvent(m_cpuId, va);
				m_fault_dispatcher->setPendingEvent(ev);
				return TranslationResult::FaultOnRead;
			}
			else {
				// Regular access violation (wrong mode permissions)
				PendingEvent ev = makeDTBAccessViolationEvent(m_cpuId, va, isWrite);
				m_fault_dispatcher->setPendingEvent(ev);
				return TranslationResult::AccessViolation;
			}
		}

		// Calculate physical address
		const quint64 pageShift = PageSizeHelpers::pageShift(sizeClass);
		const quint64 pageMask = (1ULL << pageShift) - 1;
		pa = (static_cast<quint64>(pfn) << 13) | (va & pageMask);

		return TranslationResult::Success;
	}

	// ============================================================================
	// translateVA_Instruction - Instruction TLB Translation
	// ============================================================================
	/**
	 * @brief Translate virtual address for instruction fetch
	 *
	 * Uses ITB (Instruction Translation Buffer).
	 * Called during instruction fetch pipeline stage.
	 *
	 * @param cpuId CPU identifier
	 * @param va Virtual PC address
	 * @param pa [out] Physical address (valid only if returns Success)
	 * @return TranslationResult indicating success or fault type
	 */
	AXP_HOT AXP_ALWAYS_INLINE 	TranslationResult translateVA_Instruction(
			quint64 va, /*out*/ quint64& pa) noexcept
	{
		// ========================================================================
		// CHECK 1: PAL mode uses physical addressing
		// ========================================================================
		if (m_iprGlobalMaster->isInPalMode()) {
			pa = va & ~0x1ULL;  // Clear PC[0] (PAL mode bit)
			return TranslationResult::Success;
		}

		// ========================================================================
		// CHECK 2: Physical Mode (VA_CTL bit 1 = 0)
		// ========================================================================
		const quint64 vaCtl = m_iprGlobalMaster->x->va_ctl;		// 
		const bool physicalMode = (vaCtl & 0x2) == 0;	// Bit 1 = VA_MODE

		if (physicalMode) {
			pa = va;  // Identity mapping
			return TranslationResult::Success;
		}



		// ========================================================================
		// CHECK 3: KSEG (direct-mapped kernel segment)
		// ========================================================================
		const Mode_Privilege mode = static_cast<Mode_Privilege>(m_hwpcb->cm);

		PAType kseg_pa;
		TranslationResult ksegResult = tryKsegTranslate(va, vaCtl, mode, kseg_pa);
		if (ksegResult == TranslationResult::Success) {
			pa = kseg_pa;
			return TranslationResult::Success;
		}
		if (ksegResult == TranslationResult::AccessViolation)
			return ksegResult;

		const ASNType asn = static_cast<ASNType>(m_hwpcb->asn); //			Hot Path

		// ITB lookup
		PFNType pfn;
		AlphaN_S::PermMask perm;
		SC_Type sizeClass;
		const AlphaPTE* pte = nullptr;

		if (!m_tlb->tlbLookup(m_cpuId, Realm::I, va, asn, pfn, perm, sizeClass, &pte)) {
			// ITB miss - queue exception
			PendingEvent ev = makeITBMissEvent(m_cpuId, va);
			m_fault_dispatcher->setPendingEvent(ev);
			return TranslationResult::TlbMiss;
		}

		// Use AlphaPTE's canExecute method (includes FOE check!)
		//const Mode_Privilege mode = static_cast<Mode_Privilege>(m_hwpcb->cm);

		if (!pte->canExecute(mode)) {
			if (pte->bitFOE()) {
				// Fault-on-Execute
				PendingEvent ev = makeFaultOnExecuteEvent(m_cpuId, va);
				m_fault_dispatcher->setPendingEvent(ev);
				return TranslationResult::FaultOnExecute;
			}
			else {
				// Regular access violation
				PendingEvent ev = makeITBAccessViolationEvent(m_cpuId, va);
				m_fault_dispatcher->setPendingEvent(ev);
				return TranslationResult::AccessViolation;
			}
		}

		const quint64 pageShift = PageSizeHelpers::pageShift(sizeClass);
		const quint64 pageMask = (1ULL << pageShift) - 1;
		pa = (static_cast<quint64>(pfn) << 13) | (va & pageMask);

		return TranslationResult::Success;
	}

	// ============================================================================
	// translateVA_WithAlignment - Translation + Alignment Check
	// ============================================================================
	/**
	 * @brief Translate VA with alignment checking
	 *
	 * Used for sized memory operations (LDQ, STQ, LDL, STL, etc.)
	 * Checks both translation and alignment.
	 *
	 * @param cpuId CPU identifier
	 * @param va Virtual address
	 * @param accessSize Access size in bytes (1, 2, 4, 8, 16)
	 * @param isWrite true for store, false for load
	 * @param pa [out] Physical address
	 * @return TranslationResult
	 */
	AXP_HOT AXP_ALWAYS_INLINE TranslationResult translateVA_WithAlignment(

			quint64 va,
			quint64 pc,
			quint8 accessSize,
			bool isWrite,
			/*out*/ quint64& pa) noexcept
	{
		// Alignment check first (fast path - no TLB lookup if unaligned)
		const quint64 alignMask = accessSize - 1;
		if ((va & alignMask) != 0) {
			// Unaligned access - queue exception

			PendingEvent ev = makeUnalignedEvent(
				m_cpuId, va, isWrite);
			m_fault_dispatcher->setPendingEvent(ev);
			return TranslationResult::Unaligned;
		}

		// Proceed with normal translation
		return translateVA_Data( va, pc, isWrite, pa);
	}

	// ============================================================================
	// Convenience Wrappers
	// ============================================================================

	/**
	 * @brief Translate for load operation (read-only)
	 */
	AXP_HOT AXP_ALWAYS_INLINE TranslationResult translateVA_Load(

			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_Data( va, pc,  false, pa);
	}

	/**
	 * @brief Translate for store operation (write)
	 */
	AXP_HOT AXP_ALWAYS_INLINE 	TranslationResult translateVA_Store(
			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_Data( va, pc,  true, pa);
	}

	/**
	 * @brief Translate for aligned quadword load (LDQ)
	 */
	AXP_HOT AXP_ALWAYS_INLINE TranslationResult translateVA_LDQ(
			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_WithAlignment( va, pc, 8,  false, pa);
	}

	/**
	 * @brief Translate for aligned quadword store (STQ)
	 */
	AXP_HOT AXP_ALWAYS_INLINE TranslationResult translateVA_STQ(
			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_WithAlignment( va, pc, 8,  true, pa);
	}

	/**
	 * @brief Translate for aligned longword load (LDL)
	 */
	AXP_HOT AXP_ALWAYS_INLINE
		TranslationResult translateVA_LDL(
			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_WithAlignment( va, pc, 4,  false, pa);
	}

	/**
	 * @brief Translate for aligned longword store (STL)
	 */
	AXP_HOT AXP_ALWAYS_INLINE	TranslationResult translateVA_STL(
			quint64 va,
			quint64 pc,
			/*out*/ quint64& pa) noexcept
	{
		return translateVA_WithAlignment( va, pc, 4,  true, pa);
	}

	// ============================================================================
	// Stack Operation Helpers (for CHMx, exceptions, etc.)
	// ============================================================================

	/**
	 * @brief Translate and push quadword to stack
	 *
	 * Combines translation, permission check, alignment, and write.
	 * Used by CHMx, exception handlers, CALL_PAL, etc.
	 *
	 * @param cpuId CPU identifier
	 * @param sp Stack pointer (will be decremented by 8)
	 * @param value Value to push
	 * @param mode Privilege mode of target stack
	 * @return true if successful, false if exception queued
	 */
	AXP_HOT AXP_ALWAYS_INLINE	bool pushStack(
			quint64& sp,
			quint64 value,
			quint64 pc,
			PrivilegeLevel mode) noexcept
	{
		// Pre-decrement stack pointer
		sp -= 8;

		quint64 pa;
		TranslationResult result = translateVA_STQ(sp, pc,  pa);

		if (result != TranslationResult::Success) {
			// Exception already queued by translateVA_STQ
			return false;
		}

		// Write to physical address
	MEM_STATUS mStatus =	m_guestMemory->write64(pa, value);

		return true;
	}

	/**
	 * @brief Translate and pop quadword from stack
	 *
	 * @param cpuId CPU identifier
	 * @param sp Stack pointer (will be incremented by 8)
	 * @param value [out] Value popped
	 * @param mode Privilege mode of source stack
	 * @return true if successful, false if exception queued
	 */
	AXP_HOT AXP_ALWAYS_INLINE	bool popStack(
				quint64& sp,
			/*out*/ quint64& value,
			quint64 pc,
			PrivilegeLevel mode) noexcept
	{
		quint64 pa;
		TranslationResult result = translateVA_LDQ( sp, pc,  pa);

		if (result != TranslationResult::Success) {
			// Exception already queued by translateVA_LDQ
			return false;
		}

		// Read from physical address
		MEM_STATUS mStatus = m_guestMemory->read64(pa, value);

		// Post-increment stack pointer
		sp += 8;

		return true;
	}

	static AXP_HOT AXP_ALWAYS_INLINE bool checkAlignment(quint64 addr, quint8 size) noexcept
	{
		if (size == 0)
			return true;
		return (addr & (static_cast<quint64>(size) - 1ULL)) == 0ULL;
	}


	// In MBox or as a shared helper
	AXP_HOT AXP_ALWAYS_INLINE bool translateLoadAddress(
		PipelineSlot& slot,
		quint64 va,
		quint64& pa,
		MemoryAccessType accessType,
		const char* instrName) noexcept
	{
		debugLog(QString("[%1] Translating VA: 0x%2")
			.arg(instrName)
			.arg(va, 16, 16, QChar('0')));

		// ================================================================
		// Use centralized translation helper from PTE library
		// ================================================================

		bool bAccessType = (accessType == MemoryAccessType::WRITE);
		TranslationResult tr = translateVA_Data( va, slot.di.pc, bAccessType, pa);

		if (tr != TranslationResult::Success) {
			debugLog(QString("[%1]  TRANSLATION FAILED: %2")
				.arg(instrName)
				.arg(static_cast<int>(tr)));

			slot.faultPending = true;
			slot.trapCode = mapDTranslationFault(tr);
			slot.faultVA = va;
			return false;
		}

		debugLog(QString("[%1]  Translation: VA 0x%2 -> PA 0x%3")
			.arg(instrName)
			.arg(va, 16, 16, QChar('0'))
			.arg(pa, 16, 16, QChar('0')));

		return true;
	}


	AXP_HOT AXP_ALWAYS_INLINE void contextSwitch() const noexcept
	{
		// Break this CPU's reservation on context switch
		m_reservationManager->breakReservation(m_cpuId);
	}
	AXP_HOT AXP_ALWAYS_INLINE void contextSwitch(CPUIdType cpuId) const noexcept
	{
		// Break this CPU's reservation on context switch
		m_reservationManager->breakReservation(cpuId);
	}

#pragma endregion SPAM Translation Helpers


#pragma region Pal Memory Helpers

	// ============================================================================
	// VIRTUAL MEMORY READ/WRITE HELPERS
	// ============================================================================

	/**
	 * @brief Read single byte from virtual address.
	 */
	AXP_HOT  inline  MEM_STATUS readVirtualByteFromVA( quint64 va, quint8& byte) const noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::READ,  // <- READ for byte read
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}

		//GuestMemory& guestMem = global_GuestMemory();
		return m_guestMemory->read8(paOut, byte);
	}

	/**
	 * @brief Write single byte to virtual address.
	 */
	AXP_HOT  inline  MEM_STATUS writeVirtualByte(quint64 va, quint8 byte) noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::WRITE,
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}
		return m_guestMemory->write8(paOut, byte);
	}

	/**
	 * @brief Read word (16-bit) from virtual address.
	 */
	AXP_HOT  inline  MEM_STATUS readVirtualWord(quint64 va, quint16& word) noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::READ,
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}
		return m_guestMemory->read16(paOut, word);
	}

	/**
	 * @brief Read longword (32-bit) from virtual address.
	 */
	AXP_HOT  inline  MEM_STATUS readVirtualLongword( quint64 va, quint32& lw) const noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::READ,
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}
		return m_guestMemory->read32(paOut, lw);
	}

	/**
	 * @brief Read quadword (64-bit) from virtual address.
	 */
	AXP_HOT  inline  MEM_STATUS readVirtualQuadword( quint64 va, quint64& qw) const noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::READ,
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}
		return m_guestMemory->read64(paOut, qw);
	}

	/**
	 * @brief Write quadword (64-bit) to virtual address.
	 */
	AXP_HOT  inline   MEM_STATUS writeVirtualQuadword(quint64 va, quint64 qw) noexcept
	{
		quint64 paOut;
		AlphaPTE pte{};

		TranslationResult tr = ev6TranslateFastVA(
			va,
			AccessKind::WRITE,
			static_cast<Mode_Privilege>(m_hwpcb->cm),
			paOut,
			&pte
		);

		if (tr != TranslationResult::Success) {
			raiseTranslationFault(m_cpuId, va, tr, m_fault_dispatcher);
			return MEM_STATUS::TlbMiss;
		}
		return m_guestMemory->write64(paOut, qw);
	}

	// ============================================================================
	// BULK READ/WRITE HELPERS
	// ============================================================================

	/**
	 * @brief Read string from virtual memory (null-terminated or max length).
	 */
	AXP_HOT  inline   quint64 readVirtualString( quint64 va, quint8* buffer, quint64 maxLen) noexcept
	{
		if (!buffer || maxLen == 0) {
			return 0;
		}

		quint64 bytesRead = 0;

		for (quint64 i = 0; i < maxLen; i++) {
			quint8 ch;
			if (readVirtualByteFromVA( va + i, ch) != MEM_STATUS::Ok) {
				break;  // Fault
			}

			buffer[i] = ch;
			bytesRead++;

			if (ch == 0) {
				break;  // Null terminator
			}
		}

		return bytesRead;
	}

	/**
	 * @brief Write buffer to virtual memory.
	 */
	AXP_HOT  inline  quint64 writeVirtualBuffer( quint64 va, const quint8* buffer, quint64 length) noexcept
	{
		if (!buffer || length == 0) {
			return 0;
		}

		quint64 bytesWritten = 0;

		for (quint64 i = 0; i < length; i++) {
			if (writeVirtualByte( va + i, buffer[i]) != MEM_STATUS::Ok) {
				break;  // Fault
			}
			bytesWritten++;
		}

		return bytesWritten;
	}

	// ============================================================================
	// ZERO-COPY VALIDATION (CORRECTED)
	// ============================================================================

	/**
	 * @brief Check if virtual address range is readable.
	 *
	 * Validates TLB translation without actually reading memory.
	 */
	AXP_HOT  inline  bool isVirtualRangeReadable( quint64 va, quint64 length) const noexcept
	{
		// Check pages (8KB aligned)
		constexpr quint64 PAGE_SIZE_inl = 8192;
		quint64 startPage = va & ~(PAGE_SIZE_inl - 1);
		quint64 endPage = (va + length - 1) & ~(PAGE_SIZE_inl - 1);

		for (quint64 page = startPage; page <= endPage; page += PAGE_SIZE_inl) {
			quint64 paOut;
			AlphaPTE pte{};

			// Try to translate this page
			TranslationResult tr = ev6TranslateFastVA(
				page,
				AccessKind::READ,
				static_cast<Mode_Privilege>(m_hwpcb->cm),
				paOut,
				&pte
			);

			if (tr != TranslationResult::Success) {
				return false;  // Page not readable
			}

			// Optional: Check page permissions
			if (!pte.isValid() || !pte.canRead()) {
				return false;
			}
		}

		return true;  // All pages readable
	}

	/**
	 * @brief Check if virtual address range is writable.
	 */
	AXP_HOT  inline  bool isVirtualRangeWritable(quint64 va, quint64 length) const noexcept
	{
		constexpr quint64 PAGE_SIZE = 8192;
		quint64 startPage = va & ~(PAGE_SIZE - 1);
		quint64 endPage = (va + length - 1) & ~(PAGE_SIZE - 1);

		for (quint64 page = startPage; page <= endPage; page += PAGE_SIZE) {
			quint64 paOut;
			AlphaPTE pte{};

			TranslationResult tr = ev6TranslateFastVA(
				page,
				AccessKind::WRITE,
				static_cast<Mode_Privilege>(m_hwpcb->cm),
				paOut,
				&pte
			);

			if (tr != TranslationResult::Success) {
				return false;
			}

			// Check write permissions
			if (!pte.isValid() || !pte.canWrite()) {
				return false;
			}
		}

		return true;
	}

#pragma endregion Pal Memory Helpers


#pragma region TLB Helpers

	static constexpr unsigned DTB_TAG_ASN_BITS = 8;
	static constexpr quint64 DTB_TAG_ASN_MASK = (1ULL << DTB_TAG_ASN_BITS) - 1;
	/**
 * @brief Extract Virtual Page Number from TLB tag
 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
 * @return VPN (Virtual Page Number)
 */
	static AXP_HOT AXP_ALWAYS_INLINE quint64 extractVPNFromTLBTag(quint64 tag)  noexcept
	{
		// VPN is everything above ASN
		return tag >> DTB_TAG_ASN_BITS;
	}

	/**
	 * @brief Extract virtual address from TLB tag
	 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
	 * @return Page-aligned virtual address
	 */
	static AXP_HOT AXP_ALWAYS_INLINE  quint64 extractVAFromTLBTag(quint64 vpn,
		const quint8 sizeClass,
		quint64 originalVA)  noexcept
	{
		const quint64 shift = PageSizeHelpers::pageShift(sizeClass);
		const quint64 pageOffsetMask = (1ULL << shift) - 1;
		return (vpn << shift) | (originalVA & pageOffsetMask);
	}

	/**
	 * @brief Extract virtual address from TLB tag
	 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
	 * @return Page-aligned virtual address
	 */
	static AXP_HOT AXP_ALWAYS_INLINE quint64 extractVAFromTLBTag(quint64 tag)  noexcept {
		const quint64 vpnMask = 0xFFFFFFFFFFFE000ULL;  // Bits [63:13]
		return tag & vpnMask;
	}

	/**
	 * @brief Extract ASN from TLB tag
	 * @param tag TLB_TAG value (from DTB_TAG or ITB_TAG IPR)
	 * @return Address Space Number (bits 12:5)
	 */
	static AXP_HOT AXP_ALWAYS_INLINE ASNType extractASNFromTLBTag(quint64 tag)  noexcept {
		return (tag >> 5) & 0xFF;  // Bits [12:5]
	}

	static AXP_HOT AXP_ALWAYS_INLINE  PFNType extractPFNFromPTE(quint64 pteRaw)  noexcept
	{
		// Canonical Alpha memory PTE: PFN in bits 63..32
		// Mask to the width you actually implement (28 bits here)
		return static_cast<PFNType>((pteRaw >> 32) & ((1ULL << 28) - 1));
	}


	// ========================================================================
	// Private Helpers - IPR Field Extraction
	// ========================================================================

	static AXP_HOT AXP_ALWAYS_INLINE VAType extractVA_fromTag(quint64 tagValue) noexcept
	{
		return tagValue & ~0x1FFFULL;  // Clear lower 13 bits
	}

	static AXP_HOT AXP_ALWAYS_INLINE ASNType extractASN_fromTag(quint64 tagValue) noexcept
	{
		// TODO: Verify ASN location in your TAG format
		return static_cast<ASNType>(tagValue & 0xFF);
	}


	static AXP_HOT AXP_ALWAYS_INLINE  SC_Type extractSizeClassFromPTE(const AlphaPTE& pte)   noexcept
	{
		return pte.gh();
	}

#pragma endregion TLB Helpers



#pragma region PalAtomics 


	// ------------------------------------------------------------------------
// Lock striping: 4096 locks keeps contention low
// Uses quadword-granular locking for atomicity
// ------------------------------------------------------------------------
	static constexpr quint32 kLockStripeCount = 4096;

	struct LockStripes final
	{
		std::array<QMutex, kLockStripeCount> locks;

		AXP_HOT AXP_FLATTEN QMutex& lockForPA(quint64 pa) noexcept
		{
			// Hash: drop low 3 bits (quadword aligned), then mix
			const quint64 q = (pa >> 3);
			const quint32 idx = static_cast<quint32>(
				(q ^ (q >> 11) ^ (q >> 23)) & (kLockStripeCount - 1));
			return locks[idx];
		}
	};

	// Meyers singleton (thread-safe in C++11+)
	static AXP_HOT inline  LockStripes& globalLockStripes() noexcept
	{
		static LockStripes stripes;
		return stripes;
	}

	// ------------------------------------------------------------------------
	// GuestMemory Bridge Functions
	// ------------------------------------------------------------------------

	/**
	 * @brief Read quadword from physical address via GuestMemory
	 *
	 * GuestMemory automatically routes to RAM or MMIO as appropriate.
	 *
	 * @param guestMem GuestMemory instance
	 * @param pa Physical address (must be 8-byte aligned)
	 * @param outValue [out] Value read
	 * @return true on success, false on error
	 */
	/*AXP_FLATTEN MEM_STATUS guestMemoryReadPA_Quad(
		GuestMemory* guestMem,
		quint64 pa,
		quint64& outValue) noexcept
	{
		if (!guestMem) {
			return MEM_STATUS::OutOfRange;
		}

		// GuestMemory::readPA handles RAM vs MMIO routing
		return guestMem->readPA(pa, &outValue, sizeof(quint64));
	}*/

	/**
	 * @brief Write quadword to physical address via GuestMemory
	 *
	 * GuestMemory automatically routes to RAM or MMIO as appropriate.
	 *
	 * @param guestMem GuestMemory instance
	 * @param pa Physical address (must be 8-byte aligned)
	 * @param value Value to write
	 * @return true on success, false on error
	 */
	AXP_HOT  AXP_ALWAYS_INLINE  MEM_STATUS guestMemoryWritePA_Quad(
		quint64 pa,
		quint64 value) const noexcept
	{
		if (!m_guestMemory) {
			return MEM_STATUS::OutOfRange;
		}

		// GuestMemory::writePA handles RAM vs MMIO routing
		return m_guestMemory->writePA(pa, &value, sizeof(quint64));
	}

	// ------------------------------------------------------------------------
	// atomicExchangePA_Quad - Main Entry Point
	// ------------------------------------------------------------------------
	/**
	 * @brief Atomic exchange at physical address
	 *
	 * Performs atomic read-modify-write:
	 *   1. Read old value from [pa]
	 *   2. Write new value to [pa]
	 *   3. Return old value
	 *
	 * Atomicity provided by lock striping across 4096 mutexes.
	 * GuestMemory handles RAM vs MMIO routing transparently.
	 *
	 * @param guestMem GuestMemory instance
	 * @param pa Physical address (must be 8-byte aligned)
	 * @param newValue Value to write
	 * @param oldValue [out] Previous value at address
	 * @return true on success, false on alignment error or bus error
	 *
	 * Thread-safe: uses lock striping to prevent concurrent access to same PA.
	 */
	AXP_HOT AXP_ALWAYS_INLINE bool atomicExchangePA_Quad(
		quint64 pa,
		quint64 newValue,
		quint64& oldValue) const noexcept
	{
		if ((pa & 0x7ULL) != 0 || !m_guestMemory)
			return false;

		QMutexLocker locker(&globalLockStripes().lockForPA(pa));

		quint64 tmpOld = 0;
		if (m_guestMemory->readPA(pa, &tmpOld, sizeof(quint64)) != MEM_STATUS::Ok)
			return false;

		if (m_guestMemory->writePA(pa, &newValue, sizeof(quint64)) != MEM_STATUS::Ok)
			return false;

		oldValue = tmpOld;
		return true;
	}

#pragma endregion PalAtomics
};

#endif
