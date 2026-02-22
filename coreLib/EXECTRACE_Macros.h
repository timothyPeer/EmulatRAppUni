// ============================================================================
// EXECTRACE_Macros.h - Instrumentation Macros for Alpha AXP Emulator
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

#ifndef EXECTRACE_MACROS_H
#define EXECTRACE_MACROS_H
// ============================================================================
// EXECTRACE_Macros.h - Instrumentation Macros for Alpha AXP Emulator
// ============================================================================
// All macros compile to nothing when EXECTRACE_ENABLED is not defined.
//
// Placement guide:
//   TIER 1 (Pipeline)  -> AlphaPipeline.h  (stage_WB, flush)
//   TIER 2 (PAL)       -> PalBoxBase.h     (enterPal, commitPalResult, forwarders)
//                       -> AlphaCPU.h       (enterPalMode, executeREI)
//   TIER 3 (Faults)    -> AlphaPipeline.h  (stage_EX, stage_MEM, stage_WB)
//                       -> AlphaCPU.h       (runOneInstruction fault handler)
//   TIER 4 (IPR)       -> PalBoxBase.h     (executeHW_MFPR, executeHW_MTPR)
// ============================================================================

#include "ExecTrace.h"
#include "machineLib/PipeLineSlot.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/iGrain_helper_inl.h"

#ifdef EXECTRACE_ENABLED

// ============================================================================
// EXISTING: Instruction Commit (stage_EX)
// ============================================================================

#define EXECTRACE_COMMIT_GRAIN_ASM(slot) \
    do { \
        ExecTrace::recordCommitWithGrain( \
            (slot).cpuId, \
            (slot).di.pc, \
            (slot).di.rawBits(), \
            (slot).di.opcode(), \
            (slot).di.functionCode(), \
            (slot).di.grain->mnemonic().toUtf8().constData(), \
            getGrainTypeName((slot).di.grain->grainType()), \
            static_cast<quint8>((slot).di.grain->grainType()), \
            (slot).di.grain != nullptr, \
            &(slot) \
        ); \
        ExecTrace::recordCommitAsAssembly( \
            (slot).cpuId, \
            (slot).di.pc, \
            (slot).di.raw, \
            (slot).di.grain->mnemonic().toUtf8().constData(), \
            (slot) \
        ); \
    } while(0)

#define EXECTRACE_COMMIT_GRAIN(slot) \
    ExecTrace::recordCommitWithGrain( \
        (slot).cpuId, \
        (slot).di.pc, \
        (slot).di.rawBits(), \
        (slot).di.grain->opcode(), \
        (slot).di.grain->functionCode(), \
        (slot).di.grain->mnemonic().toUtf8().constData(), \
        getGrainTypeName((slot).di.grain->grainType()), \
        static_cast<quint8>((slot).di.grain->grainType()), \
        (slot).di.grain != nullptr, \
        &(slot) \
    )

#define EXECTRACE_COMMIT_GRAIN_WITH_WRITES(slot, writes, writeCount) \
    do { \
        if (ExecTrace::isEnabled() && (slot).valid && (slot).grain) { \
            ExecTrace::recordCommitWithWrites( \
                (slot).cpuId, \
                (slot).di.pc, \
                (slot).grain->rawBits, \
                writes, \
                writeCount \
            ); \
        } \
    } while(0)

// ============================================================================
// TIER 1: Pipeline Lifecycle
// ============================================================================
// Place in: AlphaPipeline.h

// stage_WB retired an instruction normally (after commitPending, after checks)
// Place at: end of stage_WB normal path, after store commit / retire
#define EXECTRACE_WB_RETIRE(slot) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordWBRetire( \
                (slot).cpuId, \
                (slot).di.pc, \
                (slot).di.rawBits(), \
                (slot).di.grain ? (slot).di.grain->mnemonic().toUtf8().constData() : "??" \
            ); \
        } \
    } while(0)

// Deferred register write committed from m_pending
// Place at: commitPending() inside stage_WB, when m_pending.valid
//   reg    = m_pending.destReg
//   value  = m_pending.value
//   fromPC = m_pending.instrPC (PC of the instruction that produced this write)
#define EXECTRACE_COMMIT_PENDING(cpuId, reg, value, fromPC) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordCommitPending(cpuId, reg, value, fromPC); \
        } \
    } while(0)

// m_pending discarded (fault/PAL_CALL squashed younger instruction's result)
// Place at: stage_WB fault path and PAL_CALL path where m_pending = {}
//   reason      = DiscardReason::FAULT or DiscardReason::PAL_CALL
//   discardedPC = PC of the instruction whose write was discarded (0 if unknown)
#define EXECTRACE_DISCARD_PENDING(cpuId, reason, discardedPC) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordDiscardPending(cpuId, reason, discardedPC); \
        } \
    } while(0)

