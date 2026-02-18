// ============================================================================
// PalBoxBase.h - PAL Box (Header-Only Implementation)
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   PAL (Privileged Architecture Library) execution unit for Alpha processor.
//   Handles CALL_PAL instructions, HW_MFPR/HW_MTPR register access, PAL mode
//   entry/exit, shadow register management, and system service dispatch.
//   Header-only implementation with all methods inline.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef PALBOXBASE_H
#define PALBOXBASE_H

#include <memory>
#include <QString>


#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/LoggingMacros.h"
#include "palLib_EV6/PAL_core.h"
#include "coreLib/PS_helpers_inl.h"
#include "coreLib/BoxRequest.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "cpuCoreLib/cpuCore_core.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "palLib_EV6/PalVectorTable_final.h"
#include "palLib_EV6/Pal_core_inl.h"
#include "palLib_EV6/global_PalVectorTable.h"
#include "palLib_EV6/Pal_Service.h"
#include "memoryLib/global_GuestMemory.h"
#include "pteLib/ev6Translation_struct.h"

class PalService;
// Forward declarations
class PipelineSlot;
struct BoxResult;
struct PalResult;

// ============================================================================
// PalBox - PAL Execution Unit (Header-Only)
// ============================================================================

/**
 * @brief PAL execution unit for Alpha processor
 *
 * Responsibilities:
 * - CALL_PAL instruction execution
 * - HW_MFPR/HW_MTPR processor register access
 * - PAL shadow register bank management
 * - AST (Asynchronous System Trap) handling
 * - Privilege level enforcement
 * - System service dispatch
 * - Context save/restore for PAL mode transitions
 *
 * Design:
 * - Singleton per CPU (Meyers singleton pattern)
 * - Header-only implementation (all methods inline)
 * - Owns PalService instance via std::unique_ptr
 */
class PalBox final
{
    CPUIdType                   m_cpuId;
    std::unique_ptr<PalService> m_palService;

    quint64 m_entryVector{0};
    quint64 m_faultPC{0};
    bool    m_shadowRegsActive{false};

    // IRQ Controllers
    IRQPendingState* m_pending;
    InterruptRouter* m_router;
    // ==================================

    PalEntryReason m_entryReason;

    GuestMemory*                  m_guestMemory{nullptr};
    FaultDispatcher*              m_faultDispatcher{nullptr};
    QScopedPointer<Ev6Translator> m_ev6Translator{nullptr};
 
