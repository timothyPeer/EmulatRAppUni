// ============================================================================
// MBoxBase.h - Memory Box (MBox) Implementation
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Header-only Memory Box implementation with integrated TLB management.
//   Handles all memory operations: loads, stores, translations, and TLB
//   staging for both PAL IPR updates and hardware miss handling.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef MBOX_HEADERONLY_H
#define MBOX_HEADERONLY_H
#include "../machineLib/PipeLineSlot.h"
#include "../pteLib/AlphaPTE_Core.h"
#include "../pteLib/Ev6SiliconTLB_Singleton.h"
#include "../pteLib/alpha_pte_core.h"
#include "../coreLib/types_core.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../memoryLib/global_GuestMemory.h"
#include <QtGlobal>
#include <intrin0.inl.h>

#include "../coreLib/VA_types.h"
#include "coreLib/BoxRequest.h"
#include "coreLib/alpha_fp_helpers_inl.h"
#include "coreLib/fp_variant_core.h"
#include "cpuCoreLib/StagedPTECache.h"
#include "faultLib/FaultDispatcher.h"
#include "faultLib/global_faultDispatcher.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "memoryLib/GuestMemory.h"
#include "memoryLib/memory_core.h"

#include "coreLib/register_core_inl.h"
#include "cpuCoreLib/ReservationManager.h"
#include "cpuCoreLib/global_ReservationManager.h"
#include "exceptionLib/ExceptionFactory.h"
#include "pteLib/calculateEffectiveAddress.h"
#include "coreLib/EXECTRACE_Macros.h"
#include "coreLib/VA_core.h"
#include "machineLib/PipeLineSlot_inl.h"
#include "coreLib/IEEE754_FloatConversion_inl.h"
#include "pteLib/ev6Translation_struct.h"
// Forward declarations



#define MBOX_COMPONENT "MBox"

// ============================================================================
// MBox - Memory Management Box (Header-Only)
// ============================================================================
class MBox final
{


public:
    // ========================================================================
    // Construction
    // ========================================================================

    explicit MBox(CPUIdType cpuId) noexcept
        : m_cpuId(cpuId)
        , m_guestMemory(&global_GuestMemory())
        , m_faultSink(&globalFaultDispatcher(cpuId))
        , m_loadPending(false)
        , m_storePending(false)
        , m_reservationManager(&globalReservationManager())
        , m_iprGlobalMaster(getCPUStateView(cpuId))

    {
        clearIPRStaging();
        clearMissStaging();
        m_ev6Translator.reset(new Ev6Translator(cpuId));
    }

    // Disable copy/move
    MBox(const MBox&)            = delete;
    MBox& operator=(const MBox&) = delete;

    AXP_HOT AXP_ALWAYS_INLINE void executeBSR(PipelineSlot& slot) noexcept
    {
        const quint64 srcA = slot.readIntReg(slot.di.ra);

        quint64 result;
        if (srcA == 0) {
            result = 0;
        }
        else {
#if defined(__GNUC__) || defined(__clang__)
            result = 63 - __builtin_clzll(srcA);
#elif defined(_MSC_VER)
            unsigned long index;
            _BitScanReverse64(&index, srcA);
            result = index;
#else
            result = 0;
            quint64 temp = srcA;
            while (temp >>= 1) result++;
#endif
        }

               slot.payLoad = result;
        slot.needsWriteback = true;
    }



