#ifndef MemoryExecutionStages_h__
#define MemoryExecutionStages_h__
#include <QtGlobal>
#include "PipeLineSlot.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "../MBoxLib_EV6/MBoxBase.h"
#include "../EBoxLib/EBoxBase.h"
#include "../CBoxLib/CBoxBase.h"
#include "../faultLib/IFaultSink.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../exceptionLib/ExceptionFactory.h"
#include "../memoryLib/global_guestMemory.h"
#include "../memoryLib/GuestMemory.h"
#include "../coreLib/arithExtender_helpers.h"
#include "../cpuCoreLib/ExecutionResult.h"
#include "../coreLib/enum_header.h"
#include "../pteLib/AlphaPTE_Core.h"

// machineLib::MachineStateLayer
// Purpose: Execute stage work, return status to AlphaPipeline
// Contract: No orchestration, no routing - pure execution + status

namespace machineLib {

// -------------------------------------------------------------------
// UNIFIED STATUS - The only return type to AlphaPipeline
// -------------------------------------------------------------------
enum class StageStatus {
    Continue,       // Proceed to next stage
    Stall,          // Retry this stage next cycle
    Fault,          // Fault captured in slot, flush required
    EnterPAL,       // PAL entry required
    Complete        // Instruction retired successfully
};

/*
	// - di.ra: Destination register for loads (LDQ, LDL, etc.)
	// - di.rb: Base register for EA calculation (Rb + displacement)
	// - di.rc: Source data register for stores (STQ, STL, etc.)
*/
enum class RegisterType {
	Dest,	// Destination (loads, ALU results)
	Base,	// Base register (EA calculation)
	Source	// Source data (stores, ALU operand)
};
// -------------------------------------------------------------------
// STAGE CONTEXT - Everything a stage needs (passed by reference)
// -------------------------------------------------------------------
struct StageContext {
    PipelineSlot&           slot;
    AlphaProcessorContext&  ctx;
    MBox&                   mbox;
    EBox&                   ebox;
    CBox&                   cbox;

    // Fault sink for deferred fault delivery
    IFaultSink&              faults;
};

// -------------------------------------------------------------------
// MEMORY STAGES - Flat, collapsible, peer functions
// -------------------------------------------------------------------
namespace MemoryStages {

	// Stage 1: Calculate Effective Address - slot.va
	AXP_HOT AXP_FLATTEN StageStatus calculateEA(StageContext& sc) noexcept
	{
		auto& slot = sc.slot;

		// EA = Rb + displacement (for loads/stores)
		const quint64 base = sc.ctx.readIntReg(slot.di.rb);
		const qint16 disp = extractMemDisp(slot.di);

		slot.va = base + static_cast<qint64>(disp);  // <- VA set here

		return StageStatus::Continue;
	}

    // Stage 2: Virtual-to-Physical Translation
    // ============================================================================
    // machineLib::MemoryStages - Translation Stage
    // ============================================================================
    // The translation helper already creates faults and sets them on the sink.
    // This stage just orchestrates and returns status to AlphaPipeline.
    // ============================================================================


    /**
     * @brief Translate VA -> PA for data access
     *
     * Calls your existing translateVA_Data() which:
     * - Performs TLB lookup via globalEv6SPAM()
     * - Checks permissions based on current mode
     * - Creates and queues faults via IFaultSink on failure
     *
     * This stage just maps TranslationResult -> StageStatus.
     */
	AXP_HOT AXP_FLATTEN StageStatus translateVA(StageContext& sc) noexcept
	{
		auto& slot = sc.slot;
		const bool isWrite = isStore(slot.di);

		// translateVA_Data sets slot.pa on success, queues fault on failure
		TranslationResult tr = translateVA_Data(
			slot.cpuId,
			slot.va,            // <- input: VA from stage 1
			slot.di.pc,
			&sc.faults,
			isWrite,
			slot.pa             // <- output: PA set here on success
		);

		switch (tr) {
		case TranslationResult::Success:
			return StageStatus::Continue;
		case TranslationResult::DTB_Miss:
			return StageStatus::EnterPAL;   // Pipeline will flush & enter PAL
		default:
			return StageStatus::Fault;
		}
	}