// Pipeline flushed
// Place at: every flush() call site
//   source = string literal identifying caller ("stage_WB/fault", "enterPal",
//            "runLoop/PAL_CALL", "REI", "branch_mispredict")
//   pc     = current PC at time of flush
#define EXECTRACE_PIPELINE_FLUSH(cpuId, source, pc) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordPipelineFlush(cpuId, source, pc); \
        } \
    } while(0)

// ============================================================================
// TIER 2: PAL Entry / Exit
// ============================================================================
// Place in: PalBoxBase.h (enterPal, commitPalResult), AlphaCPU.h (REI)

// Entering PAL mode
// Place at: PalBox::enterPal(), after saving context, before returning
//   reason  = PalEntryReasonTrace enum
//   vector  = vector offset or PAL function code
//   faultPC = saved PC (EXC_ADDR)
//   oldPC   = PC before entering PAL
//   oldIPL  = IPL before entering PAL
//   oldCM   = CM before entering PAL (0=kernel, 1=exec, 2=super, 3=user)
#define EXECTRACE_PAL_ENTER(cpuId, reason, vector, faultPC, oldPC, oldIPL, oldCM) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordPalEnter(cpuId, reason, vector, faultPC, \
                                      oldPC, oldIPL, oldCM); \
        } \
    } while(0)

// Exiting PAL mode (HW_REI)
// Place at: executeREI(), after restoring context
//   returnPC = PC being returned to
//   newIPL   = restored IPL
//   newCM    = restored CM
#define EXECTRACE_PAL_EXIT(cpuId, returnPC, newIPL, newCM) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordPalExit(cpuId, returnPC, newIPL, newCM); \
        } \
    } while(0)

// CALL_PAL function dispatched into PalService::execute() switch
// Place at: PalBox::executeCALL_PAL(), after extracting function code
//   palFunction = PalCallPalFunction_enum enum value (cast to quint16)
//   pc          = CALL_PAL instruction PC
//   name        = string name of PAL function (e.g. "CALLSYS", "SWPCTX")
#define EXECTRACE_PAL_DISPATCH(cpuId, palFunction, pc, name) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordPalDispatch(cpuId, palFunction, pc, name); \
        } \
    } while(0)

// commitPalResult applied architectural state
// Place at: PalBox::commitPalResult(), after writing GPR/PC
//   destReg       = register written (31 = none)
//   value         = value written
//   pcModified    = true if PC was changed
//   newPC         = new PC value (0 if unchanged)
//   flushRequested = true if pipeline flush was requested
#define EXECTRACE_PAL_COMMIT(cpuId, destReg, value, pcModified, newPC, flushRequested) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordPalCommit(cpuId, destReg, value, \
                                       pcModified, newPC, flushRequested); \
        } \
    } while(0)

// ============================================================================
// TIER 3: Fault Chain
// ============================================================================
// Place in: AlphaPipeline.h (stage_EX, stage_MEM, stage_WB), AlphaCPU.h

// Fault raised (detected in a pipeline stage, slot.faultPending set)
// Place at: wherever faultPending is set on a slot
//   trapCode = TrapCode_Class value (cast to quint8)
//   faultVA  = faulting virtual address (0 if N/A)
//   faultPC  = PC of faulting instruction
//   stage    = PipelineStage enum where fault was detected
#define EXECTRACE_FAULT_RAISED(cpuId, trapCode, faultVA, faultPC, stage) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordFaultRaised(cpuId, trapCode, faultVA, faultPC, stage); \
        } \
    } while(0)

// Fault dispatched from stage_WB to run loop
// Place at: stage_WB fault handler, just before returning FAULT action
//   trapCode = TrapCode_Class value (cast to quint8)
//   faultVA  = faulting virtual address
//   faultPC  = faulting instruction PC
#define EXECTRACE_FAULT_DISPATCHED(cpuId, trapCode, faultVA, faultPC) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordFaultDispatched(cpuId, trapCode, faultVA, faultPC); \
        } \
    } while(0)

// ============================================================================
// TIER 4: IPR Read / Write (HW_MFPR / HW_MTPR)
// ============================================================================
// Place in: PalBoxBase.h (executeHW_MFPR, executeHW_MTPR)

// HW_MFPR - IPR read
// Place at: executeHW_MFPR(), after readIPR() returns
//   iprIndex = HW_IPR enum value (cast to quint16)
//   value    = value read from IPR
#define EXECTRACE_IPR_READ(cpuId, iprIndex, value) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordIPRRead(cpuId, iprIndex, value); \
        } \
    } while(0)