    AXP_HOT AXP_ALWAYS_INLINE void executeSRL(PipelineSlot& slot) noexcept
    {
        WARN_LOG(QString("CPU %1: SRL routed to MBox (should be EBox)").arg(slot.cpuId));

        const quint64 srcA = slot.readIntReg(slot.di.ra);
        const quint64 srcB = slot.readIntReg(slot.di.rb);
        const quint64 shiftAmount = srcB & 0x3F;

        const quint64 result = srcA >> shiftAmount;

        slot.payLoad = result;
        slot.needsWriteback = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE void executeCTPOP(PipelineSlot& slot) noexcept
    {
        const quint64 srcB = slot.readIntReg(slot.di.rb);

        quint64 count;
#if defined(__GNUC__) || defined(__clang__)
        count = __builtin_popcountll(srcB);
#elif defined(_MSC_VER)
        count = __popcnt64(srcB);
#else
        count = 0;
        quint64 temp = srcB;
        while (temp) {
            temp &= temp - 1;
            count++;
        }
#endif

        slot.payLoad = count;
        slot.needsWriteback = true;
    }




    AXP_HOT AXP_ALWAYS_INLINE void executeCTLZ(PipelineSlot& slot) noexcept
    {
        const quint64 srcB = slot.readIntReg(slot.di.rb);

        quint64 count;
        if (srcB == 0) {
            count = 64;
        }
        else {
#if defined(__GNUC__) || defined(__clang__)
            count = __builtin_clzll(srcB);
#elif defined(_MSC_VER)
            unsigned long index;
            _BitScanReverse64(&index, srcB);
            count = 63 - index;
#else
            count = 0;
            quint64 mask = 0x8000000000000000ULL;
            while ((srcB & mask) == 0 && count < 64) {
                mask >>= 1;
                count++;
            }
#endif
        }

        slot.payLoad = count;
        slot.needsWriteback = true;
    }

    auto executeECB(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto executeWH64(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto executeWH64EN(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }
   
    auto executeFETCH(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto executeFETCH_M(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto executeREAD_UNQ(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto executeRDUNIQUE_64(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
    auto executeRPCC(PipelineSlot& slot) -> void {
		static bool warned = false;
		if (!warned) {
			ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
			warned = true;
		}
	}
    auto executeWRUNIQUE_64(PipelineSlot& slot) -> void {
        static bool warned = false;
        if (!warned) {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    auto invalidateCachedTranslations() -> void
    {
        globalSPAM(m_cpuId).invalidateAllTLBs(m_cpuId);
    }
    MBox(MBox&&) = delete;
    MBox& operator=(MBox&&) = delete;

    AXP_HOT AXP_ALWAYS_INLINE bool isBusy() const noexcept
    {
        return m_isBusy;
    }



    AXP_HOT AXP_ALWAYS_INLINE bool shouldBypassTLB(quint64 va) const noexcept {
        // TLB is bypassed if:
        // 1. Address is in KSEG (always direct-mapped)
        // 2. Address is in low physical region (early boot)
        // 3. CPU is in physical mode (va_ctl check - if initialized)

        if (CPUStateView::isKseg(va)) return true;
        if (CPUStateView::isPhysicalSegment(va)) return true;

        // Only check va_ctl for middle ranges
        const quint64 vaCtl = m_iprGlobalMaster->x->va_ctl;
        return (vaCtl & 0x2) == 0;  // Bit 1 = VA_MODE
    }
    // ========================================================================
    // PUBLIC API - PAL IPR Staging (HW_MTPR/HW_REI Flow)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE void stageIPR_ITBPTE(quint64 pteValue) noexcept
    {
        m_iprStagedITB.pte = pteValue;
        m_iprStagedITB.hasPTE = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE void stageIPR_ITBTAG(quint64 tagValue) noexcept
    {
        m_iprStagedITB.tag = tagValue;
        m_iprStagedITB.hasTag = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE void stageIPR_DTBPTE(quint64 pteValue) noexcept
    {
        m_iprStagedDTB.pte = pteValue;
        m_iprStagedDTB.hasPTE = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE void stageIPR_DTBTAG(quint64 tagValue) noexcept
    {
        m_iprStagedDTB.tag = tagValue;
        m_iprStagedDTB.hasTag = true;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool commitIPRStagedITB() noexcept
    {
        if (!m_iprStagedITB.isComplete()) {
            WARN_LOG(QString("CPU %1: Incomplete ITB IPR staging - missing %2")
                .arg(m_cpuId)
                .arg(!m_iprStagedITB.hasPTE ? "PTE" : "TAG"));
            return false;
        }

        // Extract VA and ASN from TAG
        VAType va = m_ev6Translator.data()->extractVA_fromTag(m_iprStagedITB.tag);
        ASNType asn = m_ev6Translator.data()->extractASN_fromTag(m_iprStagedITB.tag);

        // Convert raw PTE value to AlphaPTE structure
        AlphaPTE pte;
        pte.raw = m_iprStagedITB.pte;

        // Validate PTE is valid before inserting
        if (!pte.bitV()) {
            WARN_LOG(QString("CPU %1: Attempted to commit invalid ITB PTE (V=0)")
                .arg(m_cpuId));
            clearIPRStaging();
            return false;
        }

        // Insert into silicon ITB
        auto& spam = globalEv6SPAM();
        bool success = spam.tlbInsert(m_cpuId, Realm::I, va, asn, pte);

        if (success) {
            DEBUG_LOG(QString("CPU %1: ITB entry committed - VA=0x%2 ASN=%3 PFN=0x%4")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0'))
                .arg(asn)
                .arg(pte.pfn(), 8, 16, QChar('0')));
        }
        else {
            ERROR_LOG(QString("CPU %1: ITB insertion failed - VA=0x%2")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0')));
        }

        // Clear staging regardless of success
        m_iprStagedITB.clear();

        return success;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool commitIPRStagedDTB() noexcept
    {
        if (!m_iprStagedDTB.isComplete()) {
            WARN_LOG(QString("CPU %1: Incomplete DTB IPR staging - missing %2")
                .arg(m_cpuId)
                .arg(!m_iprStagedDTB.hasPTE ? "PTE" : "TAG"));
            return false;
        }

        // Extract VA and ASN from TAG
        VAType va = m_ev6Translator.data()->extractVA_fromTag(m_iprStagedDTB.tag);
        ASNType asn =  m_ev6Translator.data()->extractASN_fromTag(m_iprStagedDTB.tag);

        // Convert raw PTE value to AlphaPTE structure
        AlphaPTE pte;
        pte.raw = m_iprStagedDTB.pte;

        // Validate PTE is valid before inserting
        if (!pte.bitV()) {
            WARN_LOG(QString("CPU %1: Attempted to commit invalid DTB PTE (V=0)")
                .arg(m_cpuId));
            clearIPRStaging();
            return false;
        }

        // Insert into silicon DTB
        auto& spam = globalEv6SPAM();
        bool success = spam.tlbInsert(m_cpuId, Realm::D, va, asn, pte);

        if (success) {
            DEBUG_LOG(QString("CPU %1: DTB entry committed - VA=0x%2 ASN=%3 PFN=0x%4")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0'))
                .arg(asn)
                .arg(pte.pfn(), 8, 16, QChar('0')));
        }
        else {
            ERROR_LOG(QString("CPU %1: DTB insertion failed - VA=0x%2")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0')));
        }

        // Clear staging regardless of success
        m_iprStagedDTB.clear();

        return success;
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasCompleteIPRStagedITB() const noexcept
    {
        return m_iprStagedITB.isComplete();
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasCompleteIPRStagedDTB() const noexcept
    {
        return m_iprStagedDTB.isComplete();
    }

    AXP_HOT AXP_ALWAYS_INLINE void clearIPRStaging() noexcept
    {
        m_iprStagedITB.clear();
        m_iprStagedDTB.clear();
    }

    // ========================================================================
    // PUBLIC API - TLB Miss Handler Staging
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE void stageTLBMissEntry(
        VAType va,
        ASNType asn,
        const AlphaPTE& pte,
        Realm realm) noexcept
    {
        if (realm == Realm::D) {
            m_missStaging.stageDTBEntry(va, asn, pte);
        }
        else {
            m_missStaging.stageITBEntry(va, asn, pte);
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasStagedEntryFor(
        VAType va,
        ASNType asn,
        Realm realm) const noexcept
    {
        return m_missStaging.matches(va, asn, realm);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool translateWithStagedEntry(
        VAType va,
        quint64& pa,
        Realm realm) const noexcept
    {
        return m_missStaging.translateWithStagedPTE(va, pa, realm);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool commitStagedTLBEntry(Realm realm) noexcept
    {
        const AlphaPTE& pte = m_missStaging.getStagedPTE(realm);

        if (!pte.bitV()) {
            WARN_LOG(QString("CPU %1: No valid staged entry to commit for realm %2")
                .arg(m_cpuId)
                .arg(realm == Realm::I ? "I" : "D"));
            return false;
        }

        // Get VA and ASN from staging cache
        VAType va = m_missStaging.m_stagedVA;
        ASNType asn = m_missStaging.m_stagedASN;

        // Insert into silicon TLB
        auto& spam = globalEv6SPAM();
        bool success = spam.tlbInsert(m_cpuId, realm, va, asn, pte);

        if (success) {
            DEBUG_LOG(QString("CPU %1: TLB miss entry committed - Realm=%2 VA=0x%3 ASN=%4")
                .arg(m_cpuId)
                .arg(realm == Realm::I ? "I" : "D")
                .arg(va, 16, 16, QChar('0'))
                .arg(asn));

            // Clear staging after successful commit
            clearMissStaging();
        }
        else {
            ERROR_LOG(QString("CPU %1: TLB miss entry commit failed")
                .arg(m_cpuId));
        }

        return success;
    }

    AXP_HOT AXP_ALWAYS_INLINE void clearMissStaging() noexcept
    {
        m_missStaging.clear();
    }

    AXP_HOT AXP_ALWAYS_INLINE void buildStagedPTE(
        PFNType pfn,
        PermMask permMask,
        SC_Type sizeClass,
        Realm realm) noexcept
    {
        m_missStaging.setPFN(pfn, realm);
        m_missStaging.setPermMask(permMask, realm);
        m_missStaging.setSizeClass(sizeClass, realm);
    }

    // ========================================================================
    // PUBLIC API - TLB Operations
    // ========================================================================



    AXP_HOT AXP_ALWAYS_INLINE void invalidateTLB(
        VAType va = 0,
        ASNType asn = 0,
        Realm realm = Realm::Both) noexcept
    {
        auto& spam = globalEv6SPAM();

        if (va == 0 && asn == 0) {
            // Invalidate all entries
            if (realm == Realm::Both || realm == Realm::I) {
                spam.invalidateAllTLBs(m_cpuId);
                DEBUG_LOG(QString("CPU %1: ITB invalidated (all entries)")
                    .arg(m_cpuId));
            }
            if (realm == Realm::Both || realm == Realm::D) {
                spam.invalidateAllTLBs(m_cpuId);
                DEBUG_LOG(QString("CPU %1: DTB invalidated (all entries)")
                    .arg(m_cpuId));
            }
        }
        else if (asn == 0) {
            // Invalidate by VA (all ASNs)
            if (realm == Realm::Both || realm == Realm::I) {
                spam.invalidateDTBEntry(m_cpuId, va, asn);
            }
            if (realm == Realm::Both || realm == Realm::D) {
                spam.invalidateITBEntry(m_cpuId, va, asn);
            }
            DEBUG_LOG(QString("CPU %1: TLB invalidated - VA=0x%2")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0')));
        }
        else {
            // Invalidate specific VA + ASN
            if (realm == Realm::Both || realm == Realm::I) {
                spam.invalidateITBEntry(m_cpuId, va, asn);
            }
            if (realm == Realm::Both || realm == Realm::D) {
                spam.invalidateDTBEntry(m_cpuId, va, asn);
            }
            DEBUG_LOG(QString("CPU %1: TLB invalidated - VA=0x%2 ASN=%3")
                .arg(m_cpuId)
                .arg(va, 16, 16, QChar('0'))
                .arg(asn));
        }

        // Also clear staging areas (might contain stale entries)
        clearIPRStaging();
        clearMissStaging();
    }


    // When MBox STARTS a memory operation:
    BoxResult executeLoadWithPA(PipelineSlot& slot) noexcept {
        m_isBusy = true;  //  Set busy
        // Calculate EA with automatic fault handling
        quint64 va;
        if (!calculateEffectiveAddress(slot)) {
            return BoxResult().faultDispatched();  // Exception already dispatched
        }

        // Translate VA -> PA
        quint64 pa;
        AlphaPTE pte{};

        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "ExecuteLoadWithPA")) {  //  Was "STL"
            // Translation fault already set in slot by helper
            slot.needsWriteback = false;
            m_isBusy = false;
            return BoxResult().faultDispatched();
        }

       
        // Read from memory
        quint64 value;
        if (m_guestMemory->read64(pa, value) != MEM_STATUS::Ok) {
            auto ev = makeMemoryFault(slot.cpuId, va);
            slot.m_faultDispatcher->setPendingEvent(ev);
            slot.va = va;
            slot.faultPending = true;
            return BoxResult().faultDispatched();
        }
        // 3. Store result
        slot.payLoad = pa;
        slot.faultPending = false;
        m_isBusy = false;  //  Clear when done
        return BoxResult().advance();
    }

    BoxResult executeStoreWithPA(PipelineSlot& slot) noexcept {
        m_isBusy = true;  //  Set busy

        // Calculate EA with automatic fault handling
        quint64 va;
        if (!calculateEffectiveAddress(slot)) {
            return BoxResult().faultDispatched();  // Exception already dispatched
        }

        // Translate VA -> PA
        quint64 pa;

        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "StoreWithPA")) {
            // Translation fault already set in slot by helper
            slot.needsWriteback = false;
            m_isBusy = false;
            return BoxResult().faultDispatched();
        }
        // Read from memory
        quint64 value;
        if (global_GuestMemory().write16(pa, value) != MEM_STATUS::Ok) {
            auto ev = makeMemoryFault(slot.cpuId, va);
            slot.m_faultDispatcher->setPendingEvent(ev);
            return BoxResult().faultDispatched();
        }
        // 3. Store result
        slot.payLoad = pa;
        slot.faultPending = false;
        m_isBusy = false;  //  Clear when done
        debugMemory("executeStoreWithPA", slot, false, slot.va, slot.payLoad, 8);
        return BoxResult().advance();
    }

  

 
    AXP_HOT AXP_ALWAYS_INLINE void  executeLDF(PipelineSlot& slot)  noexcept
    {
        quint64 raw = 0;
        quint64 pa = 0;
        auto result = executeLoadWithPA(slot);
        if (!result.hasAnyFlags()) // function is a TODO - not yet written
        {
            // we have a fault
            slot.faultPending = true;
            slot.pa = pa;
            slot.outPAData = pa;
            slot.needsWriteback = false;
            return ;
        }


        if (destRegister(slot.di) != 31)
        {
            slot.stalled = false;
            slot.payLoad = raw;
            slot.outPAData = pa;
            slot.needsWriteback = true;

        }
        return ;

    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
     AXP_HOT AXP_ALWAYS_INLINE quint64 calculateEffectiveAddress(const PipelineSlot& slot) noexcept
    {
        // EA = Rb + sign_extend(disp16)
        quint64 base = slot.readIntReg(slot.di.rb);
        qint16 disp = static_cast<qint16>(extractMemDisp(slot.di.rawBits()) & 0xFFFF);
        return base + static_cast<quint64>(disp);
    }

    // ============================================================================
    // INTEGER LOADS (Opcode 0x28-0x2F)
    // ============================================================================

    /**
     * @brief LDL - Load Longword (32-bit sign-extended)
     * Opcode: 0x28
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDL(PipelineSlot& slot) noexcept
    {
        m_isBusy = true;

        // Calculate EA
        slot.va = calculateEffectiveAddress(slot);

        // Check alignment (longword = 4-byte aligned)
        if ((slot.va & 0x3) != 0) {
            slot.faultPending = true;
            slot.trapCode = TrapCode_Class::ALIGNMENT_FAULT;
            slot.faultVA = slot.va;
            m_isBusy = false;
            return;
        }

  
        quint64 pa;
  
        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDL")) {
            // Translation fault already set in slot by helper
            slot.needsWriteback = false;
            m_isBusy = false;
            return ;
        }

        // Read 4 bytes
        quint32 data32;
        MEM_STATUS memStat = m_guestMemory->read32(pa, data32);

        // Sign-extend to 64 bits
        slot.payLoad = static_cast<quint64>(static_cast<qint32>(data32));
        slot.needsWriteback = true;
        slot.pa = pa;
        debugMemory("executeLDL", slot, true, slot.va, slot.payLoad, 8);
        m_isBusy = false;
    }
     // ============================================================================
     // MBox::executeLDQ - Load Quadword (Non-Deferred Model)
     // ============================================================================
     // Opcode: 0x29
     // Format: Memory[Ra.wq, Rb.rq, disp.sl]
     // Operation: Ra <- MEM[Rb + SEXT(disp)]
     // Size: 64-bit (8 bytes)
     // 
     // Faults:
     //   - ALIGNMENT_FAULT: VA not 8-byte aligned
     //   - DTBM_SINGLE: TLB miss on data read
     //   - ACV_FAULT: Access violation (permission denied)
     //   - FAULT_ON_READ: Other translation failures
     // 
     // Pipeline Flow:
     //   1. EX stage: Translate VA -> PA, read memory -> slot.payLoad
     //   2. WB stage: Write slot.payLoad to Ra
     // 
     // Notes:
     //   - Sets slot.needsWriteback = true for WB stage completion
     //   - slot.payLoad holds the loaded value
     //   - slot.pa holds translated physical address
     //   - R31 target writes are discarded by WB stage
     // ============================================================================


    /**
     * @brief LDQ_U - Load Quadword Unaligned
     * Opcode: 0x0B
     */
     /*AXP_HOT AXP_ALWAYS_INLINE void executeLDQ_U(PipelineSlot& slot) noexcept
    {
        m_isBusy = true;

        slot.va = calculateEffectiveAddress(slot);

        // LDQ_U ignores low 3 bits for translation
        quint64 alignedVA = slot.va & ~0x7ULL;

        quint64 pa;
        TranslationResult tr = translate(
            alignedVA,
            getASN_Active(slot.cpuId),
            pa,
            Realm::D,
            MemoryAccessType::READ
        );

        // ===== ADD THIS DEBUG =====
        bool physMode = isPhysicalMode();
        bool palMode = isInPalMode();

        qDebug() << "=== LDQ DEBUG ===";
        qDebug() << "  VA:" << Qt::hex << slot.va;
        qDebug() << "  Physical Mode:" << physMode;
        qDebug() << "  PAL Mode:" << palMode;
        qDebug() << "  Will translate:" << (!physMode && !palMode);
        if (tr != TranslationResult::Success) {
            slot.faultPending = true;
            slot.trapCode = mapDTranslationFault(tr);
            slot.faultVA = alignedVA;
            m_isBusy = false;
            return;
        }

        // Read 8 bytes from aligned address
        m_guestMemory->read64(pa, slot.payLoad);
        slot.needsWriteback = true;
        slot.pa = pa;

        m_isBusy = false;
    }*/

    AXP_HOT AXP_ALWAYS_INLINE void executeLDQ(PipelineSlot& slot) noexcept
    {
        m_isBusy = true;

        // Calculate effective address
        const quint64 base = slot.readIntReg(slot.di.rb);
        const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
        slot.va = base + disp;

        // Alignment check
        if (slot.va & 0x7) {
            debugLog(QString(" LDQ UNALIGNED: 0x%1")
                .arg(slot.va, 16, 16, QChar('0')));
            slot.faultPending = true;
            slot.trapCode = TrapCode_Class::UN_ALIGNED;
            slot.faultVA = slot.va;
            m_isBusy = false;
            return;
        }

        // ================================================================
        // Use centralized translation helper
        // ================================================================
        quint64 pa;
        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDQ")) {
            m_isBusy = false;
            return;
        }

        qDebug() << QString("LDQ: VA 0x%1 -> PA 0x%2")
            .arg(slot.va, 16, 16, QChar('0'))
            .arg(pa, 16, 16, QChar('0'));

        // Physical memory read
        m_guestMemory->read64(pa, slot.payLoad);

        debugMemory("executeLDQ", slot, true, slot.va, slot.payLoad, 8);

        if (slot.di.ra != 31) {
            slot.needsWriteback = true;
            slot.writeRa = true;
        }

        qDebug() << "LDQ: di.ra =" << slot.di.ra;
        qDebug() << "LDQ: About to check if di.ra != 31";

        if (slot.di.ra != 31) {
            qDebug() << "LDQ: Setting needsWriteback and writeRa TRUE";
            slot.needsWriteback = true;
            slot.writeRa = true;
        }
        else {
            qDebug() << "LDQ: Ra is 31, skipping writeback";
        }

        qDebug() << "LDQ: After setting, writeRa =" << slot.writeRa;

        slot.pa = pa;
        m_isBusy = false;
    }
    AXP_HOT AXP_ALWAYS_INLINE void executeLDQ_U(PipelineSlot& slot) noexcept
    {
        m_isBusy = true;

        const quint64 base = slot.readIntReg(slot.di.rb);
        const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
        const quint64 virtualAddr = base + disp;

        // Force alignment
        slot.va = virtualAddr & ~0x7ULL;

        if (virtualAddr != slot.va) {
            debugLog(QString("LDQ_U: 0x%1 aligned to 0x%2")
                .arg(virtualAddr, 16, 16, QChar('0'))
                .arg(slot.va, 16, 16, QChar('0')));
        }

        // ================================================================
        // Use centralized translation helper
        // ================================================================
        quint64 pa;
        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDQ_U")) {
            m_isBusy = false;
            return;
        }

        MEM_STATUS memStat = m_guestMemory->read64(pa, slot.payLoad);

        debugMemory("MEM", slot, true, slot.va, slot.payLoad, 8);

        if (slot.di.ra != 31) {
            slot.needsWriteback = true;
            slot.writeRa = true;
        }

        slot.pa = pa;
        m_isBusy = false;
    }




     // ============================================================================
     // MBox::executeLDA - Load Address (Non-Deferred Model)
     // ============================================================================
     // Opcode: 0x08
     // Format: Memory[Ra.wq, Rb.rq, disp.sl]
     // Operation: Ra <-  Rb + SEXT(disp)
     // 
     // Notes:
     //   - Pure address calculation, no memory access
     //   - No faults possible
     //   - Completes in EX stage with direct register write
     //   - R31 writes are discarded (hardware zero)
     // ============================================================================

    AXP_HOT AXP_ALWAYS_INLINE void executeLDA(PipelineSlot& slot) noexcept
    {
        // LDA Ra, disp(Rb)
        // Ra = Rb + sign_extend(disp)
        // This is COMPUTATIONAL, not a memory operation!

        // Extract 16-bit displacement and sign-extend to 64 bits
        const qint16 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
        const qint64 signExtDisp = static_cast<qint64>(disp);

        // Read base register
        const quint64 rbValue = slot.readIntReg(slot.di.rb);

        // Compute effective address (this is the RESULT, not a memory address!)
        const quint64 result = rbValue + static_cast<quint64>(signExtDisp);

        // ================================================================
        // DEBUG: Integer operation (NOT memory!)
        // ================================================================
        debugInteger("EXEC", slot, rbValue, signExtDisp, result, "LDA");

        // Or if you don't have debugInteger for LDA:
        qDebug() << QString("[EXEC::LDA] PC: 0x%1 | R%2 = R%3 + %4 = 0x%5")
            .arg(slot.di.pc, 16, 16, QChar('0'))
            .arg(slot.di.ra)
            .arg(slot.di.rb)
            .arg(disp)
            .arg(result, 16, 16, QChar('0'));

        // ================================================================
        // SETUP WRITEBACK (pipeline style, not direct write!)
        // ================================================================
        if (slot.di.ra != 31) {
            slot.payLoad = result;
            slot.needsWriteback = true;
            slot.writeRa = true;  // Write to integer register Ra
        }
        else {
            slot.needsWriteback = false;  // R31 writes are no-ops
        }

       
    }
   

     // ============================================================================
    // MBox::executeLDAH - Load Address High (Non-Deferred Model)
    // ============================================================================
    // Opcode: 0x09
    // Format: Memory[Ra.wq, Rb.rq, disp.sl]
    // Operation: Ra <-  Rb + SEXT(disp << 16)
    // 
    // Notes:
    //   - Pure address calculation, no memory access
    //   - Displacement is shifted left 16 bits before sign-extension
    //   - Used with LDA to build 32-bit constants
    //   - No faults possible
    //   - Completes in EX stage with direct register write
    //   - R31 writes are discarded (hardware zero)
    // ============================================================================

     AXP_HOT AXP_ALWAYS_INLINE  void executeLDAH(PipelineSlot& slot) noexcept
     {
         // Extract 16-bit displacement (bits 15:0)
         const qint16 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);

         // Shift left 16 bits THEN sign-extend to 64 bits
         const qint64 shiftedDisp = static_cast<qint64>(disp) << 16;
         const quint64 signExtDisp = static_cast<quint64>(shiftedDisp);

         // Read Rb value
         const quint64 rbValue = slot.readIntReg(slot.di.rb);

         // Compute effective address
         const quint64 result = rbValue + signExtDisp;


         slot.payLoad = result;
         slot.needsWriteback = true;
         slot.writeRa = true;

         DEBUG_LOG(QString("LDAH: R%1 <-  R%2(0x%3) + (0x%4 << 16) = 0x%5")
             .arg(slot.di.ra)
             .arg(slot.di.rb)
             .arg(rbValue, 16, 16, QChar('0'))
             .arg(static_cast<quint16>(disp), 4, 16, QChar('0'))
             .arg(result, 16, 16, QChar('0')));

     }


    /**
     * @brief LDBU - Load Byte Unsigned
     * Opcode: 0x0A
     * TODO check trapcode.
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDBU(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         // ================================================================
         // Calculate effective address
         // ================================================================
         quint64 va = calculateEffectiveAddress(slot);

         // No alignment check for byte loads

         // ================================================================
         // Address translation
         // ================================================================
         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, va, pa, MemoryAccessType::READ, "LDBU")) {
             // Translation fault already set in slot by helper
             slot.needsWriteback = false;
             m_isBusy = false;
             return;
         }

         // ================================================================
         // Physical memory read
         // ================================================================
         quint8 data8;
         MEM_STATUS memStat = m_guestMemory->read8(pa, data8);

         if (memStat != MEM_STATUS::Ok) {
             auto ev = makeMemoryFault(slot.cpuId, va);
             slot.m_faultDispatcher->setPendingEvent(ev);
             slot.va = va;
             slot.faultPending = true;
           

             debugLog(QString("x LDBU MEMORY FAULT at PA 0x%1")
                 .arg(pa, 16, 16, QChar('0')));

             m_isBusy = false;
             return ;
         }

         // ================================================================
         // Zero-extend byte to 64 bits
         // ================================================================
         slot.payLoad = static_cast<quint64>(data8);

         debugMemory("MEM", slot, true, slot.va, data8, 1);
         debugLog(QString("LDBU: R%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 (zero-extended)")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(data8, 2, 16, QChar('0')));

         if (slot.di.ra != 31) {
             slot.needsWriteback = true;
             slot.writeRa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief LDWU - Load Word Unsigned
     * Opcode: 0x0C
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDWU(PipelineSlot& slot)  noexcept
    {
        m_isBusy = true;

        slot.va = calculateEffectiveAddress(slot);

        // Check alignment (word = 2-byte aligned)
        if ((slot.va & 0x1) != 0) {
            slot.faultPending = true;
            slot.trapCode = TrapCode_Class::ALIGNMENT_FAULT;
            slot.faultVA = slot.va;
            m_isBusy = false;
            return;
        }

        quint64 pa;
        if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDWU")) {
            m_isBusy = false;
            return;
        }

        
        // Read 2 bytes, zero-extend
        quint16 data16;
        m_guestMemory->read16(pa, data16);
        slot.payLoad = static_cast<quint64>(data16);
        slot.needsWriteback = true;
        slot.pa = pa;
        debugMemory("executeLDWU", slot, true, slot.va, data16, 2);
        m_isBusy = false;
    }

    // ============================================================================
    // INTEGER STORES (Opcode 0x2C-0x2F)
    // ============================================================================

    /**
     * @brief STL - Store Longword (32-bit)
     * Opcode: 0x2C
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTL(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // Alignment check (4-byte boundary)
         if (slot.va & 0x3) {
             debugLog(QString(" STL UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         const quint64 value64 = slot.readIntReg(slot.di.ra);
         const quint32 value32 = static_cast<quint32>(value64);  // Truncate to 32 bits

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE,  "STL")) {
             m_isBusy = false;
             return;
         }

       MEM_STATUS memStat =   m_guestMemory->write32(pa, value32);

         debugMemory("executeSTL", slot, false, slot.va, value32, 4);

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

     // ============================================================================
     // MBox::executeSTQ - Store Quadword (Non-Deferred Model)
     // ============================================================================
     // Opcode: 0x2D
     // Format: Memory[Ra.rq, Rb.rq, disp.sl]
     // Operation: MEM[Rb + SEXT(disp)] <- Ra
     // Size: 64-bit (8 bytes)
     // 
     // Faults:
     //   - ALIGNMENT_FAULT: VA not 8-byte aligned
     //   - DTBM_SINGLE: TLB miss on data write
     //   - ACV_FAULT: Access violation (permission denied, read-only page)
     //   - FAULT_ON_WRITE: Other translation failures
     // 
     // Pipeline Flow:
     //   1. EX stage: Translate VA -> PA, write Ra to memory
     //   2. Completes in EX - no writeback stage needed
     // 
     // Cache Coherency:
     //   - Breaks all LDx_L/STx_C reservations on this cache line
     //   - Ensures sequentially consistent memory ordering
     // 
     // Notes:
     //   - R31 stores write architectural zero (0x0000_0000_0000_0000)
     //   - slot.pa holds translated physical address for debugging
     //   - Write completes immediately (no buffering in this model)
     // ============================================================================

     AXP_HOT AXP_ALWAYS_INLINE void executeSTQ(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         // ================================================================
         // STAGE 1: Calculate effective address
         // ================================================================
         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (STQ requires 8-byte alignment)
         // ================================================================
         if (slot.va & 0x7) {
             debugLog(QString(" STQ UNALIGNED ACCESS: VA 0x%1 (alignment fault)")
                 .arg(slot.va, 16, 16, QChar('0')));

             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         // ================================================================
         // STAGE 2: Read value from Ra register
         // ================================================================
         const quint64 value = slot.readIntReg(slot.di.ra);

         // ================================================================
         // STAGE 3: Address translation (using centralized helper)
         // ================================================================
         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STQ")) {
             m_isBusy = false;
             return;  // Translation fault already set in slot
         }

         // ================================================================
         // STAGE 4: Physical memory write
         // ================================================================
     MEM_STATUS mStatus=    m_guestMemory->write64(pa, value);

         // ================================================================
         // DEBUG: Show memory access
         // ================================================================
         debugMemory("executeSTQ_U", slot, false, slot.va, value, 8);

         debugLog(QString("STQ: [VA 0x%1 / PA 0x%2] <- R%3 = 0x%4")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value, 16, 16, QChar('0')));

         // ================================================================
         // STAGE 5: No writeback needed (stores don't write registers)
         // ================================================================
         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief STQ_U - Store Quadword Unaligned
     * Opcode: 0x0F
     */

     AXP_HOT AXP_ALWAYS_INLINE void executeSTQ_U(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         // ================================================================
         // STAGE 1: Calculate effective address
         // ================================================================
         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         const quint64 virtualAddr = base + disp;

         // STQ_U forces alignment by clearing low 3 bits
         slot.va = virtualAddr & ~0x7ULL;

         // ================================================================
         // DEBUG: Show unaligned access details
         // ================================================================
         if (virtualAddr != slot.va) {
             debugLog(QString("STQ_U unaligned: requested=0x%1, aligned=0x%2, offset=%3")
                 .arg(virtualAddr, 16, 16, QChar('0'))
                 .arg(slot.va, 16, 16, QChar('0'))
                 .arg(virtualAddr & 0x7));
         }

         // ================================================================
         // STAGE 2: Read value from Ra register
         // ================================================================
         const quint64 value = slot.readIntReg(slot.di.ra);

         // ================================================================
         // STAGE 3: Address translation (using centralized helper)
         // ================================================================
         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa,MemoryAccessType::WRITE, "STQ_U")) {
             m_isBusy = false;
             return;
         }

         // ================================================================
         // STAGE 4: Physical memory write
         // ================================================================
       MEM_STATUS msmStat =  m_guestMemory->write64(pa, value);

         // ================================================================
         // DEBUG: Show memory access
         // ================================================================
         debugMemory("executeSTQ_U", slot, false, slot.va, value, 8);

         debugLog(QString("STQ_U: [VA 0x%1 / PA 0x%2] <- R%3 = 0x%4 (requested: 0x%5)")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value, 16, 16, QChar('0'))
             .arg(virtualAddr, 16, 16, QChar('0')));

         // ================================================================
         // STAGE 5: No writeback needed
         // ================================================================
         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }
    /**
     * @brief STB - Store Byte
     * Opcode: 0x0E
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTB(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // No alignment check for byte stores

         const quint64 value64 = slot.readIntReg(slot.di.ra);
         const quint8 value8 = static_cast<quint8>(value64);  // Truncate to 8 bits

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STB")) {
             m_isBusy = false;
             return;
         }

       MEM_STATUS memStat =   m_guestMemory->write8(pa, value8);

         debugMemory("executeSTB", slot, false, slot.va, value8, 1);

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }
  

    /**
     * @brief STW - Store Word
     * Opcode: 0x0D
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTW(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // Alignment check (2-byte boundary)
         if (slot.va & 0x1) {
             debugLog(QString(" STW UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         const quint64 value64 = slot.readIntReg(slot.di.ra);
         const quint16 value16 = static_cast<quint16>(value64);  // Truncate to 16 bits

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa,MemoryAccessType::WRITE,  "STW")) {
             m_isBusy = false;
             return;
         }

         MEM_STATUS memStat = m_guestMemory->write16(pa, value16);

         debugMemory("executeSTW", slot, false, slot.va, value16, 2);

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief LDG - Load G-format (64-bit VAX double)
     * Opcode: 0x21
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDG(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         // ================================================================
         // STAGE 1: Calculate effective address
         // ================================================================
         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (8-byte boundary for G_floating)
         // ================================================================
         if (slot.va & 0x7) {
             debugLog(QString(" LDG UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         // ================================================================
         // STAGE 2: Address translation
         // ================================================================
         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa,MemoryAccessType::READ, "LDG")) {
             m_isBusy = false;
             return;
         }

         // ================================================================
         // STAGE 3: Physical memory read
         // ================================================================
         MEM_STATUS memStat = m_guestMemory->read64(pa, slot.payLoad);

         debugMemory("executeLDG", slot, true, slot.va, slot.payLoad, 8);
         debugLog(QString("LDG: F%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 (G_floating)")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.payLoad, 16, 16, QChar('0')));

         // ================================================================
         // STAGE 4: Writeback to floating-point register
         // ================================================================
         if (slot.di.ra != 31) {
             slot.needsWriteback = true;
             slot.writeFa = true;  // Write to float register, not Ra
         }

         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief LDS - Load S-format (32-bit IEEE float)
     * Opcode: 0x22
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDS(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (4-byte boundary for S_floating)
         // ================================================================
         if (slot.va & 0x3) {
             debugLog(QString(" LDS UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa,MemoryAccessType::READ, "LDS")) {
             m_isBusy = false;
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::TRANSLATION_FAULT;
             slot.faultVA = slot.va;
             return;
         }

         // Read 32-bit IEEE single precision
         quint32 value32;
         MEM_STATUS memStat = m_guestMemory->read32(pa, value32);

         // Convert to 64-bit representation (T_floating format in register)
         // This is a format conversion from S to T floating
         slot.payLoad = convertSFloatToTFloat(value32);

         debugMemory("executeLDS", slot, true, slot.va, value32, 4);
         debugLog(QString("LDS: F%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 -> 0x%5 (S->T)")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(value32, 8, 16, QChar('0'))
             .arg(slot.payLoad, 16, 16, QChar('0')));

         if (slot.di.ra != 31) {
             slot.needsWriteback = true;
             slot.writeFa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief LDT - Load T-format (64-bit IEEE double)
     * Opcode: 0x23
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDT(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (8-byte boundary for T_floating)
         // ================================================================
         if (slot.va & 0x7) {
             debugLog(QString("LDT UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa,MemoryAccessType::READ,  "LDT")) {
             m_isBusy = false;
             return;
         }

         MEM_STATUS memStat = m_guestMemory->read64(pa, slot.payLoad);

         debugMemory("executeLDT", slot, true, slot.va, slot.payLoad, 8);
         debugLog(QString("LDT: F%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 (T_floating)")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.payLoad, 16, 16, QChar('0')));

         if (slot.di.ra != 31) {
             slot.needsWriteback = true;
             slot.writeFa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

    // ============================================================================
    // FLOATING-POINT STORES (Opcode 0x24-0x27)
    // ============================================================================

    /**
     * @brief STF - Store F-format
     * Opcode: 0x24
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTF(PipelineSlot& slot) noexcept
     {
         // F_floating is obsolete VAX format, rarely used
         // Treat as 32-bit store for compatibility
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         if (slot.va & 0x3) {
             debugLog(QString(" STF UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         const quint64 value64 = slot.readFpReg(slot.di.ra);
         const quint32 value32 = static_cast<quint32>(value64);  // Truncate

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STF")) {
             m_isBusy = false;
             return;
         }

       MEM_STATUS mStatus =   m_guestMemory->write32(pa, value32);

         debugMemory("executeSTF", slot, false, slot.va, value32, 4);
         debugLog(QString("STF: [VA 0x%1 / PA 0x%2] <- F%3 = 0x%4 (F_floating/obsolete)")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value32, 8, 16, QChar('0')));

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief STG - Store G-format
     * Opcode: 0x25
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTG(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK
         // ================================================================
         if (slot.va & 0x7) {
             debugLog(QString("STG UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         // Read from floating-point register
         const quint64 value = slot.readFpReg(slot.di.ra);

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE,  "STG")) {
             m_isBusy = false;
             return;
         }

         MEM_STATUS mStatus = m_guestMemory->write64(pa, value);

         debugMemory("executeSTG", slot, false, slot.va, value, 8);
         debugLog(QString("STG: [VA 0x%1 / PA 0x%2] <- F%3 = 0x%4 (G_floating)")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value, 16, 16, QChar('0')));

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief STS - Store S-format
     * Opcode: 0x26
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTS(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         if (slot.va & 0x3) {
             debugLog(QString("STS UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         // Read T_floating from register and convert to S_floating
         const quint64 value64 = slot.readFpReg(slot.di.ra);
         const quint32 value32 = convertTFloatToSFloat(value64);

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STS")) {
             m_isBusy = false;
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::TRANSLATION_FAULT;
             slot.faultVA = slot.va;
             return;
         }

         MEM_STATUS mStatus = m_guestMemory->write32(pa, value32);

         debugMemory("executeSTS", slot, false, slot.va, value32, 4);
         debugLog(QString("STS: [VA 0x%1 / PA 0x%2] <- F%3 = 0x%4 (T->S)")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value32, 8, 16, QChar('0')));

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief STT - Store T-format
     * Opcode: 0x27
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTT(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         if (slot.va & 0x7) {
             debugLog(QString(" STT UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         const quint64 value = slot.readFpReg(slot.di.ra);

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STT")) {
             m_isBusy = false;
             return;
         }

         MEM_STATUS mStatus = m_guestMemory->write64(pa, value);

         debugMemory("executeSTT", slot, false, slot.va, value, 8);
         debugLog(QString("STT: [VA 0x%1 / PA 0x%2] <- F%3 = 0x%4 (T_floating)")
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(slot.di.ra)
             .arg(value, 16, 16, QChar('0')));

         slot.needsWriteback = false;
         slot.pa = pa;
         m_isBusy = false;
     }

    // ============================================================================
    // ATOMIC OPERATIONS (Opcode 0x2A-0x2B)
    // ============================================================================

    /**
     * @brief LDL_L - Load Longword Locked
     * Opcode: 0x2A
     *
     * NOTE: Lock tracking requires CBox coordination (TODO)
     */
     /**
      * @brief LDL_L - Load Longword Locked
      */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDL_L(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (4-byte boundary)
         // ================================================================
         if (slot.va & 0x3) {
             debugLog(QString("x LDL_L UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         // ================================================================
         // Address translation
         // ================================================================
         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDL_L")) {
             m_isBusy = false;
             return;
         }

         // ================================================================
         // Physical memory read
         // ================================================================
         quint32 value32;
         MEM_STATUS mStatus = m_guestMemory->read32(pa, value32);

         // Sign-extend to 64 bits
         const quint64 value64 = static_cast<qint32>(value32);

         // ================================================================
         // SET RESERVATION using global ReservationManager
         // ================================================================
         globalReservationManager().setReservation(slot.cpuId, pa);

         debugMemory("MEM", slot, true, slot.va, value32, 4);
         debugLog(QString("LDL_L: R%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 | LOCK SET on cache line 0x%5")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(value32, 8, 16, QChar('0'))
             .arg(pa & ReservationManager::CACHE_LINE_MASK, 16, 16, QChar('0')));

         // ================================================================
         // Writeback
         // ================================================================
         if (slot.di.ra != 31) {
             slot.payLoad = value64;
             slot.needsWriteback = true;
             slot.writeRa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }


    /**
     * @brief LDQ_L - Load Quadword Locked
     * Opcode: 0x2B
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeLDQ_L(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         // ================================================================
         // ALIGNMENT CHECK (8-byte boundary)
         // ================================================================
         if (slot.va & 0x7) {
             debugLog(QString("x LDQ_L UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::READ, "LDQ_L")) {
             m_isBusy = false;
             return;
         }

         quint64 value;
         MEM_STATUS mStatus = m_guestMemory->read64(pa, value);

         // ================================================================
         // SET RESERVATION
         // ================================================================
         globalReservationManager().setReservation(slot.cpuId, pa);

         debugMemory("MEM", slot, true, slot.va, value, 8);
         debugLog(QString("LDQ_L: R%1 <- [VA 0x%2 / PA 0x%3] = 0x%4 | LOCK SET on cache line 0x%5")
             .arg(slot.di.ra)
             .arg(slot.va, 16, 16, QChar('0'))
             .arg(pa, 16, 16, QChar('0'))
             .arg(value, 16, 16, QChar('0'))
             .arg(pa & ReservationManager::CACHE_LINE_MASK, 16, 16, QChar('0')));

         if (slot.di.ra != 31) {
             slot.payLoad = value;
             slot.needsWriteback = true;
             slot.writeRa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

     AXP_HOT AXP_ALWAYS_INLINE void executeSTL_C(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         if (slot.va & 0x3) {
             debugLog(QString("x STL_C UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STL_C")) {
             m_isBusy = false;
             return;
         }

         // ================================================================
         // CHECK AND CLEAR RESERVATION
         // This is atomic - checks validity and clears in one operation
         // ================================================================
         const bool reservationValid = globalReservationManager().checkAndClearReservation(slot.cpuId, pa);

         quint64 resultValue;

         if (reservationValid) {
             // Reservation was valid - perform the store
             const quint64 value64 = slot.readIntReg(slot.di.ra);
             const quint32 value32 = static_cast<quint32>(value64);

             MEM_STATUS mStatus = m_guestMemory->write32(pa, value32);

             debugMemory("MEM", slot, false, slot.va, value32, 4);
             debugLog(QString("STL_C: [VA 0x%1 / PA 0x%2] <- R%3 = 0x%4 | v/ SUCCESS (lock was valid)")
                 .arg(slot.va, 16, 16, QChar('0'))
                 .arg(pa, 16, 16, QChar('0'))
                 .arg(slot.di.ra)
                 .arg(value32, 8, 16, QChar('0')));

             // ================================================================
             // IMPORTANT: Break other CPUs' reservations on this cache line
             // This maintains cache coherency
             // ================================================================
             globalReservationManager().breakReservationsOnCacheLine(pa);

             resultValue = 1;  // Success
         }
         else {
             debugLog(QString("STL_C: [VA 0x%1 / PA 0x%2] | x FAILED (no valid lock)")
                 .arg(slot.va, 16, 16, QChar('0'))
                 .arg(pa, 16, 16, QChar('0')));

             resultValue = 0;  // Failure
         }

         // ================================================================
         // WRITE SUCCESS/FAILURE TO Ra
         // This is how software knows if the store succeeded
         // ================================================================
         if (slot.di.ra != 31) {
             slot.payLoad = resultValue;
             slot.needsWriteback = true;
             slot.writeRa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

    /**
     * @brief STQ_C - Store Quadword Conditional
     * Opcode: 0x2F
     * TODO verify reservation locking
     */
     AXP_HOT AXP_ALWAYS_INLINE void executeSTQ_C(PipelineSlot& slot) noexcept
     {
         m_isBusy = true;

         const quint64 base = slot.readIntReg(slot.di.rb);
         const qint32 disp = static_cast<qint16>(slot.di.rawBits() & 0xFFFF);
         slot.va = base + disp;

         if (slot.va & 0x7) {
             debugLog(QString("x STQ_C UNALIGNED: 0x%1").arg(slot.va, 16, 16, QChar('0')));
             slot.faultPending = true;
             slot.trapCode = TrapCode_Class::UN_ALIGNED;
             slot.faultVA = slot.va;
             m_isBusy = false;
             return;
         }

         quint64 pa;
         if (!m_ev6Translator->translateLoadAddress(slot, slot.va, pa, MemoryAccessType::WRITE, "STQ_C")) {
             m_isBusy = false;
             return;
         }

         // ================================================================
         // CHECK AND CLEAR RESERVATION
         // ================================================================
         const bool reservationValid = m_reservationManager->checkAndClearReservation(slot.cpuId, pa);

         quint64 resultValue;

         if (reservationValid) {
             const quint64 value = slot.readIntReg(slot.di.ra);

             MEM_STATUS mStatus = m_guestMemory->write64(pa, value);

             debugMemory("MEM", slot, false, slot.va, value, 8);
             debugLog(QString("STQ_C: [VA 0x%1 / PA 0x%2] <- R%3 = 0x%4 | v/ SUCCESS")
                 .arg(slot.va, 16, 16, QChar('0'))
                 .arg(pa, 16, 16, QChar('0'))
                 .arg(slot.di.ra)
                 .arg(value, 16, 16, QChar('0')));

             // Break other reservations on this cache line
             m_reservationManager->breakReservationsOnCacheLine(pa);

             resultValue = 1;  // Success
         }
         else {
             debugLog(QString("STQ_C: [VA 0x%1 / PA 0x%2] | x FAILED")
                 .arg(slot.va, 16, 16, QChar('0'))
                 .arg(pa, 16, 16, QChar('0')));

             resultValue = 0;  // Failure
         }

         if (slot.di.ra != 31) {
             slot.payLoad = resultValue;
             slot.needsWriteback = true;
             slot.writeRa = true;
         }

         slot.pa = pa;
         m_isBusy = false;
     }

   



private:
    // ========================================================================
    // Private Data - PAL IPR Staging
    // ========================================================================

    /**
     * \brief 
     */
    struct IPRStagedEntry {
        quint64 pte{ 0 };
        quint64 tag{ 0 };
        bool hasPTE{ false };
        bool hasTag{ false };

        void clear() noexcept {
            pte = 0;
            tag = 0;
            hasPTE = false;
            hasTag = false;
        }

        bool isComplete() const noexcept {
            return hasPTE && hasTag;
        }
    };

    IPRStagedEntry m_iprStagedITB;
    IPRStagedEntry m_iprStagedDTB;

    // ========================================================================
    // Private Data - TLB Miss Handler Staging
    // ========================================================================

    stagePTECache m_missStaging;

    // ========================================================================
    // Private Data - General
    // ========================================================================

    CPUIdType m_cpuId;
    GuestMemory* m_guestMemory {nullptr};
    FaultDispatcher* m_faultSink{ nullptr };
    QScopedPointer<Ev6Translator> m_ev6Translator{ nullptr };
    CPUStateView  m_cpuView;                            // value member
    CPUStateView* m_iprGlobalMaster{ &m_cpuView };
    ReservationManager* m_reservationManager;


    bool m_isInPalMode{ false };
    bool m_loadPending{ false };
    bool m_storePending{ false };
    bool m_isBusy{ false };



    // ========================================================================
    // Private Helpers - Translation
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE TranslationResult handleTLBMiss(
        VAType va,
        ASNType asn,
        Realm realm) const noexcept
    {
        DEBUG_LOG(QString("CPU %1: TLB miss - Realm=%2 VA=0x%3 ASN=%4")
            .arg(m_cpuId)
            .arg(realm == Realm::I ? "I" : "D")
            .arg(va, 16, 16, QChar('0'))
            .arg(asn));

        // TODO: Implement page table walk
        // For now, return TLB_MISS to trigger PAL handler
        return (realm == Realm::I)
            ? TranslationResult::IlbMiss
            : TranslationResult::DlbMiss;
    }

    AXP_HOT AXP_ALWAYS_INLINE TranslationResult validatePermissions(
        const AlphaPTE& pte,
        MemoryAccessType accessType,
        Realm realm)  noexcept
    {
        // Check valid bit
        if (!pte.bitV()) {
            return TranslationResult::INVALID_PTE;
        }

        // Get current mode (user vs kernel)
        CMType currentMode = m_iprGlobalMaster->h->cm;
        bool isKernelMode = (currentMode == 0);

        // Check permissions based on access type
        switch (accessType) {
        case MemoryAccessType::READ:
            if (isKernelMode) {
                if (!pte.bitKRE()) {
                    return TranslationResult::FaultOnRead;
                }
            }
            else {
                if (!pte.bitURE()) {
                    return TranslationResult::FaultOnRead;
                }
            }
            break;

        case MemoryAccessType::WRITE:
            // Check FOW (Fault on Write)
            if (pte.bitFOW()) {
                return TranslationResult::FaultOnWrite;
            }

            if (isKernelMode) {
                if (!pte.bitKWE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            else {
                if (!pte.bitUWE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            break;

        case MemoryAccessType::EXECUTE:
            // ITB only - check FOE (Fault on Execute)
            if (realm == Realm::I && pte.bitFOE()) {
                return TranslationResult::FaultOnExecute;
            }

            // Execute permission is read permission for Alpha
            if (isKernelMode) {
                if (!pte.bitKRE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            else {
                if (!pte.bitURE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            break;
        }

        return TranslationResult::Success;
    }
    // ========================================================================
// Overload 1: Accepts pointer (for tlbLookup results)
// ========================================================================
    AXP_HOT AXP_ALWAYS_INLINE TranslationResult validatePermissions(
        const AlphaPTE* pte,  //  Pointer version
        MemoryAccessType accessType,
        Realm realm) const noexcept
    {
        // Null check
        if (!pte) {
            return TranslationResult::INVALID_PTE;
        }

        // Delegate to reference version
        return validatePermissions(*pte, accessType, realm);
    }

    // ========================================================================
    // Overload 2: Accepts reference (your existing implementation)
    // ========================================================================
    AXP_HOT AXP_ALWAYS_INLINE TranslationResult validatePermissions(
        const AlphaPTE& pte,  //  Reference version
        MemoryAccessType accessType,
        Realm realm) const noexcept
    {
        // Check valid bit
        if (!pte.bitV()) {
            return TranslationResult::INVALID_PTE;
        }

        // Get current mode (user vs kernel)
        CMType currentMode = m_iprGlobalMaster->h->cm;
        bool isKernelMode = (currentMode == 0);

        // Check permissions based on access type
        switch (accessType) {
        case MemoryAccessType::READ:
            if (isKernelMode) {
                if (!pte.bitKRE()) {
                    return TranslationResult::FaultOnRead;
                }
            }
            else {
                if (!pte.bitURE()) {
                    return TranslationResult::FaultOnRead;
                }
            }
            break;

        case MemoryAccessType::WRITE:
            // Check FOW (Fault on Write)
            if (pte.bitFOW()) {
                return TranslationResult::FaultOnWrite;
            }

            if (isKernelMode) {
                if (!pte.bitKWE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            else {
                if (!pte.bitUWE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            break;

        case MemoryAccessType::EXECUTE:
            // ITB only - check FOE (Fault on Execute)
            if (realm == Realm::I && pte.bitFOE()) {
                return TranslationResult::FaultOnExecute;
            }

            // Execute permission is read permission for Alpha
            if (isKernelMode) {
                if (!pte.bitKRE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            else {
                if (!pte.bitURE()) {
                    return TranslationResult::AccessViolation;
                }
            }
            break;
        }

        return TranslationResult::Success;
    }
};

#endif // MBOX_HEADERONLY_H