    /**
     * @brief Translate VA for instruction fetch (ITB path)
     */
    AXP_HOT AXP_FLATTEN StageStatus translateVA_Fetch(StageContext& sc) noexcept
    {
        auto& slot = sc.slot;

        TranslationResult tr = translateVA_Instruction(
            slot.cpuId,
            slot.va,           // fetch VA (PC)
            &sc.faults,
            slot.pa
        );

        switch (tr) {
        case TranslationResult::Success:
            return StageStatus::Continue;

        case TranslationResult::ITB_Miss:
            return StageStatus::EnterPAL;

        case TranslationResult::AccessViolation:
            return StageStatus::Fault;

        default:
            return StageStatus::Fault;
        }
    }

    /**
     * @brief Translate with explicit alignment check
     *
     * For sized memory operations (LDQ, STQ, LDL, STL, etc.)
     */
    AXP_HOT AXP_FLATTEN StageStatus translateVA_Aligned(StageContext& sc, quint8 accessSize) noexcept
    {
        auto& slot = sc.slot;
        const bool isWrite = isStore(slot.di);

        TranslationResult tr = translateVA_WithAlignment(
            slot.cpuId,
            slot.va,
            slot.di.pc,
            accessSize,
            &sc.faults,
            isWrite,
            slot.pa
        );

        switch (tr) {
        case TranslationResult::Success:
            return StageStatus::Continue;

        case TranslationResult::DTB_Miss:
            return StageStatus::EnterPAL;

        case TranslationResult::Unaligned:
        case TranslationResult::AccessViolation:
        case TranslationResult::FaultOnRead:
        case TranslationResult::FaultOnWrite:
        default:
            return StageStatus::Fault;
        }
    }


	/*
	// Register field usage in memory operations:
	// - di.ra: Destination register for loads (LDQ, LDL, etc.)
	// - di.rb: Base register for EA calculation (Rb + displacement)
	// - di.rc: Source data register for stores (STQ, STL, etc.)

    *** writeback is handled in the pipeline stage_WB. 
	*/

// Stage 3: Physical Memory Access
	AXP_HOT AXP_FLATTEN StageStatus accessMemory(StageContext& sc) noexcept
	{
		auto& slot = sc.slot;

		if (isLoad(slot.di)) {
			quint8 accessSize = getMemSize(slot.di);
			// Use slot.pa directly - no PTE needed
			GuestMemory* memory = &global_GuestMemory();
			if (memory->read64(slot.pa, slot.outPAData) != MEM_STATUS::Ok) {
				slot.faultSink->setPendingEvent(makeMemoryFault(slot.va));
				slot.faultPending = true;
				return StageStatus::Fault;
			}
			slot.memResultValid = true;  // Mark that memory result is valid

		}
		else if (isStore(slot.di)) {
			quint8 accessSize = getMemSize(slot.di);
			// Read source data from register for store
			quint64 storeData = sc.ctx.readIntReg(slot.di.rc);

			GuestMemory* memory = &global_GuestMemory();
			if (memory->write64(slot.pa, storeData) != MEM_STATUS::Ok) {
				slot.faultSink->setPendingEvent(makeMemoryFault(slot.va));
				slot.faultPending = true;
				return StageStatus::Fault;
			}
		}

		return StageStatus::Continue;
	}




	// Convert semantic instruction fields to register numbers
	AXP_FLATTEN quint8 convertSToRegister(const DecodedInstruction& di, RegisterType regType) noexcept
	{
		switch (regType) {
		case RegisterType::Dest:   return di.ra;  // Destination (loads, ALU results)
		case RegisterType::Base:   return di.rb;  // Base register (EA calculation) 
		case RegisterType::Source: return di.rc;  // Source data (stores, ALU operand)
		default: return 31; // R31 = zero register
		}
	}
} // namespace MemoryStages