// HW_MTPR - IPR write
// Place at: executeHW_MTPR(), before and after writeIPR()
//   iprIndex = HW_IPR enum value (cast to quint16)
//   newValue = value being written
//   oldValue = value before write (0 if not captured)
#define EXECTRACE_IPR_WRITE(cpuId, iprIndex, newValue, oldValue) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordIPRWrite(cpuId, iprIndex, newValue, oldValue); \
        } \
    } while(0)

// ============================================================================
// EXISTING: Markers, Triggers
// ============================================================================

#define EXECTRACE_MARKER(cpuId, markerId, arg0, arg1, arg2) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::recordMarker(cpuId, markerId, arg0, arg1, arg2); \
        } \
    } while(0)

#define EXECTRACE_TRIGGER(cpuId, reason) \
    do { \
        if (ExecTrace::isEnabled()) { \
            ExecTrace::trigger(cpuId, reason); \
        } \
    } while(0)


#ifdef AXP_INSTRUMENTATION_TRACE

/**
 * @brief Record interrupt event in execution trace
 * @param cpuId      CPU that received the interrupt
 * @param fromPC     PC at point of interruption
 * @param vector     PAL entry vector (destination)
 * @param type       Interrupt type (HW_INT, AST, CLK, etc.)
 * @param ipl        Current IPL at time of interrupt
 */
#define EXECTRACE_INTERRUPT(cpuId, fromPC, vector, type, ipl) \
    ExecTrace::recordInterrupt(                               \
        (cpuId),                                              \
        (fromPC),                                             \
        (vector),                                             \
        (type),                                               \
        (ipl)                                                 \
    )

#else

#define EXECTRACE_INTERRUPT(cpuId, fromPC, vector, type, ipl) \
    ((void)0)

#endif
// ============================================================================
// EXISTING: Grain Debug
// ============================================================================

#define GRAIN_DEBUG(mnemonic, ...) \
    qDebug() << "===" << mnemonic << "EXECUTE ===" \
             << "PC:" << Qt::hex << slot.di.pc \
             << "Raw:" << Qt::hex << slot.di.grain->rawBits \
             << "Flags:" << Qt::hex << slot.di.grain->flags \
             << "Fmt:" << (slot.di.isOperateFormat() ? "Oper" : \
                            slot.di.isMemoryFormat() ? "Mem" : \
                            slot.di.isBranchFormat() ? "Br" : "PAL") \
             << "Latency:" << Qt::dec << (int)slot.di.grain->latency \
             << __VA_ARGS__

// ============================================================================
#else // EXECTRACE_ENABLED not defined
// ============================================================================
// All macros compile to nothing
// ============================================================================

// Existing
#define EXECTRACE_COMMIT_GRAIN_ASM(slot)                                      do {} while(0)
#define EXECTRACE_COMMIT_GRAIN(slot)                                          do {} while(0)
#define EXECTRACE_COMMIT_GRAIN_WITH_WRITES(slot, writes, writeCount)          do {} while(0)

// Tier 1: Pipeline
#define EXECTRACE_WB_RETIRE(slot)                                             do {} while(0)
#define EXECTRACE_COMMIT_PENDING(cpuId, reg, value, fromPC)                   do {} while(0)
#define EXECTRACE_DISCARD_PENDING(cpuId, reason, discardedPC)                 do {} while(0)
#define EXECTRACE_PIPELINE_FLUSH(cpuId, source, pc)                           do {} while(0)

// Tier 2: PAL
#define EXECTRACE_PAL_ENTER(cpuId, reason, vector, faultPC, oldPC, oldIPL, oldCM)  do {} while(0)
#define EXECTRACE_PAL_EXIT(cpuId, returnPC, newIPL, newCM)                    do {} while(0)
#define EXECTRACE_PAL_DISPATCH(cpuId, palFunction, pc, name)                  do {} while(0)
#define EXECTRACE_PAL_COMMIT(cpuId, destReg, value, pcModified, newPC, flushRequested) do {} while(0)

// Tier 3: Faults
#define EXECTRACE_FAULT_RAISED(cpuId, trapCode, faultVA, faultPC, stage)      do {} while(0)
#define EXECTRACE_FAULT_DISPATCHED(cpuId, trapCode, faultVA, faultPC)         do {} while(0)

// Tier 4: IPR
#define EXECTRACE_IPR_READ(cpuId, iprIndex, value)                            do {} while(0)
#define EXECTRACE_IPR_WRITE(cpuId, iprIndex, newValue, oldValue)              do {} while(0)

// Markers, Triggers
#define EXECTRACE_MARKER(cpuId, markerId, arg0, arg1, arg2)                   do {} while(0)
#define EXECTRACE_TRIGGER(cpuId, reason)                                      do {} while(0)

// Grain debug
#define GRAIN_DEBUG(mnemonic, ...)                                            do {} while(0)

#endif // EXECTRACE_ENABLED
#endif // EXECTRACE_MACROS_H