    CPUStateView*                 m_iprGlobalMaster{nullptr};
    // IPRStorage_IntRegs*   m_intReg{nullptr};
    // IPRStorage_FloatRegs* m_fpReg{nullptr};

public:
    // ====================================================================
    // BoxResult -> PipelineSlot Unpacker
    // ====================================================================
    // Grains return void; execution boxes return BoxResult. This helper
    // maps BoxResult flags and fault info into the slot's individual
    // fields so the pipeline can consume them after grain::execute().
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE static auto applyBoxResult(PipelineSlot& slot, const BoxResult& br) noexcept -> void
    {
        if (br.flags & BOX_FLUSH_PIPELINE)
            slot.flushPipeline = true;

        if (br.flags & BOX_ENTER_PALMODE)
            slot.enterPalMode = true;

        if (br.pcModified)
            slot.pcModified = true;

        if (br.hasFault())
        {
            slot.faultPending = true;
            slot.trapCode     = br.faultClass;
            slot.faultVA      = br.faultingVA;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeAMOVRM(PipelineSlot& slot) -> void
    {
        m_palService->executeAMOVRM(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeAMOVRR(PipelineSlot& slot) -> void
    {
        m_palService->executeAMOVRR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeBPT(PipelineSlot& slot) -> void
    {
        m_palService->executeBPT(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeBUGCHK(PipelineSlot& slot) -> void
    {
        m_palService->executeBUGCHK(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCALL_PAL(PipelineSlot& slot) noexcept
    {
        const quint8 palFunction = static_cast<quint8>(slot.di.rawBits() & 0xFF);

#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_PAL_DISPATCH(m_cpuId,                      //  INSERT
                               palFunction,
                               slot.di.pc,
                               palFunctionName(palFunction));
#endif

        m_palService->execute(
            static_cast<PalCallPalFunction>(palFunction), slot, slot.palResult);
        commitPalResult(slot);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCALLKD(PipelineSlot& slot) -> void
    {
        m_palService->executeCALLKD(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCALLSYS(PipelineSlot& slot) -> void
    {
        m_palService->executeCALLSYS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCFLUSH(PipelineSlot& slot) -> void
    {
        m_palService->executeCFLUSH(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCHME(PipelineSlot& slot) -> void
    {
        m_palService->executeCHME(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCHMK(PipelineSlot& slot) -> void
    {
        m_palService->executeCHMK(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCHMS(PipelineSlot& slot) -> void
    {
        m_palService->executeCHMS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQHILR(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQHILR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINITPAL(PipelineSlot& slot) -> void// missing in PalService
    {
        m_palService->executeINITPAL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeHALT(PipelineSlot& slot) -> void
    {
        m_palService->executeHALT(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeIMB(PipelineSlot& slot) -> void
    {
        m_palService->executeIMB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCHMU(PipelineSlot& slot) -> void
    {
        m_palService->executeCHMU(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCSERVE(PipelineSlot& slot) -> void
    {
        m_palService->executeCSERVE(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeCSIR(PipelineSlot& slot) -> void// missing in PalService
    {
        m_palService->executeCSIR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeDI(PipelineSlot& slot) -> void// missing in PalService
    {
        m_palService->executeDI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeDRAINA(PipelineSlot& slot) -> void
    {
        m_palService->executeDRAINA(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeDTBIS(PipelineSlot& slot) -> void// missing in PalService
    {
        m_palService->executeDTBIS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeEI(PipelineSlot& slot) -> void// missing in PalService
    {
        m_palService->executeEI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeGENTRAP(PipelineSlot& slot) -> void
    {
        m_palService->executeGENTRAP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQHIL(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQHIL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQHIQ(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQHIQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQHIQR(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQHIQR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQTIL(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQTIL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQTILR(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQTILR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQTIQ(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQTIQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQTIQR(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQTIQR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQUEL(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQUEL(slot, slot.palResult);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQUEQ(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQUEQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQUEQ_D(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQUEQ_D(slot, slot.palResult);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeKBPT(PipelineSlot& slot) -> void // missing in PalService
    {
        m_palService->executeKBPT(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeLDQP(PipelineSlot& slot) -> void
    {
        m_palService->executeLDQP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_ASN(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_ASN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_ASTEN(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_ASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_ASTSR(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_ASTSR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_ESP(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_ESP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_FEN(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_FEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_IPL(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_IPL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_MCES(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_MCES(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_PCBB(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_PCBB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_PRBR(PipelineSlot& slot) -> void
    {
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_PTBR(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_PTBR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_SISR(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_SISR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREAD_UNQ(PipelineSlot& slot) -> void
    {
        m_palService->executeREAD_UNQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQTIQR(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTIQR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQTIQ(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTIQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQTILR(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTILR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQTIL(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTIL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQHIQ(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTIQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQHILR(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQTILR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQUEUD(PipelineSlot& slot) -> void
    {
        m_palService->executeINSQUEL_D(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_SSP(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_SSP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_TBCHK(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_TBCHK(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_USP(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_USP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_VPTB(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_VPTB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMFPR_WHAMI(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_WHAMI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_ASTEN(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_ASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    // AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_ASTSR(PipelineSlot& slot) -> void
    // {
    //     m_palService->executeMTPR_ASTSR(slot, slot.palResult);
    // }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_DATFX(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_DATFX(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_ESP(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_ESP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_FEN(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_FEN(slot, slot.palResult);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_IPIR(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_IPIR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_IPL(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_IPL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_MCES(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_MCES(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_PERFMON(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_PERFMON(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_PRBR(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_PRBR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_SCBB(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_SCBB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    // AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_SIRR(PipelineSlot& slot) -> void
    // {
    //     m_palService->executeMTPR_SIRR(slot, slot.palResult);
    // }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_SSP(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_SSP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_TBIA(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBIA(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_TBIAP(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBIAP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_TBIS(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBIS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_TBISD(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBISD(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_TBISI(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBISI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_USP(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_TBISI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeMTPR_VPTB(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_VPTB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePROBER(PipelineSlot& slot) -> void
    {
        m_palService->executePROBER(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePROBEW(PipelineSlot& slot) -> void
    {
        m_palService->executePROBEW(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDCOUNTERS(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDCOUNTERS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDIRQL(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDIRQL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDMCES(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDMCES(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPCBB(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDPCBB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPER(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDPER(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPS(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDPS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPSR(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDPSR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDTEB(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDTEB(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDTHREAD(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDTHREAD(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDUNIQUE(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDUNIQUE(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDUSP(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDUSP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDVAL(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRDVAL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRD_PS(PipelineSlot& slot) -> void
    {
        m_palService->executeRD_PS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREBOOT(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeREBOOT(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQHIL(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQHIL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQUEL(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQUEL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQUEQ(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQUEQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQUEQ_D(PipelineSlot& slot) -> void
    {
        m_palService->executeREMQUEQ_D(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREMQUE_UD(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeREMQUE_UD(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRESTART(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRESTART(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRETSYS(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRETSYS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRFE(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRFE(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRSCC(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRSCC(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRTI(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeRTI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWR_PS_SW(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWR_PS_SW(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRVPTPTR(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRVPTPTR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRVAL(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRVAL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRUSP(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRUSP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRUNIQUE(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRUNIQUE(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRPRBR(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRPRBR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRPERFMON(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRPERFMON(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRMCES(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRMCES(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRKGP(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRKGP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRITE_UNQ(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRITE_UNQ(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRIPIR(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRIPIR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRFEN(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRFEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRENT(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWRENT(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWHAMI(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeWHAMI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTHIS(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeTHIS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTBISASN(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeTBISASN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTBIS(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeTBIS(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTBIA(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeTBIA(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTBI(PipelineSlot& slot) -> void   // missing in PalService
    {
        m_palService->executeTBI(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPPROCESS(PipelineSlot& slot) -> void
    {
        m_palService->executeSWPPROCESS(slot, slot.palResult);   // missing in PalService
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPPAL(PipelineSlot& slot) -> void
    {
        m_palService->executeSWPPAL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPKSP(PipelineSlot& slot) -> void    // missing in PalService
    {
        m_palService->executeSWPKSP(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPIRQL(PipelineSlot& slot) -> void // missing in PalService
    {
        m_palService->executeSWPIRQL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPIPL(PipelineSlot& slot) -> void    // missing in PalService
    {
        m_palService->executeSWPIPL(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWPCTX(PipelineSlot& slot) -> void
    {
        m_palService->executeSWPCTX(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSWASTEN(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSTQP(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSSIR(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeINSQUEL_D(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    /*AXP_HOT AXP_ALWAYS_INLINE auto executeOPC01(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC02(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }*/

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDKSP_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDMCES_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPCBB_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPSR_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDTHREAD_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDUSP_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeREBOOT_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRETSYS_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeSSIR_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRUSP_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRPRBR_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeWRMCES_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeTHIS_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeRDPER_64(PipelineSlot& slot) -> void
    {
        m_palService->executeSWASTEN(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePAL1F(PipelineSlot& slot) -> void
    {
        // HW_ST - Hardware Store (opcode 0x1F, PALmode only)
        applyBoxResult(slot, executeHW_ST(slot));
        commitPalResult(slot);                    // Commit Pal Results
    }

    // AXP_HOT AXP_ALWAYS_INLINE auto executeOPC03(PipelineSlot& slot) -> void
    // {
    //     m_palService->executeSWASTEN(slot, slot.palResult);
    // }
    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC01(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC02(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC03(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC04(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC05(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC06(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executeOPC07(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePAL19(PipelineSlot& slot) -> void
    {
        // HW_MFPR - Move From Processor Register (opcode 0x19, PALmode only)
        applyBoxResult(slot, executeHW_MFPR(slot));
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePAL1B(PipelineSlot& slot) -> void
    {
        // HW_LD - Hardware Load (opcode 0x1B, PALmode only)
        applyBoxResult(slot, executeHW_LD(slot));
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePAL1D(PipelineSlot& slot) -> void
    {
        // HW_MTPR - Move To Processor Register (opcode 0x1D, PALmode only)
        applyBoxResult(slot, executeHW_MTPR(slot));
        commitPalResult(slot);                    // Commit Pal Results
    }

    AXP_HOT AXP_ALWAYS_INLINE auto executePAL1E(PipelineSlot& slot) -> void
    {
        // HW_RET / HW_REI - Return from PALmode (opcode 0x1E)
        applyBoxResult(slot, executeREI(slot));
        commitPalResult(slot);                    // Commit Pal Results
    }

    auto executeRDKSP(PipelineSlot& slot) -> void
    {
        static bool warned = false;
        if (!warned)
        {
            ERROR_LOG(QString("UNIMPLEMENTED: %1").arg(Q_FUNC_INFO));
            warned = true;
        }
        commitPalResult(slot);                    // Commit Pal Results
    }

    auto executeMTPR_SIRR(PipelineSlot& slot) -> void
    {
        m_palService->executeMFPR_SIRR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    auto executeMTPR_ASTSR(PipelineSlot& slot) -> void
    {
        m_palService->executeMTPR_ASTSR(slot, slot.palResult);
        commitPalResult(slot);                    // Commit Pal Results
    }

    // Prevent copying
    PalBox(const PalBox&)                    = delete;
    auto operator=(const PalBox&) -> PalBox& = delete;
    ~PalBox()                                = default;

    /**
     * @brief Unified PAL entry point for all entry reasons
     *
     * Handles both CALL_PAL instructions and fault/interrupt entries.
     * Saves complete CPU context, computes entry PC, activates shadow
     * registers, and enters PAL mode.
     *
     * @param reason Why we're entering PAL mode
     * @param vectorOrSelector PAL function code or exception vector
     * @param faultPC PC to save (return address or fault PC)
     * @return BoxResult with pipeline flush request
     */
    AXP_HOT inline auto enterPal(PalEntryReason reason, quint64 vectorOrSelector, quint64 faultPC) noexcept -> BoxResult
    {
        // 1. Record metadata
        m_entryReason = reason;
        m_entryVector = vectorOrSelector;
        m_faultPC     = faultPC;

        // 2. Save context (UNIFIED - same for all entry types)
        m_iprGlobalMaster->saveContext();

        // 3. Compute entry PC
        quint64 entryPC;
        if (reason == PalEntryReason::CALL_PAL_INSTRUCTION)
        {
            entryPC = m_iprGlobalMaster->computeCallPalEntry( vectorOrSelector);
        }
        else
        {
            entryPC = vectorOrSelector;  // Direct vector for faults
        }

        // 4. Set exception address
        m_iprGlobalMaster->h->exc_addr = faultPC;     // fast path update the exception address in the IPR64EXT
        //setEXC_ADDR_Active(m_cpuId, faultPC);
        // For CALL_PAL: faultPC is pc+4 (return address)
        // For faults: faultPC is faulting instruction (retry address)

        // 5. Enter PAL mode (CENTRALIZED - only place PC[0] set)
        // setCM_Active(m_cpuId, CM_KERNEL);
        //setPC_Active(m_cpuId, entryPC | 0x1ULL);
        m_iprGlobalMaster->h->pc = entryPC | 0x1ULL;             // fast path // updates the PC
        m_iprGlobalMaster->h->setIPL_Unsynced( 7);
        m_iprGlobalMaster->h->setCM(CM_KERNEL);    // fast path // updates the PS and shadow CM


        // 6. Activate shadow registers
        m_shadowRegsActive = true;

        DEBUG_LOG(QString("PalBox CPU %1: Entered PAL mode reason=%2 vector=0x%3 PC=0x%4")
            .arg(m_cpuId)
            .arg(static_cast<int>(reason))
            .arg(vectorOrSelector, 16, 16, QChar('0'))
            .arg(entryPC, 16, 16, QChar('0')));

        // 7. Return flush request
        return BoxResult().flushPipeline();
    }


    // exception handlers

    AXP_HOT AXP_ALWAYS_INLINE auto handleException(const BoxResult& result) const noexcept -> void
    {
        TrapCode_Class faultClass = result.getFaultClass();
        quint64        faultVA    = result.getFaultVA();
        quint64        faultPC    = result.getFaultPC();

        DEBUG_LOG(QString("PalBox CPU %1: Handling exception class=%2 VA=0x%3 PC=0x%4")
            .arg(m_cpuId)
            .arg(static_cast<int>(faultClass))
            .arg(faultVA, 16, 16, QChar('0'))
            .arg(faultPC, 16, 16, QChar('0')));

        switch (faultClass)
        {
        case TrapCode_Class::DTB_MISS:
            handleDTB_Miss(faultVA, faultPC);
            break;

        case TrapCode_Class::ITB_MISS:
            handleITB_Miss(faultVA, faultPC);
            break;

        case TrapCode_Class::DTB_FAULT:
            handleDFault(faultVA, faultPC);
            break;

        case TrapCode_Class::ITB_FAULT:
            handleIFault(faultVA, faultPC);
            break;

        case TrapCode_Class::UN_ALIGNED:
            handleUnaligned(faultVA, faultPC);
            break;

        case TrapCode_Class::ARITHMETIC_TRAP:
            handleArithmetic(faultPC, 0);
            break;

        default:
            ERROR_LOG(QString("PalBox CPU %1: Unhandled exception class: %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(faultClass)));
            break;
        }
    }

    /**
     * @brief Get CPU ID this PalBox is associated with
     * @return CPU identifier
     */
    AXP_HOT AXP_ALWAYS_INLINE auto cpuId() const noexcept -> CPUIdType
    {
        return m_cpuId;
    }

    // ====================================================================
    // CALL_PAL Handler
    // ====================================================================

    /**
     * @brief Handle CALL_PAL instruction (opcode 0x00)
     *
     * Extracts PAL function code and enters PAL mode with return
     * address set to instruction after CALL_PAL.
     *
     * @param slot Pipeline slot containing CALL_PAL instruction
     * @return BoxResult with PAL entry request
     */

    AXP_HOT AXP_ALWAYS_INLINE auto executeCALL_PALL(PipelineSlot& slot) noexcept -> BoxResult
    {
        const quint8 palFunction = getFunctionCode(slot.di);

        DEBUG_LOG(QString("PalBox CPU %1: CALL_PAL func=0x%2 PC=0x%3")
            .arg(m_cpuId)
            .arg(palFunction, 2, 16, QChar('0'))
            .arg(slot.di.pc, 16, 16, QChar('0')));

        // ================================================================
        // Option 1: Check for emulator-registered handler FIRST
        // ================================================================
        auto& palTable = global_PalVectorTable();
        if (palTable.hasHandler(palFunction))
        {
            PalArgumentPack args;
            PalResult       result;

            if (palTable.executeHandler(palFunction, args, slot.cpuId, result))
            {
                return convertPalResult(slot, slot.di.pc);  // Convert PalResult -> BoxResult
            }
        }

        // ================================================================
        // Option 2: Vector to PAL firmware in memory
        // ================================================================
        return enterPal(
            PalEntryReason::CALL_PAL_INSTRUCTION,
            palFunction,
            slot.di.pc + 4
        );
    }

    // ====================================================================
    // HW_LD - Hardware Load (opcode 0x1B) - PALmode only
    // ====================================================================
    //
    // EV6 encoding:
    //   [31:26] opcode = 0x1B
    //   [25:21] Ra     = destination register
    //   [20:16] Rb     = base register
    //   [15]    phys   = 1: physical address (bypass DTB), 0: virtual
    //   [14]    alt    = use alternate processor mode for access check
    //   [13]    wchk   = check write permission (pre-fault for stores)
    //   [12]    quad   = 1: quadword (8 bytes), 0: longword (4 bytes, sign-ext)
    //   [11:0]  disp   = signed 12-bit byte displacement
    //
    // EA = Rb + sign_extend(disp[11:0])
    //
    // Physical mode (phys=1): EA is physical address, no TLB lookup
    // Virtual mode  (phys=0): EA is virtual, translated through DTB
    // ====================================================================

    AXP_HOT inline auto executeHW_LD(PipelineSlot& slot) const noexcept -> BoxResult
    {
        // Validate PAL mode
        if (!(m_iprGlobalMaster->isInPalMode()))
        {
            ERROR_LOG(QString("PalBox CPU %1: HW_LD outside PAL mode").arg(m_cpuId));
            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                   .flushPipeline();
        }

        const quint32 raw  = slot.di.rawBits();
        const quint8  ra   = (raw >> 21) & 0x1F;
        const quint8  rb   = (raw >> 16) & 0x1F;
        const bool    phys = (raw >> 15) & 1;
        // const bool alt  = (raw >> 14) & 1;  // TODO: alternate mode access
        const bool wchk = (raw >> 13) & 1;
        const bool quad = (raw >> 12) & 1;

        // Sign-extend 12-bit displacement to 64 bits
        const qint64 disp = static_cast<qint64>(
            static_cast<qint16>(static_cast<quint16>((raw & 0xFFF) << 4))) >> 4;

        // Compute effective address: EA = Rb + SEXT(disp)
        const quint64 rbVal = (rb == 31) ? 0ULL : slot.readIntReg(rb);
        const quint64 ea    = rbVal + static_cast<quint64>(disp);

        quint64 value  = 0;
        auto    status = MEM_STATUS::Ok;

        if (phys)
        {
            // Physical mode: bypass TLB, access guest physical memory directly
            //       auto& guestMem = global_GuestMemory();
            if (quad)
            {
                status = m_guestMemory->read64(ea, value);          // Fast Path
            }
            else
            {
                quint32 tmp = 0;
                status      = m_guestMemory->read32(ea, tmp);                //Fast Path
                // Longword loads are sign-extended to 64 bits (EV6 spec)
                value = static_cast<quint64>(static_cast<qint32>(tmp));
            }
        }
        else
        {
            // Virtual mode: translate through DTB
            quint64  pa  = 0;
            AlphaPTE pte = {};
            auto     cm  = static_cast<Mode_Privilege>(m_iprGlobalMaster->h->cm);

            TranslationResult tResult = m_ev6Translator->ev6TranslateFullVA(
                ea,
                wchk ? AccessKind::WRITE : AccessKind::READ,
                cm, pa, pte);

            if (tResult != TranslationResult::Success)
            {
                DEBUG_LOG(QString("PalBox CPU %1: HW_LD virtual xlate failed EA=0x%2")
                    .arg(m_cpuId).arg(ea, 16, 16, QChar('0')));
                return BoxResult()
                       .setTrapCodeFaultClass(TrapCode_Class::DTB_MISS)
                       .setFaultVA(ea)
                       .flushPipeline();
            }

            //auto& guestMem = global_GuestMemory();
            if (quad)
                status = m_guestMemory->read64(pa, value);      // Fast Path
            else
            {
                quint32 tmp = 0;
                status      = m_guestMemory->read32(pa, tmp);        // Fast Path
                value       = static_cast<quint64>(static_cast<qint32>(tmp));
            }
        }

        if (status != MEM_STATUS::Ok)
        {
            ERROR_LOG(QString("PalBox CPU %1: HW_LD memory error EA=0x%2")
                .arg(m_cpuId).arg(ea, 16, 16, QChar('0')));
            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::MACHINE_CHECK)
                   .flushPipeline();
        }

        // Write result to destination register
        if (ra != 31)
            slot.writeIntReg(ra, value);

        // Also set slot result fields for writeback stage
        slot.ra_value = value;
        slot.writeRa  = (ra != 31);

        DEBUG_LOG(QString("PalBox CPU %1: HW_LD%2 %3 EA=0x%4 -> R%5=0x%6")
            .arg(m_cpuId)
            .arg(quad ? "Q" : "L")
            .arg(phys ? "PHYS" : "VIRT")
            .arg(ea, 16, 16, QChar('0'))
            .arg(ra)
            .arg(value, 16, 16, QChar('0')));

        return BoxResult();
    }

    // ====================================================================
    // HW_ST - Hardware Store (opcode 0x1F) - PALmode only
    // ====================================================================
    //
    // Same encoding as HW_LD but writes Ra value to memory.
    //   [15] phys  [12] quad  [11:0] disp
    //
    // EA = Rb + sign_extend(disp[11:0])
    // MEM[EA] = Ra (quad: 8 bytes, !quad: low 4 bytes)
    // ====================================================================

    AXP_HOT inline auto executeHW_ST(PipelineSlot& slot) const noexcept -> BoxResult
    {
        // Validate PAL mode
        if (!(m_iprGlobalMaster->isInPalMode()))
        {
            ERROR_LOG(QString("PalBox CPU %1: HW_ST outside PAL mode").arg(m_cpuId));
            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                   .flushPipeline();
        }

        const quint32 raw  = slot.di.rawBits();
        const quint8  ra   = (raw >> 21) & 0x1F;
        const quint8  rb   = (raw >> 16) & 0x1F;
        const bool    phys = (raw >> 15) & 1;
        // const bool alt  = (raw >> 14) & 1;  // TODO: alternate mode
        const bool quad = (raw >> 12) & 1;

        // Sign-extend 12-bit displacement
        const qint64 disp = static_cast<qint64>(
            static_cast<qint16>(static_cast<quint16>((raw & 0xFFF) << 4))) >> 4;

        // Compute effective address
        const quint64 rbVal = (rb == 31) ? 0ULL : slot.readIntReg(rb);
        const quint64 ea    = rbVal + static_cast<quint64>(disp);

        // Value to store
        const quint64 raVal = (ra == 31) ? 0ULL : slot.readIntReg(ra);

        auto status = MEM_STATUS::Ok;

        if (phys)
        {
            // Physical mode: bypass TLB
            //auto& guestMem = global_GuestMemory();
            if (quad)
                status = m_guestMemory->write64(ea, raVal);                 // Fast Path
            else
                status = m_guestMemory->write32(ea, static_cast<quint32>(raVal));   // Fast Path
        }
        else
        {
            // Virtual mode: translate through DTB
            quint64  pa  = 0;
            AlphaPTE pte = {};
            auto     cm  = static_cast<Mode_Privilege>(m_iprGlobalMaster->h->getCM());

            TranslationResult tResult = m_ev6Translator->ev6TranslateFullVA(
                ea, AccessKind::WRITE, cm, pa, pte);

            if (tResult != TranslationResult::Success)
            {
                DEBUG_LOG(QString("PalBox CPU %1: HW_ST virtual xlate failed EA=0x%2")
                    .arg(m_cpuId).arg(ea, 16, 16, QChar('0')));
                return BoxResult()
                       .setTrapCodeFaultClass(TrapCode_Class::DTB_MISS)
                       .setFaultVA(ea)
                       .flushPipeline();
            }

            //auto& guestMem = global_GuestMemory();
            if (quad)
                status = m_guestMemory->write64(pa, raVal);                             // Fast Path
            else
                status = m_guestMemory->write32(pa, static_cast<quint32>(raVal));             // Fast Path
        }

        if (status != MEM_STATUS::Ok)
        {
            ERROR_LOG(QString("PalBox CPU %1: HW_ST memory error EA=0x%2")
                .arg(m_cpuId).arg(ea, 16, 16, QChar('0')));
            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::MACHINE_CHECK)
                   .flushPipeline();
        }


        return BoxResult();
    }

    // ====================================================================
    // HW_REI wrapper (opcode 0x1E) - delegates to executeREI
    // ====================================================================

    // ============================================================================
    // REI - Return from Exception/Interrupt
    // ============================================================================
    /**
     * @brief Return from PAL exception/interrupt handler
     *
     * Restores processor state from snapshot and returns to interrupted code.
     * This is the counterpart to exception entry (which saves to snapshot).
     *
     * Restoration process:
     * 1. Restore full context from snapshot (PC, PS, registers)
     * 2. Extract IPL from restored PS
     * 3. Sync IPL with IRQController
     * 4. Exit PAL mode
     * 5. Resume execution at restored PC
     *
     * Arguments: None
     * Returns: Does not return (transfers control to restored PC)
     *
     * Note: This is the PAL exit path for exceptions and interrupts.
     *       NOT used for CALL_PAL returns (those use normal return).
     *
     */
    AXP_HOT AXP_ALWAYS_INLINE auto executeREI(PipelineSlot& slot) noexcept -> BoxResult
    {
        // Validate in PAL mode
        //if (!(getPC_Active(m_cpuId) & 0x1))
        if (!(m_iprGlobalMaster->isInPalMode()))                                                       // Fast Path
        {
            ERROR_LOG(QString("PalBox CPU %1: HW_REI outside PAL mode")
                .arg(m_cpuId));

            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                   .flushPipeline();
        }

        DEBUG_LOG(QString("PalBox CPU %1: HW_REI - exiting PAL mode")
            .arg(m_cpuId));

        // Restore context
        m_iprGlobalMaster->restoreContext();
        // Deactivate shadow registers
        m_shadowRegsActive = false;

        // Ensure PC[0] clear (exit PAL mode)
        //quint64 pc = getPC_Active(m_cpuId);
        if (m_iprGlobalMaster->h->pc & 0x1)                                                             // Fast Path
        {
            m_iprGlobalMaster->h->pc = (m_iprGlobalMaster->h->pc & ~0x1ULL);
        }

        DEBUG_LOG(QString("PalBox CPU %1: Exited PAL mode, PC=0x%2")
            .arg(m_cpuId)
            .arg(m_iprGlobalMaster->h->pc & ~0x1ULL, 16, 16, QChar('0')));

        // Return flush request
        return BoxResult().flushPipeline();
    }


    AXP_HOT AXP_ALWAYS_INLINE auto handleDTB_Miss(quint64 faultVA, quint64 faultPC) const noexcept -> void
    {
        // Use ev6TranslateFullVA - it does EVERYTHING:
        // 1. Walks page table
        // 2. Fills TLB automatically
        // 3. Returns PA and PTE

        quint64  pa;
        AlphaPTE pte;
        auto     mode = static_cast<Mode_Privilege>(m_iprGlobalMaster->h->getCM());

        TranslationResult result = m_ev6Translator->ev6TranslateFullVA(

            faultVA,
            AccessKind::READ,  // Assume read for now
            mode,
            pa,
            pte
        );


        if (result == TranslationResult::Success)
        {
            DEBUG_LOG(QString("PalBox CPU %1: DTB filled VA=0x%2 -> PA=0x%3 PTE=0x%4")
                .arg(m_cpuId)
                .arg(faultVA, 16, 16, QChar('0'))
                .arg(pa, 16, 16, QChar('0'))
                .arg(pte.raw, 16, 16, QChar('0')));

            qDebug() << QString("PalBox CPU %1: DTB filled VA=0x%2 -> PA=0x%3 PTE=0x%4")
                        .arg(m_cpuId)
                        .arg(faultVA, 16, 16, QChar('0'))
                        .arg(pa, 16, 16, QChar('0'))
                        .arg(pte.raw, 16, 16, QChar('0'));
        }
        else
        {
            // Translation failed - escalate to OS
            ERROR_LOG(QString("PalBox CPU %1: DTB translation failed: %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(result)));

            // Create appropriate fault event
            auto         faultType = MemoryFaultType::PAGE_NOT_PRESENT;
            PendingEvent ev        = makeDTBFaultEvent(m_cpuId, faultVA, false, faultType);
            m_faultDispatcher->setPendingEvent(ev);                                                 // Fast Path
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto handleITB_Miss(quint64 faultVA, quint64 faultPC) const noexcept -> void
    {
        DEBUG_LOG(QString("PalBox CPU %1: ITB_MISS VA=0x%2")
            .arg(m_cpuId)
            .arg(faultVA, 16, 16, QChar('0')));

        quint64  pa;
        AlphaPTE pte;
        auto     mode = static_cast<Mode_Privilege>(m_iprGlobalMaster->h->getCM());

        // ev6TranslateFullVA does everything for instruction TLB too
        TranslationResult result = m_ev6Translator->ev6TranslateFullVA(
            faultVA,
            AccessKind::EXECUTE,  // Instruction fetch
            mode,
            pa,
            pte
        );

        if (result == TranslationResult::Success)
        {
            DEBUG_LOG(QString("PalBox CPU %1: ITB filled VA=0x%2 -> PA=0x%3")
                .arg(m_cpuId)
                .arg(faultVA, 16, 16, QChar('0'))
                .arg(pa, 16, 16, QChar('0')));
        }
        else
        {
            ERROR_LOG(QString("PalBox CPU %1: ITB translation failed: %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(result)));

            PendingEvent ev = makeITBMissEvent(m_cpuId, faultVA);
            m_faultDispatcher->setPendingEvent(ev);                                           // Fast Path
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto handleDFault(quint64 faultVA, quint64 faultPC) const noexcept -> void
    {
        // Data faults are permission violations PAL cannot resolve
        // Examples: Access Control Violation, Fault On Read/Write/Execute

        DEBUG_LOG(QString("PalBox CPU %1: DFAULT VA=0x%2 - escalating to OS")
            .arg(m_cpuId)
            .arg(faultVA, 16, 16, QChar('0')));

        // Create fault event for OS to handle
        // OS will typically send SIGSEGV to the process
        PendingEvent ev = makeDTBAccessViolationEvent(m_cpuId, faultVA, false);
        m_faultDispatcher->setPendingEvent(ev);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto handleIFault(quint64 faultVA, quint64 faultPC) const noexcept -> void
    {
        // Instruction fetch permission violation

        DEBUG_LOG(QString("PalBox CPU %1: IFAULT VA=0x%2 - escalating to OS")
            .arg(m_cpuId)
            .arg(faultVA, 16, 16, QChar('0')));

        PendingEvent ev = makeITBAccessViolationEvent(m_cpuId, faultVA);
        m_faultDispatcher->setPendingEvent(ev);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto handleUnaligned(quint64 faultVA, quint64 faultPC) const noexcept -> void
    {
        // Unaligned memory access
        // OS can either:
        //   - Emulate the access (slow but correct)
        //   - Terminate the process (SIGBUS)

        DEBUG_LOG(QString("PalBox CPU %1: UNALIGNED VA=0x%2 - escalating to OS")
            .arg(m_cpuId)
            .arg(faultVA, 16, 16, QChar('0')));

        PendingEvent ev = makeUnalignedEvent(m_cpuId, faultVA, false);
        m_faultDispatcher->setPendingEvent(ev);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto handleArithmetic(quint64 faultPC, quint64 excSum) const noexcept -> void
    {
        DEBUG_LOG(QString("PalBox CPU %1: ARITHMETIC TRAP - vectoring to OS")
            .arg(m_cpuId));

        // 1. Build exception frame on kernel stack
        //quint64 ksp = getKSP_Active(m_cpuId);
        quint64 ksp = m_iprGlobalMaster->h->ksp;                                                 // Fast Path
        //	buildExceptionFrame(ksp, faultPC, excSum, ExceptionClass_EV6::Arithmetic);

        // 2. Get OS entry point from SCBB
        //quint64 scbb = globalIPRHotExt(m_cpuId).scbb;
        quint64 scbb    = m_iprGlobalMaster->x->scbb;                                    // Fast Path
        quint64 osEntry = scbb + (0x00 * 16);  // Arithmetic trap vector

        // 3. Transfer control to OS
        //setPC_Active(m_cpuId, osEntry);  // No PC[0] - exit PAL mode
        //setCM_Active(m_cpuId, CM_KERNEL);
        m_iprGlobalMaster->h->pc = osEntry;                                                      // Fast Path
        m_iprGlobalMaster->h->setCM(CM_KERNEL);                                     // Fast Path

        DEBUG_LOG(QString("PalBox CPU %1: Transferred to OS @ 0x%2")
            .arg(m_cpuId)
            .arg(osEntry, 16, 16, QChar('0')));
    }


    // ============================================================================
    // Global Accessor
    // ============================================================================

    /**
     * @brief Convenience global accessor for PalBox singleton
     * @param cpuId CPU identifier
     * @return Reference to PalBox instance for specified CPU
     */

    // AXP_HOT AXP_FLATTEN static auto instance(CPUIdType cpuId) noexcept -> PalBox&
    // {
    //     static PalBox s_instance(cpuId);
    //     return s_instance;
    // }

    AXP_HOT inline PalBox(CPUIdType cpuId, IRQPendingState* pendingState, InterruptRouter* interruptRouter) noexcept
    : m_cpuId(cpuId)
      , m_palService(std::make_unique<PalService>(cpuId, pendingState,interruptRouter))
      , m_entryVector(0), m_faultPC(0)
      , m_pending(pendingState)
      , m_router{(interruptRouter)}
      , m_entryReason(PalEntryReason::CALL_PAL_INSTRUCTION)
    , m_guestMemory(&global_GuestMemory())
    , m_faultDispatcher(&globalFaultDispatcher(cpuId))
        , m_iprGlobalMaster(getCPUStateView(cpuId))
    {
        DEBUG_LOG(QString("PalBox: Initialized for CPU %1").arg(cpuId));
        m_ev6Translator.reset(new Ev6Translator(m_cpuId));

    }


    // ====================================================================
    // commitPalResult - Single exit point for ALL PalService results
    // Called after every PalService dispatch, before returning to run loop.
    // Applies all deferred architectural state from PalResult.
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void commitPalResult(PipelineSlot& slot)  noexcept
    {
        const PalResult& pr = slot.palResult;

        // 1. GPR writeback
        if (pr.hasReturnValue && pr.returnReg != PalReturnReg::NONE)
        {
            const quint8 reg = static_cast<quint8>(pr.returnReg);
            if (reg != 31)
                m_iprGlobalMaster->writeInt(reg, pr.returnValue);
        }

        // 2. PC update
        if (pr.pcModified)
        {
            m_iprGlobalMaster->h->pc = pr.newPC;
        }

        // 3. Side-effect flags -> pipeline signals
        // Pipeline flush (combines several triggers)
        if (pr.has(PipelineEffect::kRequestPipelineFlush |
            PipelineEffect::kDrainWriteBuffers))
            slot.flushPipeline = true;

        if (pr.has(PipelineEffect::kFlushPendingTraps)) {
            const quint8 currentIPL = m_iprGlobalMaster->h->getIPL();
            if (m_pending->hasDeliverable(currentIPL)) {
                ClaimedInterrupt claimed = m_pending->claimNext(currentIPL);
                if (claimed.valid) {
                    // If software interrupt, clear IPR.SISR bit
                    m_palService->clearSisrIfSoftware(claimed);
                    // if (IrqSource::isSoftwareSource(claimed.source)) {
                    //     m_iprGlobalMaster->h->sisr &=
                    //         ~static_cast<quint16>(1u << claimed.ipl);
                    // }
                    m_palService->deliverInterrupt( claimed);
                }
            }
        }

        if (pr.has(PipelineEffect::kMemoryBarrier))
            slot.m_cBox->RequestMemoryBarrier(slot, MemoryBarrierKind::PAL);

        if (pr.has(PipelineEffect::kNotifyHalt))
            slot.halted = true;

        // --- The missing handlers ---

        if (pr.has(PipelineEffect::kTLBModified))
            slot.m_mBox->invalidateCachedTranslations();

        if (pr.has(PipelineEffect::kIPLChanged))
            slot.m_cBox->reevaluatePendingInterrupts();

        if (pr.has(PipelineEffect::kContextSwitched))
            slot.m_cBox->reloadProcessContext();

        if (pr.has(PipelineEffect::kPCBBChanged))
            slot.m_cBox->updatePCBBPointer();

        if (pr.has(PipelineEffect::kClearBranchPredictor))
            slot.m_cBox->flushBranchPredictor();

        if (pr.has(PipelineEffect::kFlushPendingIPRWrites))
            slot.m_cBox->commitStagedIPRWrites();

        // 4. Fault routing — suppress writeback if PAL didn't return
        if (!pr.doesReturn)
            slot.needsWriteback = false;

#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_PAL_COMMIT(m_cpuId,
            pr.hasReturnValue ? static_cast<quint8>(pr.returnReg) : quint8(31),
            pr.hasReturnValue ? pr.returnValue : 0,
            pr.pcModified,
            pr.pcModified ? pr.newPC : 0,
            slot.flushPipeline);
#endif
    }

    AXP_HOT AXP_ALWAYS_INLINE auto execute(PipelineSlot& slot, PalResult& palResult) noexcept -> BoxResult
    {
        switch (const quint8 opcode = extractOpcode(slot.di.rawBits()))
        {
        case 0x00:  // CALL_PAL
        case 0x19:  // HW_MFPR
            return executeHW_MFPR(slot);

        case 0x1B:  // HW_LD (physical/virtual load, PALmode only)
            return executeHW_LD(slot);

        case 0x1D:  // HW_MTPR
            return executeHW_MTPR(slot);

        case 0x1E:  // HW_RET / HW_REI
            return executeREI(slot);

        case 0x1F:  // HW_ST (physical/virtual store, PALmode only)
            return executeHW_ST(slot);

        default:
            // Unknown PAL opcode
            ERROR_LOG(QString("PalBox CPU %1: Illegal PAL opcode 0x%2")
                .arg(m_cpuId)
                .arg(opcode, 2, 16, QChar('0')));

            return BoxResult()
                   .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                   .flushPipeline();
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto isInPalMode() const noexcept -> bool
    {
        return m_palService && m_palService->isInPalMode();
    }

    AXP_ALWAYS_INLINE auto palService() const noexcept -> PalService*
    {
        return m_palService.get();
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    AXP_HOT inline auto executeHW_MFPR(PipelineSlot& slot) noexcept -> BoxResult
    {
        if (!m_iprGlobalMaster->isInPalMode())
        {
            return BoxResult()
                .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                .flushPipeline();
        }

        const quint16 iprIndex = getFunctionCode(slot.di);
        const quint8 ra = slot.di.ra;
        quint64 result = 0;

        m_palService->readIPR(static_cast<HW_IPR>(iprIndex), result);       // Reads the registers by IPR function code. Updates result

#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_IPR_READ(m_cpuId, iprIndex, result);
#endif

        // Ra <- IPR value (architectural contract)
        if (ra != 31)
            m_iprGlobalMaster->writeInt(ra, result);

        return BoxResult().advance();
    }

    AXP_HOT inline auto executeHW_MTPR(PipelineSlot& slot) noexcept -> BoxResult
    {
        if (!m_iprGlobalMaster->isInPalMode())
        {
            return BoxResult()
                .setTrapCodeFaultClass(TrapCode_Class::ILLEGAL_INSTRUCTION)
                .flushPipeline();
        }

        const quint16 iprIndex = getFunctionCode(slot.di);
        const quint8 rb = slot.di.rb;       // source register
        const quint64 value = m_iprGlobalMaster->readInt(rb);

#if AXP_INSTRUMENTATION_TRACE
        quint64 result;
        m_palService->readIPR(static_cast<HW_IPR>(iprIndex), result);
        EXECTRACE_IPR_WRITE(m_cpuId, iprIndex, value, result);
#endif

        m_palService->writeIPR(static_cast<HW_IPR>(iprIndex), slot);

        return BoxResult().advance();
    }


    AXP_HOT AXP_ALWAYS_INLINE auto executeRETSYS_OSF(PipelineSlot& slot, PalResult& result) noexcept -> void
    {
        // RETSYS is essentially REI for system calls
        // Restores user context from snapshot
        executeREI(slot);

        DEBUG_LOG(QString("CPU %1: RETSYS_OSF - returning to user mode")
            .arg(slot.cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE auto convertPalResult(PipelineSlot& slot,
        quint64 pc) const noexcept -> BoxResult
    {
        BoxResult br{};
        const auto& pr = slot.palResult;

        // ============================================================
        // 1. Status-based routing
        // ============================================================
        switch (pr.status)
        {
        case PalStatus::Success:
            br.advance();
            break;
        case PalStatus::Fault:
            br.setFaultInfo(pr.trapCode, pc, pr.faultVA);
            break;
        case PalStatus::RequiresPalMode:
            br.requestEnterPalMode();
            br.pcModified = true;
            break;
        case PalStatus::Halt:
            br.stallPipeline();
            br.pcModified = true;
            break;
        case PalStatus::Retry:
            br.flushPipeline();
            break;
        }

        // ============================================================
        // 2. Return value -> DEFERRED (slot carries it to commit)
        // ============================================================
        // Do NOT write registers here. The slot already holds:
        //   slot.palResult.hasReturnValue
        //   slot.palResult.returnReg
        //   slot.palResult.returnValue
        // Pipeline commit stage calls commitPalResult() below.

        // ============================================================
        // 3. Side-effect flags -> BoxResult
        // ============================================================
        if (pr.pcModified)
        {
            m_iprGlobalMaster->h->pc = pr.newPC;
            br.pcModified = true;
        }
        if (pr.hasRequestPipelineFlush() )
            br.flushPipeline();
        if (pr.hasDrainWriteBuffers())
            br.drainWriteBuffers();
        if (pr.hasMemoryBarrier())
            br.requestMemoryBarrier();
        if (pr.hasNotifyHalt())
            br.requestHalted();

        return br;
    }

    
};
#endif // PALBOXBASE_H