	// ========================================================================
	// FINAL: Ultra-Optimized Translation with Triple-Layer Caching
	// ========================================================================
AXP_HOT AXP_FLATTEN StageStatus translateVA(StageContext& sc) noexcept
{
	auto& slot = sc.slot;
	const bool isWrite = isStore(slot.di);
	const CPUIdType cpuId = sc.ctx.cpuId();
	const ASNType currentASN = getASN_Active(cpuId);

	// ========================================================================
	// LAYER 1: Staged PTE Cache (ultra-fast path) - ~2 cycles
	// ========================================================================
	if (slot.dtbValid && slot.ptbStage.matches(slot.va, Realm::D)) {
		quint64 pa;
		if (slot.ptbStage.translateWithStagedPTE(slot.va, pa, Realm::D)) {
			slot.pa = pa;
			return StageStatus::Continue;
		}
	}

	// ========================================================================
	// LAYER 2: TLB Cache (fast path) - ~5 cycles  
	// ========================================================================
	auto& spam = globalEv6Silicon().spam();
	PFNType pfn = 0;
	AlphaN_S::PermMask perm = 0;
	SC_Type sizeClass = 0;

	if (spam.tlbLookup(cpuId, Realm::D, slot.va, currentASN, pfn, perm, sizeClass)) {
		// TLB HIT - Update staged cache
		slot.ptbStage.setPFN(pfn, Realm::D);
		slot.ptbStage.setPermMask(perm, Realm::D);
		slot.ptbStage.setSizeClass(sizeClass, Realm::D);
		slot.dtbValid = true;
		slot.dtbTr = TranslationResult::Success;

		slot.pa = (static_cast<quint64>(pfn) << 13) | (slot.va & 0x1FFF);
		return StageStatus::Continue;
	}

	// ========================================================================
	// LAYER 3: Page Walk (slow path) - FULLY INLINED
	// ========================================================================
	const quint64 ptbr = getPTBR_Active(cpuId);

	// Page walk constants
	constexpr quint64 PAGE_SHIFT = 13, L3_BITS = 10, L2_BITS = 12, L1_BITS = 8;
	const quint64 vpn = slot.va >> PAGE_SHIFT;
	const quint64 idx_l1 = (vpn >> (L2_BITS + L3_BITS)) & ((1ULL << L1_BITS) - 1);
	const quint64 idx_l2 = (vpn >> L3_BITS) & ((1ULL << L2_BITS) - 1);
	const quint64 idx_l3 = vpn & ((1ULL << L3_BITS) - 1);

	// Memory reader
	auto& guestMem = global_GuestMemory();
	auto readMem64 = [&](quint64 pa) -> quint64 {
		quint64 data;
		return (guestMem.read64(pa, data) == MEM_STATUS::Ok) ? data : 0;
		};

	// L1 -> L2 -> L3 walk
	const quint64 l1_raw = readMem64(ptbr + idx_l1 * 8);
	if (!(l1_raw & 1)) goto dtb_miss;

	const quint64 l2_raw = readMem64((AlphaPTE::fromRaw(l1_raw).pfn() << PAGE_SHIFT) + idx_l2 * 8);
	if (!(l2_raw & 1)) goto dtb_miss;

	const quint64 l3_raw = readMem64((AlphaPTE::fromRaw(l2_raw).pfn() << PAGE_SHIFT) + idx_l3 * 8);
	if (!(l3_raw & 1)) goto dtb_miss;

	// ========================================================================
	// Access Rights Check & Success
	// ========================================================================
	{
		AlphaPTE final_pte = AlphaPTE::fromRaw(l3_raw);

		// Access rights check
		if (isWrite && final_pte.faultOnWrite()) goto access_violation;
		if (!isWrite && final_pte.faultOnRead()) goto access_violation;

		// SUCCESS: Update all caches
		spam.tlbInsert(cpuId, Realm::D, slot.va, currentASN, final_pte);
		slot.ptbStage.stageDTBEntry(slot.va, currentASN, final_pte);
		slot.dtbValid = true;
		slot.dtbTr = TranslationResult::Success;

		slot.pa = (final_pte.pfn() << PAGE_SHIFT) | (slot.va & 0x1FFF);
		return StageStatus::Continue;
	}

	// ========================================================================
	// Fault Paths - CORRECTED to match MemoryExecutionStages.h
	// ========================================================================
dtb_miss:
	slot.dtbTr = TranslationResult::DTB_Miss;
	slot.dtbValid = false;
	// Use the same fault creation pattern as accessMemory()
	slot.faultSink->setPendingEvent(makeMemoryFault(slot.va));
	slot.faultPending = true;
	return StageStatus::EnterPAL;

access_violation:
	slot.dtbTr = isWrite ? TranslationResult::FaultOnWrite : TranslationResult::FaultOnRead;
	slot.dtbValid = false;
	// Use consistent fault creation
	slot.faultSink->setPendingEvent(makeMemoryFault(slot.va));
	slot.faultPending = true;
	return StageStatus::Fault;
}


} // namespace machineLib
#endif // MemoryExecutionStages_h__
