// ============================================================================
// AlphaPipeline.h - Alpha AXP 6-Stage Pipeline Implementation
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
// PIPELINE ARCHITECTURE OVERVIEW
// ============================================================================
//
// This module implements a 6-stage in-order pipeline for the Alpha AXP
// architecture. The pipeline is designed to execute Alpha instructions
// with proper handling of hazards, exceptions, and architectural state.
//
// PIPELINE STAGES (Index and Execution Order):
// ============================================================================
//
//   Stage 5 [WB ] - Write Back / Retirement (oldest instruction)
//   Stage 4 [MEM] - Memory Access + Register Writeback
//   Stage 3 [EX ] - Execute / Compute
//   Stage 2 [IS ] - Instruction Slot
//   Stage 1 [DE ] - Decode (grain pre-decoded in IF)
//   Stage 0 [IF ] - Instruction Fetch (youngest instruction)
//
// CRITICAL DESIGN DECISIONS
// ============================================================================
//
// 1. REGISTER WRITEBACK IN MEM STAGE (Not WB!)
//    - ALL register writes occur in stage_MEM(), not stage_WB()
//    - This includes: integer registers, float registers, link registers
//    - Rationale: Resolves RAW hazards through intra-cycle ordering
//    - stage_WB() only handles: store commits, retirement, exceptions
//
// 2. MEMORY STORE COMMITS IN WB STAGE
//    - Stores do NOT write to memory in stage_MEM()
//    - Stores commit in stage_WB() after all hazards are cleared
//    - Rationale: Ensures stores can be rolled back on exceptions
//
// 3. RAW HAZARD RESOLUTION (Read-After-Write)
//    - NO forwarding logic required
//    - NO pipeline stalls for register dependencies
//    - Hazards resolved through intra-cycle sequential execution order
//
//    Example: LDA R26, 268(R27)  followed by  LDQ R1, 0(R26)
//
//      Cycle 6 execution order:
//        1. stage_WB()  - (process older instructions)
//        2. stage_MEM() - LDA writes R26 = 0x20008110
//        3. stage_EX()  - LDQ reads R26 = 0x20008110 (correct value!)
//        4. stage_IS()  - (process younger instructions)
//        5. stage_DE()
//        6. stage_IF()
//
//    Because stage_MEM() executes BEFORE stage_EX() in the same cycle,
//    LDQ reads the value that LDA just wrote - no stall needed!
//
// 4. RING BUFFER IMPLEMENTATION
//    - 6-slot circular buffer (m_ringSlots[6])
//    - m_head points to oldest instruction (always in WB stage)
//    - Slot assignment: stage(N) maps to slot[(m_head - (5-N)) % 6]
//    - Advancement: m_head = (m_head + 1) % 6 after each cycle
//
// ============================================================================
// STAGE RESPONSIBILITIES
// ============================================================================
//
// stage_IF() - Instruction Fetch
//   - Fetch 32-bit instruction from physical address
//   - Decode opcode and lookup instruction grain
//   - Perform branch prediction (if applicable)
//   - Advance PC to next instruction
//   - Output: DecodedInstruction with grain reference
//
// stage_DE() - Decode
//   - Mostly a pass-through stage (grain decoded in IF)
//   - Future: Could detect early hazards here
//
// stage_IS() - Instruction Slot
//   - Prepare instruction for execution
//   - Future: Could handle dual-issue logic here
//
// stage_EX() - Execute
//   - Read source registers from register file
//   - Perform ALU operations
//   - Calculate memory addresses
//   - Detect branch mispredictions
//   - Execute instruction-specific logic
//   - Output: Results in slot.payLoad, flags in slot fields
//
// stage_MEM() - Memory Access + Register Writeback
//   CRITICAL: This is where ALL register writes occur!
//
//   Register Writeback Actions:
//     1. Write link registers (BSR, JSR) - Ra = PC+4
//     2. Write integer registers (ADDQ, LDA, etc.) - Ra or Rc = payLoad
//     3. Write float registers (ADDF, etc.) - Fa or Fc = payLoad
//     4. Clear dirty bits in execution boxes
//
//   Memory Actions:
//     - Complete memory loads (data already in payLoad)
//     - Translate store addresses (don't commit stores yet!)
//     - Check alignment for loads/stores
//
//   Does NOT:
//     - Commit stores to memory (happens in WB)
//     - Retire instructions (happens in WB)
//
// stage_WB() - Write Back / Retirement
//   Execution order within stage_WB():
//     1. Check for faults -> early return if fault
//     2. Check for CALL_PAL -> early return if PAL transfer
//     3. Commit stores to memory (only if no fault/PAL)
//     4. Update branch predictor
//     5. Retire instruction (mark as architecturally committed)
//     6. Clear slot
//
//   Note: Registers already written in stage_MEM()!
//
// ============================================================================
// EXECUTION FLOW (tick() method)
// ============================================================================
//
// void tick() {
//     m_cycleCount++;
//
//     // Execute stages in reverse order (oldest to youngest)
//     stage_WB();   // Stage 5 - Retire, commit stores
//     stage_MEM();  // Stage 4 - WRITE REGISTERS HERE
//     stage_EX();   // Stage 3 - READ REGISTERS HERE
//     stage_IS();   // Stage 2
//     stage_DE();   // Stage 1
//     stage_IF();   // Stage 0 - Fetch next
//
//     advanceRing(); // Rotate buffer: m_head = (m_head + 1) % 6
// }
//
// The sequential execution order is CRITICAL:
//   - stage_MEM() writes registers BEFORE stage_EX() reads them
//   - This happens within the same cycle
//   - No stalls or forwarding logic needed for RAW hazards
//
// ============================================================================
// RING BUFFER MECHANICS
// ============================================================================
//
// The pipeline uses a 6-slot circular buffer:
//
//   m_ringSlots[0..5]  - The 6 pipeline slots
//   m_head             - Index of oldest instruction (always in WB)
//
// Stage to Slot Mapping:
//
//   stage(5) [WB ] = m_ringSlots[m_head]           - Oldest
//   stage(4) [MEM] = m_ringSlots[(m_head-1+6) % 6]
//   stage(3) [EX ] = m_ringSlots[(m_head-2+6) % 6]
//   stage(2) [IS ] = m_ringSlots[(m_head-3+6) % 6]
//   stage(1) [DE ] = m_ringSlots[(m_head-4+6) % 6]
//   stage(0) [IF ] = m_ringSlots[(m_head-5+6) % 6] - Youngest
//
// After each cycle, advanceRing() increments m_head:
//   m_head = (m_head + 1) % 6
//
// This causes each instruction to "flow forward" one stage:
//   - Instruction in IF moves to DE
//   - Instruction in DE moves to IS
//   - ... etc ...
//   - Instruction in WB retires (slot cleared)
//
// Example with m_head = 3:
//
//   Cycle N:
//     slot[3] = WB  (head, oldest)
//     slot[2] = MEM
//     slot[1] = EX
//     slot[0] = IS
//     slot[5] = DE
//     slot[4] = IF  (youngest)
//
//   After advanceRing() -> m_head = 4:
//
//   Cycle N+1:
//     slot[4] = WB  (head, oldest) <- was IF, now retiring
//     slot[3] = MEM                <- was WB, now in MEM
//     slot[2] = EX                 <- was MEM
//     slot[1] = IS                 <- was EX
//     slot[0] = DE                 <- was IS
//     slot[5] = IF  (youngest)     <- was DE, new fetch here
//
// ============================================================================
// EXCEPTION AND FAULT HANDLING
// ============================================================================
//
// Faults are detected in multiple stages but committed only in WB:
//
//   stage_EX():  Detect alignment faults, TLB misses, access violations
//                Set slot.faultPending, slot.trapCode, slot.faultVA
//
//   stage_MEM(): Additional fault checks (if needed)
//                Fault flags propagate forward
//
//   stage_WB():  Check slot.faultPending
//                If fault: flush pipeline, enter exception handler
//                If no fault: commit stores and retire
//
// This ensures faulting instructions don't commit architectural state.
//
// ============================================================================
// BRANCH PREDICTION AND MISPREDICTION RECOVERY
// ============================================================================
//
// Branches are predicted in stage_IF():
//   - Static prediction or dynamic predictor
//   - Predicted target stored in slot.predictionTarget
//
// Branches are resolved in stage_EX():
//   - Compare actual target with predicted target
//   - If misprediction: flush stages 0-2 (IF, DE, IS)
//                       redirect PC to correct target
//
// Branch predictor updated in stage_WB():
//   - Only after branch commits (no speculative updates)
//
// ============================================================================

#ifndef ALPHA_PIPELINE_H
#define ALPHA_PIPELINE_H

#include <QVarLengthArray>
#include "coreLib/types_core.h"
#include "coreLib/LoggingMacros.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "machineLib/PipeLineSlot.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "CBoxLib/CBoxBase.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "coreLib/Axp_Attributes_core.h"
#include "EBoxLib/EBoxBase.h"
#include "FBoxLib/FBoxBase.h"
#include "PalBoxLib/PalBoxBase.h"
#include "MBoxLib_EV6/MBoxBase.h"
#include "coreLib/BoxRequest.h"
#include "coreLib/FetchResult.h"
#include "configLib/global_EmulatorSettings.h"
#include "coreLib/ExecTrace.h"
#include "coreLib/EXECTRACE_Macros.h"
#include "grainFactoryLib/iGrain_helper_inl.h"
#include "grainFactoryLib/grain_core.h"
#include "machineLib/PipeLineSlot_inl.h"

struct DecodedInstruction;
// Forward Declarations
class CBox;
class FaultDispatcher;
struct WriteEntry;


#ifdef EXECTRACE_ENABLED
#pragma message("EXECTRACE_ENABLED is DEFINED")
#else
#pragma message("EXECTRACE_ENABLED is NOT DEFINED")
#endif


// ============================================================================
// R31 TRACE COUNTERS - Per Specification Section 7
// ============================================================================

enum class R31CounterType : quint8
{
    DiscardedWrites = 0,
    DiscardedLinkWrites = 1,
    AtomicLDLToR31 = 2,      // Handled in memory stage
    AtomicSTCToR31 = 3,      // Handled in memory stage  
    PrefetchLoadsToR31 = 4,  // Handled in memory stage
    OperandConstraintViolations = 5  // Handled in decode/execute
};

// Per-CPU counters (declare in AlphaPipeline class)
static quint64 m_r31Counters[6];


// ============================================================================
// AlphaPipeline - 6-Stage In-Order Pipeline
// ============================================================================

class AlphaPipeline
{
public:
    static constexpr int STAGE_COUNT = 6; // IF, DE, IS, EX, MEM, WB
    static constexpr int STAGEWB     = 5;
    static constexpr int STAGEMEM    = 4;
    static constexpr int STAGEEX     = 3;
    static constexpr int STAGEIS     = 2;
    static constexpr int STAGEDE     = 1;
    static constexpr int STAGEIF     = 0;


    // ============================================================================
   // Updated flush() - commit before clearing
   // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto flush(const char* caller) -> void
    {
        // m_pending was already committed by stage_WB earlier this tick.
        // Clear defensively — should already be empty at this point.
       stage(m_head).m_pending = PendingCommit{};

#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_PIPELINE_FLUSH(m_cpuId, caller, m_iprGlobalMaster->h->pc);
#endif
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            stage(i).clear();
            stage(i).valid = false;
        }

        if (m_mBox)
        {
            m_mBox->clearMissStaging();
            m_mBox->clearIPRStaging();
        }
    }

private:
    quint64 m_cycleCount{0};



#pragma region Private Members
    FaultDispatcher* m_faultSink;								// Fault/exception reporting service

    // ====================================================================
    // Privilege/Mode Snapshot
    // ====================================================================
    // Some instructions (REI, CHME) change privilege level.
    // We snapshot the state BEFORE instruction executes to:
    //   1. Determine if AST delivery is allowed
    //   2. Detect privilege escalation/de-escalation

    quint8  preCM{0};        // Current Mode BEFORE instruction (kernel/user/etc.)
    quint8  preIPL{0};       // Interrupt Priority Level BEFORE instruction
    quint64 prePS{0};        // Processor Status BEFORE instruction


    // ====================================================================
    // Instruction Sequencing
    // ====================================================================

    quint64 m_nextSequence{0};			// Monotonic counter for instruction age
    quint64 m_instructionsRetired{0};		// Monotonic counter of instructions which have committed
    quint64 m_totalCycles{0};				// Monotonic counter of total cycles which have executed.

    // Incremented in stage_IF when instruction enters
    // Used to determine fault precedence (oldest first)

    bool anyEventPending{false}; // Is there a pending interrupt/trap/fault?
    /*
        Some instructions change IPL/CM (REI, CHME, etc.). We need to know:
        - **Before instruction:** Can we take AST?
        - **After instruction:** Did instruction change delivery conditions?
    */


    // ====================================================================
    // Fetch Interface (IBox -> Pipeline)
    // ====================================================================

    FetchResult m_pendingFetch;		// 1-deep buffer from IBox
    // IBox calls supplyFetchResult(fr) to populate
    // stage_IF() consumes it


    // ====================================================================
    // Pipeline Slots (Ring Buffer)
    // ====================================================================

    QVarLengthArray<PipelineSlot, STAGE_COUNT> m_slots; // Physical slots [0..5]
    int                                        m_head = 0;                                     // Logical "start" index
    // stage(0) = m_slots[(m_head+0) % 6]
    // stage(5) = m_slots[(m_head+5) % 6]

    CPUIdType m_cpuId;  // CPU identifier (for multi-core systems)


    // ====================================================================
    // Temporary Decoded Instruction (Reused)
    // ====================================================================
    // Avoids allocating a new DecodedInstruction on every decode
    // Note: This might not be needed if slots have their own di

    //DecodedInstruction di{};

    // AlphaPipeline does NOT own boxes.

    CBox*   m_cBox{nullptr};		// owned by AlphaCPU
    PalBox* m_palBox{nullptr};	// owned by AlphaCPU
    EBox*   m_eBox{nullptr};		// owned by IBox and latched - bindBoxInstance()
    FBox*   m_fBox{nullptr};		// owned by IBox and latched - bindBoxInstance()
    MBox*   m_mBox{nullptr};		// owned by IBox and latched - bindBoxInstance()

    GuestMemory*        m_guestMemory{nullptr};
    ReservationManager* m_reservationManager{nullptr};
    CPUStateView* m_iprGlobalMaster{ nullptr };

#pragma endregion Private Members

public:
    // ====================================================================
    // Constructor
    // ====================================================================
    AlphaPipeline(CPUIdType  cpuId, CBox* cbox,
                  MBox*      mbox,
                  EBox*      ebox,
                  FBox*      fbox,
                  PalBox*    palbox)
    // ReSharper disable once CppMemberInitializersOrder
        : m_cpuId(cpuId)
          , m_faultSink(&globalFaultDispatcher(cpuId))
          , m_cBox(cbox)
          , m_mBox(mbox)
          , m_eBox(ebox)
          , m_fBox(fbox)
          , m_palBox(palbox)
          , m_slots(STAGE_COUNT)  // Initialize slots array
          , m_guestMemory(&global_GuestMemory())
          , m_reservationManager(&globalReservationManager())
        , m_iprGlobalMaster(getCPUStateView(cpuId))
    {
        initializePipeline();
       
    }


    // ====================================================================
    // Frontend Stall Detection
    // ====================================================================
    // Returns true if IBox should NOT fetch new instructions
    // Reasons to stall frontend:
    //   1. Stage 0 (IF) still has unconsumed data
    //   2. Any stage is stalled (cache miss, resource conflict, etc.)

    AXP_HOT AXP_ALWAYS_INLINE auto isFrontendStalled() const -> bool
    {
        return stage(0).valid ||      // Slot 0 (IF) has unconsumed fetch
            stage(1).stalled ||    // Decode stalled (rare)
            stage(2).stalled ||    // Issue stalled (RAW hazard, scoreboard full)
            stage(3).stalled ||    // Execute stalled (multi-cycle op not done)
            stage(4).stalled ||    // Memory stalled (cache miss, TLB miss)
            stage(5).stalled;      // WB stalled (should be rare)
    }

    // ----------------------------------------------------------------
    // isPipelineStalled() - Check if ANY stage is stalled
    // ----------------------------------------------------------------
    AXP_HOT AXP_ALWAYS_INLINE auto isPipelineStalled() const -> bool
    {
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            if (stage(i).stalled) return true;
        }
        return false;
    }


    // ============================================================================
// deferWriteback() - Capture instruction result for next-cycle commit
// ============================================================================
// Called at the END of stage_EX after grain->execute() completes.
// Stores the result in m_pending for commitPending() next cycle.
//
// Handles three writeback paths:
//   1. Link register (BSR/JSR): Ra = PC + 4
//   2. Integer ALU/Load result: destReg = payLoad
//   3. Float result:            destReg = payLoad

    AXP_HOT AXP_ALWAYS_INLINE auto deferWriteback(PipelineSlot& slot) noexcept -> void
    {
        // -----------------------------------------------------------
        // Path 1: Branch-with-link (BSR, JSR, JSR_COROUTINE)
        //   Destination: Ra receives return address (PC + 4)
        // -----------------------------------------------------------
        if (slot.di.semantics & S_BranchWriteLink)
        {
            const quint8 ra = slot.di.ra;
            if (ra != 31)
            {
                slot.m_pending.intValid = true;
                slot.m_pending.intReg = ra;
                slot.m_pending.intValue = slot.di.pc + 4;
                slot.m_pending.intClearDirty = true;
#if AXP_INSTRUMENTATION_TRACE
                slot.m_pending.destReg = ra;
                slot.m_pending.instrPC = slot.di.pc;
                slot.m_pending.value = slot.di.pc + 4;
#endif
            }
            return;
        }

        // -----------------------------------------------------------
        // Path 2 & 3: ALU / Load / Float result
        //   Destination: Rc (operate), Ra (load), Fc (float)
        //   Value: slot.payLoad (computed by EX stage)
        // -----------------------------------------------------------
        if (slot.needsWriteback)
        {
            const quint8 reg = destRegister(slot.di);
            if (reg == 31) return;          // R31/F31 hardwired zero

#if AXP_INSTRUMENTATION_TRACE
            slot.m_pending.destReg = reg;
            slot.m_pending.instrPC = slot.di.pc;
            slot.m_pending.value = slot.payLoad;
#endif

            if (destIsFloat(slot.di))
            {
                slot.m_pending.fpValid = true;
                slot.m_pending.fpReg = reg;
                slot.m_pending.fpValue = slot.payLoad;
                slot.m_pending.fpClearDirty = true;
            }
            else
            {
                slot.m_pending.intValid = true;
                slot.m_pending.intReg = reg;
                slot.m_pending.intValue = slot.payLoad;
                slot.m_pending.intClearDirty = true;
            }
        }
    }
    // ============================================================================
    // Assembly Format Helper (for debugging/tracing)
    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE auto formatAssembly(PipelineSlot& slot) noexcept -> QString
    {
        QString mnem   = slot.grain ? slot.grain->mnemonic() : "UNKNOWN";
        quint8  opcode = extractOpcode(slot.di.rawBits());

        // Branch format (BR, BSR, BEQ, etc.)
        if (isBranchFormat(slot.di))
        {
            qint32  disp   = extractBranchDisplacement(slot.di.rawBits());
            quint64 target = slot.di.pc + 4 + (disp << 2);
            return QString("%1 R%2, 0x%3")
                   .arg(mnem, -6)  // Left-aligned, 6 chars
                   .arg(slot.di.ra)
                   .arg(target, 16, 16, QChar('0'));
        }

        // Memory format (LDQ, STQ, LDA, etc.)
        if (isMemoryFormat(slot.di))
        {
            qint16 disp = extractMemDisp(slot.di.rawBits());
            return QString("%1 R%2, %3(R%4)")
                   .arg(mnem, -6)
                   .arg(slot.di.ra)
                   .arg(disp)
                   .arg(slot.di.rb);
        }

        // Operate format (ADDQ, SUBQ, etc.)
        if (isOperateFormat(slot.di))
        {
            bool isLiteral = (slot.di.rawBits() & (1 << 12)) != 0;
            if (isLiteral)
            {
                quint8 lit = extractLiteral(slot.di.rawBits());
                return QString("%1 R%2, #%3, R%4")
                       .arg(mnem, -6)
                       .arg(slot.di.ra)
                       .arg(lit)
                       .arg(slot.di.rc);
            }
            else
            {
                return QString("%1 R%2, R%3, R%4")
                       .arg(mnem, -6)
                       .arg(slot.di.ra)
                       .arg(slot.di.rb)
                       .arg(slot.di.rc);
            }
        }

        // Jump format (JMP, JSR, RET)
        if (opcode == 0x1A)
        {
            return QString("%1 R%2, (R%3)")
                   .arg(mnem, -6)
                   .arg(slot.di.ra)
                   .arg(slot.di.rb);
        }

        // PAL format (CALL_PAL, HW_REI, etc.)
        if (opcode == 0x00 || opcode == 0x1E)
        {
            quint8 func = getFunctionCode(slot.di);
            return QString("%1 0x%2")
                   .arg(mnem, -6)
                   .arg(func, 2, 16, QChar('0'));
        }

        // Default - just mnemonic
        return mnem;
    }


    // ====================================================================
    // PRIMARY INTERFACE
    // ====================================================================


    AXP_HOT AXP_ALWAYS_INLINE auto execute(FetchResult& fetchResult) noexcept -> BoxResult
    {
        PipelineStepResult result{};
        fetchResult.pipelineStepResult        = result;
        fetchResult.pipelineStepResult.action = PipelineAction::ADVANCED;
        // ================================================================
        // Execute stages from OLDEST to YOUNGEST (reverse order)
        // ================================================================

        stage_WB(fetchResult.pipelineStepResult);  // Stage 5: Write back (OLDEST instruction in pipeline)
        debugStageExit("WB", stage(STAGEWB));
        if (fetchResult.pipelineStepResult.action != PipelineAction::ADVANCED)
        {
            if (fetchResult.pipelineStepResult.action == PipelineAction::FAULT)
            {

                return BoxResult()
                       .faultDispatched()
                       .setFaultDispatched();
            }
            if (fetchResult.pipelineStepResult.action == PipelineAction::PAL_CALL)
            {
                return BoxResult().requestEnterPalMode();
            }
        }
        stage_MEM(); // Stage 4: Memory access
        debugStageExit("MEM", stage(STAGEMEM));
        // Check if EX-stage grain requested a flush (PAL side-effects)
        if (stage(STAGEEX).flushPipeline)
        {
            // Flush younger stages (IF, DE, IS)
            for (int i = 0; i < STAGEEX; i++)
            {
                stage(i).valid = false;
                stage(i).clear();
            }
            fetchResult.pipelineStepResult.action = PipelineAction::ADVANCED;
            return BoxResult().flushPipeline();
        }

        if (faultDetected())
        {
            fetchResult.pipelineStepResult = PipelineStepResult::Fault(
                stage(4).trapCode,
                stage(4).faultVA,
                stage(4).di.pc
            );
            return BoxResult().faultDispatched();
        }
        stage_EX();  // Stage 3: Execute (ALU, FPU, branch resolution)
        debugStageExit("EX", stage(STAGEEX));
        // Check if EX-stage grain requested a flush (PAL side-effects)
        if (stage(STAGEEX).flushPipeline)
        {
            // Flush younger stages (IF, DE, IS)
            for (int i = 0; i < STAGEEX; i++)
            {
                stage(i).valid = false;
                stage(i).clear();
            }
            fetchResult.pipelineStepResult.action = PipelineAction::ADVANCED;
            return BoxResult().flushPipeline();
        }


        if (faultDetected())
        {
            BoxResult faultedBoxResult;
            fetchResult.pipelineStepResult = PipelineStepResult::Fault(
                stage(3).trapCode,
                stage(3).faultVA,
                stage(3).di.pc
            );

            if (fetchResult.pipelineStepResult.action == PipelineAction::FAULT)
            {
                //sets BOTH the flag AND the fault details
                faultedBoxResult.setFaultInfo(
                    fetchResult.pipelineStepResult.trapCode,   // Sets faultClass
                    fetchResult.pipelineStepResult.faultPC,    // Sets faultingPC
                    fetchResult.pipelineStepResult.faultVA     // Sets faultingVA
                );
            }

            return faultedBoxResult;
        }
        // NOTE: stage_IS NOOP - hazard checks now in boxes -
        //       stage_IS - Just increment the stage for now.
        // ================================================================
        stage_IS();  // Stage 2: Issue (hazard check, scoreboard allocation)
        debugStageExit("IS", stage(STAGEIS));
        stage_DE();  // Stage 1: Decode (mostly NOOP - IBox already decoded)
        debugStageExit("DE", stage(STAGEDE));
        stage_IF();  // Stage 0: Instruction fetch (consume m_pendingFetch)
        debugStageExit("IF", stage(STAGEIF));

        // ================================================================
        // Check for stalls
        // ================================================================

        if (isPipelineStalled())
        {
            fetchResult.pipelineStepResult = PipelineStepResult::Stalled();
            return BoxResult().stallPipeline();  // Don't advance ring
        }

        // ================================================================
        // Check for branch misprediction
        // ================================================================

        PipelineSlot& wbSlot = stage(5);  // stage_WB - we should 
        // if (wbSlot.valid && isBranchFormat(wbSlot.di)) {			// is a Branch Instruction Grain
        // 	if (wbSlot.predictionValid && (wbSlot.predictionTaken != wbSlot.branchTaken)) {
        // 		result.action = PipelineAction::MISPREDICTION;
        // 		result.redirectPC = wbSlot.branchTarget;
        // 		flush();  // Clear all younger instructions
        //
        // 		fetchResult.pipelineStepResult = PipelineStepResult::Mispredict(wbSlot.branchTarget);
        // 		return BoxResult().misPredictBranchTarget();
        // 	}
        // }

        // ================================================================
        // All clear - advance ring
        // ================================================================

        //  advance the current slot after exit.
        fetchResult.pipelineStepResult = PipelineStepResult::Advanced();
        return BoxResult().advance();
    }

    // ====================================================================
    // Fetch Supply Interface (IBox -> Pipeline)
    // ====================================================================
    // IBox calls this to provide a fetched/decoded instruction
    // Pipeline stores it in m_pendingFetch (1-deep buffer)
    // Next stage_IF() will consume it

    auto supplyFetchResult(const FetchResult& fr) -> void
    {
        m_pendingFetch = fr;  // Simple assignment (copy)
    }


    // ====================================================================
    // Execution Box Accessors (Singleton Pattern)
    // ====================================================================

    // Passed in by AlphaCPU


    AXP_HOT AXP_ALWAYS_INLINE auto injectOtherBoxes(EBox* eBox, FBox* fBox, MBox* mBox, PalBox* pBox,
                                                    CBox* cBox) noexcept -> void
    {
        m_eBox   = eBox;
        m_fBox   = fBox;
        m_mBox   = mBox;
        m_palBox = pBox;
        m_cBox   = cBox;
    }

    // ========================================================================
    // PIPELINE EXECUTION
    // ========================================================================
    /**
     * @brief Execute one pipeline cycle
     *
     * Advances all 6 stages in order: WB -> MEM -> EX -> IS -> DE -> IF
     * Then rotates ring buffer (advanceRing).
     *
     * Critical ordering for RAW hazard resolution:
     *   1. stage_MEM() writes registers
     *   2. stage_EX() reads registers (same cycle, sees updated values)
     *
     * @return PipelineStepResult indicating pipeline state/actions
     */
    AXP_HOT AXP_FLATTEN auto tick(FetchResult& fetchResult) noexcept -> BoxResult
    {
        // Supply fetch result to pipeline
        supplyFetchResult(fetchResult);

        debugTickStart(m_cycleCount);
        //   debugPipelineSummary();  // Show pipeline state at start of cycle

        // Execute pipeline stages
        BoxResult result = execute(fetchResult);

        advanceRing();  // Rotate the ring buffer!
        if (m_cycleCount % 500 == 0)
        {
            debugPipelineSummary();
        }

        m_cycleCount++;

        return result;
    }

    auto initializePipeline() noexcept -> void
    {
        // Get reference to physical slot 0
        PipelineSlot& slot = stage(0);

        // BIND BOX POINTERS - ONCE, FOREVER
        // Repeat for all physical slots
        for (int i = 0; i < STAGE_COUNT; ++i)
        {
            stage(i).injectOtherBoxes(m_eBox, m_fBox, m_mBox, m_palBox, m_cBox);
            m_slots[i].cpuId     = m_cpuId;
            m_slots[i].m_faultDispatcher = m_faultSink;
        }

        ExecTrace::setFormat("asm");
        INFO_LOG(QString("AlphaPipeline CPU %1: All %2 slots initialized with box references")
            .arg(m_cpuId).arg(STAGE_COUNT));
    }

private:
    // ====================================================================
    // STAGE IMPLEMENTATIONS
    // ====================================================================

    /**
        * @brief Stage 0: Instruction Fetch
        *
        * Responsibilities:
        *   - Fetch 32-bit instruction from PA
        *   - Decode opcode, lookup grain
        *   - Predict branch target
        *   - Advance PC
        *
        * Output:
        *   - slot.di: DecodedInstruction with grain reference
        *   - slot.valid = true
        */
    AXP_HOT AXP_ALWAYS_INLINE auto stage_IF() -> void
    {
        PipelineSlot& slot = stage(0);
        slot.clear();

        if (!m_pendingFetch.valid)
        {
            return;
        }

        // Populate slot
        slot.di           = m_pendingFetch.di;
        slot.grain        = m_pendingFetch.di.grain;
        slot.valid        = true;
        slot.slotSequence = m_nextSequence++;
        slot.stage        = PipelineStage::IFetch;
        slot.currentStage = 0;
        slot.cpuId        = m_cpuId;
        slot.m_faultDispatcher    = m_faultSink;

        // Copy prediction info
        slot.predictionValid  = m_pendingFetch.predictedValid;
        slot.predictionTaken  = m_pendingFetch.predictedTaken;
        slot.predictionTarget = m_pendingFetch.predictedTarget;


        // ================================================================
        // PC ADVANCEMENT - Use PREDICTION
        // ================================================================
        quint64 nextPC;
        quint64 target = 0;


        // Check if ANY control flow instruction (uses existing helpers!)
        if (slot.di.semantics & S_BranchFmt)
        {
            // Unconditional branch prediction (BR/BSR): always taken, target known in IF
            if (slot.di.semantics & S_Uncond)
            {
                const qint32  disp21 = extractDisp21(slot.di.rawBits());
                const quint64 target = (slot.di.pc + 4) + (static_cast<qint64>(disp21) << 2);

                slot.predictionValid  = true;
                slot.predictionTaken  = true;
                slot.predictionTarget = target;

                nextPC = target;
            }
            else
            {
                // Conditional branch: simplest policy = predict not taken
                slot.predictionValid  = true;            // keep true so later stages know prediction policy was applied
                slot.predictionTaken  = false;
                slot.predictionTarget = slot.di.pc + 4; // "not taken" target = fall-through

                nextPC = slot.di.pc + 4;
            }
        }
        else
        {
            // Not a branch: sequential
            nextPC = slot.di.pc + 4;
        }

        slot.cycle       = m_cycleCount;
        slot.predictedPC = slot.predictionTarget;

        m_iprGlobalMaster->h->pc = nextPC;
        //setPC_Active(slot.cpuId, nextPC);
        m_pendingFetch = FetchResult{};

        qDebug().noquote()
            << QString("IF: PC=%1 RAW=%2 OP=%3 MNE=%4 disp21=%5 tgt=%6 pred(V=%7,T=%8,PC=%9) nextPC=%10")
               .arg(hx64(slot.di.pc))
               .arg(hx32(slot.di.rawBits()))
               .arg(hx8(getOpcodeFromPacked(slot.di)))
               .arg(getMnemonicFromRaw(slot.di.rawBits()))
               .arg(extractDisp21(slot.di.rawBits()))
               .arg(hx64(target))
               .arg(slot.predictionValid ? 1 : 0)
               .arg(slot.predictionTaken ? 1 : 0)
               .arg(hx64(slot.predictionTarget))
               .arg(hx64(nextPC));
    }

    /**
    * @brief Stage 1: Decode
    *
    * Responsibilities:
    *   - Mostly pass-through (grain decoded in IF)
    *   - Future: Early hazard detection
    */
    AXP_HOT AXP_ALWAYS_INLINE auto stage_DE() -> void
    {
        PipelineSlot& slot = stage(1);
        if (!slot.valid) return;

        slot.stage        = PipelineStage::Decode;
        slot.currentStage = 1;
        // extract the box from the decodedInstruction - it will be referenced later
        if (slot.grain != nullptr)
        {
            quint16 opCode = getOpcodeFromPacked(slot.di);
            switch (executionBoxDecoder(opCode))
            {
            case ExecutionBox::EBox:
            case ExecutionBox::IBox:
                slot.execUnit = ExecUnit::EBOX;
                break;
            case ExecutionBox::MBox:
                slot.execUnit = ExecUnit::MBOX;
                break;
            case ExecutionBox::FBox:
                slot.execUnit = ExecUnit::FBOX;
                break;
            case ExecutionBox::CBox:
                slot.execUnit = ExecUnit::CBOX;
                break;
            case ExecutionBox::HWBox:
                slot.execUnit = ExecUnit::PALBOX;
                break;
            case ExecutionBox::VBox:
                break;
            case ExecutionBox::IBoxOnly:
                break;
            case ExecutionBox::PalBox:
                break;
            case ExecutionBox::Unknown:
                break;
            default:
                slot.execUnit = ExecUnit::None;
                break;
            }
        }
    }

    /**
     * @brief Stage 2: Instruction Slot
     *
     * Responsibilities:
     *   - Prepare instruction for execution
     *   - Future: Dual-issue logic
     */
    AXP_HOT AXP_ALWAYS_INLINE auto stage_IS() noexcept -> void // report the stall up to IBox to return and 
    {
        // refetch the same PC and rerun the cycle
        // The stall will last indefinitely until all other instruction have completed.


        PipelineSlot& slot = stage(2); // IS stage
        if (!slot.valid || slot.stalled)
        {
            qDebug().noquote()
                << QString("IS: STALL PC=%1 MNE=%2 reason=%3 valid=%4 stalled=%5 dual=%6")
                   .arg(hx64(slot.di.pc))
                   .arg(getMnemonicFromRaw(slot.di.rawBits()))
                   .arg(static_cast<quint8>(slot.trapCode))  // make this a small enum->string helper
                   .arg(slot.valid ? 1 : 0)
                   .arg(slot.stalled ? 1 : 0)
                   .arg(slot.dualIssued ? 1 : 0);

            return;
        }
        slot.currentStage = 2;
    }

    /**
     * @brief Stage 3: Execute
     *
     * Responsibilities:
     *   - Read source registers
     *   - Execute instruction logic (via grain->execute())
     *   - Calculate memory addresses
     *   - Detect branch mispredictions
     *   - Set flags: needsWriteback, writeRa, branchTaken, etc.
     *   - Defer register writeback to m_pending (committed next cycle)
     *
     * Output:
     *   - slot.payLoad: Result value for writeback
     *   - slot.va: Virtual address (for memory ops)
     *   - slot.branchTaken: Branch outcome
     *
     * DOES NOT:
     *   - Write to the register file (deferred to commitPending next cycle)
     */
    AXP_HOT AXP_ALWAYS_INLINE auto stage_EX() -> void
    {
        PipelineSlot& slot = stage(STAGEEX);

        // ================================================================
        // EARLY EXITS
        // ================================================================
        if (!slot.valid || slot.stalled || slot.faultPending)
            return;

        debugStageTransition("FETCH", "EXECUTE", slot.di.pc, true);
        debugExecutionEntry(slot.di);

        // ================================================================
        // ILLEGAL INSTRUCTION CHECK
        // ================================================================
        if (!slot.grain)
        {
            slot.faultPending = true;
            slot.trapCode     = TrapCode_Class::ILLEGAL_INSTRUCTION;
            quint8  opcCode   = extractOpcode(slot.di.rawBits());
            quint16 fcCode    = getFunctionCode(slot.di);
            DEBUG_LOG(QString("Illegal Instruction opc: %1 fc: %2 seq: %3")
                .arg(opcCode).arg(fcCode).arg(slot.slotSequence));
#if AXP_INSTRUMENTATION_TRACE
            EXECTRACE_DISCARD_PENDING(                       //  Instrumentation Trace
                m_cpuId,
                DiscardReason::FAULT,
                slot.m_pending.isValid() ? slot.m_pending.instrPC : 0);
#endif
            return;  // No deferral — faulting instruction produces no result
        }

        // ================================================================
        // EXECUTE THE GRAIN
        // ================================================================
        slot.pcReason = PipelineSlot::PCReason::Sequential;
        slot.nextPC   = slot.di.pc + 4;
        slot.grain->execute(slot);

#if AXP_INSTRUMENTATION_TRACE

        if (slot.faultPending) {
            EXECTRACE_FAULT_RAISED(m_cpuId,                  
                static_cast<quint8>(slot.trapCode),
                slot.faultVA,
                slot.di.pc,
                PipelineStage_enum::EX);
        }

#endif

        // ================================================================
        // BRANCH / JUMP MISPREDICTION DETECTION
        // ================================================================
        const quint8 opcCode        = extractOpcode(slot.di.rawBits());
        const bool   isBranchOrJump = isBranchOpcodeFamily(opcCode) ||
            isJumpOpcodeFamily(opcCode);

        if (isBranchOrJump && slot.branchTaken)
        {
            const quint64 actualTarget    = slot.branchTarget;
            const quint64 predictedTarget = slot.predictionTarget;
            const bool    wasPredicted    = slot.predictionValid;

            bool mispredicted = false;

            if (!wasPredicted)
            {
                mispredicted = true;
                DEBUG_LOG(QString("Branch @0x%1: No prediction, actually taken -> MISPREDICTION")
                    .arg(slot.di.pc, 16, 16, QChar('0')));
            }
            else if (actualTarget != predictedTarget)
            {
                mispredicted = true;
                DEBUG_LOG(QString("Branch @0x%1: Target misprediction! Predicted: 0x%2, Actual: 0x%3")
                    .arg(slot.di.pc, 16, 16, QChar('0'))
                    .arg(predictedTarget, 16, 16, QChar('0'))
                    .arg(actualTarget, 16, 16, QChar('0')));
            }

            if (mispredicted)
            {
                // Flush younger instructions (IF, DE, IS — stages 0-2)
                for (int i = 0; i < STAGEEX; i++)
                {
                    stage(i).valid = false;
                    stage(i).clear();
                }
                slot.mispredict = true;
                m_pendingFetch  = FetchResult{};
                m_iprGlobalMaster->h->pc     = actualTarget;

                DEBUG_LOG(QString("FLUSHED pipeline and redirected PC -> 0x%1")
                    .arg(actualTarget, 16, 16, QChar('0')));
            }

            if (slot.m_cBox)
                slot.m_cBox->updatePrediction(slot.di.pc, true, actualTarget);
        }
        else if (isBranchOrJump && !slot.branchTaken)
        {
            const quint64 fallThrough    = slot.di.pc + 4;
            const bool    predictedTaken = slot.predictionTaken;

            if (predictedTaken)
            {
                DEBUG_LOG(QString("Branch @0x%1: Predicted taken, actually not taken -> MISPREDICTION")
                    .arg(slot.di.pc, 16, 16, QChar('0')));

                for (int i = 0; i < STAGEEX; i++)
                {
                    stage(i).valid = false;
                    stage(i).clear();
                }

                m_iprGlobalMaster->h->pc = fallThrough;

                DEBUG_LOG(QString("FLUSHED pipeline and redirected PC -> 0x%1 (fall-through)")
                    .arg(fallThrough, 16, 16, QChar('0')));
            }

            if (slot.m_cBox)
                slot.m_cBox->updatePrediction(slot.di.pc, false, fallThrough);
        }

        if (!isBranchOrJump)
        {
            slot.nextPC   = slot.di.pc + 4;
            slot.pcReason = PipelineSlot::PCReason::Sequential;
        }

        // ================================================================
        // DEFER REGISTER WRITEBACK
        // ================================================================
        // Result stored in m_pending — committed at top of next cycle's
        // stage_WB via commitPending(). This ensures the NEXT instruction
        // in EX reads the correct value without forwarding or stalls.
        //
        // If this instruction faulted, we already returned above and
        // deferWriteback() is never called — no corrupt state.
        // ================================================================

        if (!slot.faultPending)
            deferWriteback(slot);

        slot.stage        = PipelineStage::Execute;
        slot.currentStage = STAGEEX;
    }

    /**
       * @brief Stage 4: Memory Access + Register Writeback
       *
       * CRITICAL: ALL REGISTER WRITES HAPPEN IN THIS STAGE!
       *
       * This design enables RAW hazard resolution through intra-cycle ordering.
       * Since tick() executes stages sequentially:
       *   1. stage_MEM() writes registers
       *   2. stage_EX() reads registers (same cycle, updated values visible)
       *
       * Register Writeback Actions:
       *   - Write link registers (BSR, JSR): Ra = PC+4
       *   - Write integer results: Ra or Rc = payLoad
       *   - Write float results: Fa or Fc = payLoad
       *   - Clear dirty bits
       *
       * Memory Actions:
       *   - Complete memory loads (data -> payLoad)
       *   - Translate store addresses (don't commit yet)
       *   - Check alignment
       *
       * Does NOT:
       *  - Write to the register file (handled by deferred writeback)
       *  - Commit stores (handled in WB)
       *  ============================================================================
       */
    AXP_HOT AXP_ALWAYS_INLINE auto stage_MEM() -> void
    {
        PipelineSlot& slot = stage(STAGEMEM);
        if (!slot.valid) return;

        // ================================================================
        // STALL CONDITIONS
        // ================================================================
        if (slot.needsMemoryBarrier && !slot.memoryBarrierCompleted)
        {
            slot.stalled = true;
            return;
        }

        if (slot.needsWriteBufferDrain && !slot.writeBufferDrained)
        {
            slot.stalled = true;
            return;
        }
#if AXP_INSTRUMENTATION_TRACE

        if (faultDetected()) {
            EXECTRACE_FAULT_RAISED(m_cpuId,                 
                static_cast<quint8>(stage(4).trapCode),
                stage(4).faultVA,
                stage(4).di.pc,
                PipelineStage_enum::MEM);
        }

#endif
        // ================================================================
        // MEMORY OPERATIONS
        // ================================================================
        // Loads:  Data already in slot.payLoad (grain executed load in EX).
        //         Register write happens via deferred writeback path.
        //
        // Stores: Address computed in EX, stored in slot.pa/slot.va.
        //         Actual memory write deferred to stage_WB after fault check.
        //
        // No register writes here — the deferred writeback from stage_EX
        // is committed at the top of stage_WB next cycle.
        // ================================================================

        slot.stage        = PipelineStage::MemAccess;
        slot.currentStage = STAGEMEM;
    }

    // ========================================================================
    // PIPELINE STAGES (executed in tick() order)
    // ========================================================================

    /**
     * @brief Stage 5: Write Back / Retirement
     *
     * Execution order within stage_WB:
     *   1. commitPending()  - Write deferred register result (from EX last cycle)
     *   2. Fault check      - If fault, flush and return (no store commit)
     *   3. CALL_PAL check   - If PAL, transfer and return
     *   4. Store commit     - Write store data to memory
     *   5. Branch predictor - Update prediction tables
     *   6. Retirement       - Mark instruction as architecturally committed
     *   7. Cleanup          - Clear slot
     *
     * The pending is from a **younger** instruction (it was in EX last cycle, WB is 2 stages ahead). 
     * 
     * @param result Output parameter for pipeline actions
     * ============================================================================
     */

    AXP_HOT AXP_ALWAYS_INLINE auto stage_WB(PipelineStepResult& result) -> void
    {
        PipelineSlot& slot = stage(STAGEWB);

        // ================================================================
        // EMPTY SLOT — commit pending and return
        // ================================================================
        // Pending is from an instruction that passed EX without faulting.
        // WB is empty (pipeline filling or post-flush), safe to commit.
        if (!slot.valid)
        {
            commitPending(slot);
            return;
        }

        // ================================================================
        // 1. FAULT CHECK — discard pending (younger instruction)
        // ================================================================
        if (slot.faultPending)
        {
            slot.m_pending = PendingCommit{};    // younger instruction squashed
            result.action = PipelineAction::FAULT;
            result.trapCode = slot.trapCode;
            result.faultVA = slot.faultVA;
            result.faultPC = slot.di.pc;
            slot.valid = false;

#if AXP_INSTRUMENTATION_TRACE
            // Instrumentation Tracing
            EXECTRACE_DISCARD_PENDING(                       //  Fault Path
                m_cpuId,
                DiscardReason::FAULT,
                slot.m_pending.isValid() ? slot.m_pending.instrPC : 0);

            EXECTRACE_FAULT_DISPATCHED(m_cpuId,             
                static_cast<quint8>(slot.trapCode),
                slot.faultVA,
                slot.di.pc);
#endif
            
            return;
        }


        // ================================================================
        // 2. CALL_PAL — discard pending (pipeline serializes)
        // ================================================================
        if (isCallPal(slot.di))
        {
#if AXP_INSTRUMENTATION_TRACE
            slot.m_pending.instrPC = slot.di.pc;
            slot.m_pending.destReg = 0xFF;       // sentinel: no register write
            slot.m_pending.value = result.palVector;  // where control goes
            EXECTRACE_DISCARD_PENDING(                       // 
                m_cpuId,
                DiscardReason::PAL_CALL,
                slot.m_pending.isValid() ? slot.m_pending.instrPC : 0);

#endif
            slot.m_pending = PendingCommit{};    // pipeline about to be drained
            result.action = PipelineAction::PAL_CALL;
            result.palFunction = palFunction(slot.di.rawBits());
            result.callPC = slot.di.pc;
            result.palVector = m_iprGlobalMaster->computeCallPalEntry( result.palFunction);
            slot.valid = false;
            return;
        }

        // ================================================================
        // 3. NORMAL — WB instruction passed, pending is safe to commit
        // ================================================================
        commitPending(slot);

        // ================================================================
        // 4. MEMORY STORE COMMIT
        // ================================================================
        if (slot.di.semantics & S_Store)
        {
         MEM_STATUS memStat =    m_guestMemory->write64(slot.pa, slot.payLoad);
            m_reservationManager->breakReservationsOnCacheLine(slot.pa);
        }

        // ================================================================
        // 5. BRANCH PREDICTION UPDATE
        // ================================================================
        if (slot.branchTaken)
        {
            m_cBox->updatePrediction(slot.di.pc, slot.branchTaken, slot.branchTarget);
        }

        // ================================================================
        // 6. RETIREMENT
        // ================================================================
        commitInstruction(slot);

        slot.valid = false;
        slot.clear();
    }


    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    /**
     * @brief Commit instruction (mark as retired)
     *
     * Called in stage_WB after all hazards cleared.
     * Updates performance counters, traces execution.
     */
    AXP_HOT AXP_ALWAYS_INLINE auto commitInstruction(PipelineSlot& slot) noexcept -> void
    {
        // Mark instruction as architecturally committed
        m_instructionsRetired++;

        // Update performance counters
        m_totalCycles = m_cycleCount;

        // Trace execution (if enabled)
        debugRetirement(slot.di, true /*success */);
        //ExecTraceCommitSlot(slot);
        EXECTRACE_WB_RETIRE(slot);
        // That's it! Register already written in MEM stage
    }

    // ====================================================================
    // CONDITION CHECKS
    // ====================================================================

    /**
     * @brief Check if any slot has fault pending
     */
    auto detectFaultInAnySlot(int& faultingSlotIndex) const -> bool
    {
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            if (stage(i).faultPending)
            {
                faultingSlotIndex = i;
                return true;
            }
        }
        return false;
    }

    // these boxes are owned by AlphaCPU and will be accessed by reference
    auto getEBox() const noexcept -> EBox* { return m_eBox; }
    auto getFBox() const noexcept -> FBox* { return m_fBox; }
    auto getMBox() const noexcept -> MBox* { return m_mBox; }
    auto getCBox() const noexcept -> CBox* { return m_cBox; }
    auto getPBox() const noexcept -> PalBox* { return m_palBox; }

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Print pipeline state visualization
     *
     * Shows all 6 stages with instruction mnemonics, PCs, and writeback info.
     * Marks head position with "-->" indicator.
     *
     * Example output:
     *   [PIPELINE] CYCLE 00006 - CPU 0  |  HEAD=4
     *   [ 5 ][WB ] ADDQ     @ 0x0000000020008004 -> R25
     *   [-->][MEM] LDA      @ 0x0000000020008008 -> R26
     *   [ 3 ][EX ] LDQ      @ 0x000000002000800C -> R1
     *   [ 2 ][IS ] SUBQ     @ 0x0000000020008010 -> R30
     *   [ 1 ][DE ] STQ      @ 0x0000000020008014 -> MEM
     *   [ 0 ][IF ] STQ      @ 0x0000000020008018 -> MEM
     */
    AXP_HOT AXP_ALWAYS_INLINE auto debugPipelineSummary() const noexcept -> void
    {
#ifdef AXP_DEBUG_PIPELINE
        qDebug() << "================================================================================";
        qDebug() << QString("[PIPELINE] CYCLE %1 - CPU %2  |  HEAD=%3")
                    .arg(m_cycleCount, 5, 10, QChar('0'))
                    .arg(m_cpuId)
                    .arg(m_head);
        qDebug() << "================================================================================";

        for (int stageIdx = 5; stageIdx >= 0; --stageIdx)
        {
            const PipelineSlot& slot = stage(stageIdx);

            const char* stageName = nullptr;
            switch (stageIdx)
            {
            case 5: stageName = "WB ";
                break;
            case 4: stageName = "MEM";
                break;
            case 3: stageName = "EX ";
                break;
            case 2: stageName = "IS ";
                break;
            case 1: stageName = "DE ";
                break;
            case 0: stageName = "IF ";
                break;
            default: ;
            }

            QString stageMarker = (stageIdx == m_head) ? "-->" : QString("%1").arg(stageIdx);

            QString mnemonic = slot.valid
                                   ? QString(slot.di.grain->mnemonic()).leftJustified(8, ' ')
                                   : QString("EMPTY   ");

            // Add writeback flag indicator
            QString wbFlag = "";
            if (slot.valid)
            {
                if (slot.needsWriteback && slot.writeRa)
                {
                    wbFlag = QString(" → R%1").arg(slot.di.ra);
                }
                else if (slot.di.semantics & S_Store)
                {
                    wbFlag = " → MEM";
                }
            }

            qDebug() << QString("[%1][%2] %3 @ 0x%4%5")
                        .arg(stageMarker, 3)
                        .arg(stageName)
                        .arg(mnemonic)
                        .arg(slot.di.pc, 16, 16, QChar('0'))
                        .arg(wbFlag);
        }

        qDebug() << QString("[   ][PC] Next: 0x%1")
            .arg(m_iprGlobalMaster->h->pc, 16, 16, QChar('0'));
        qDebug() << "================================================================================";
#endif
    }


    auto debugTickStart(quint64 cycle) const noexcept -> void
    {
#ifdef AXP_DEBUG_PIPELINE
        qDebug() << "================================================================================";
        qDebug() << "[PIPELINE] CYCLE" << cycle << "START - CPU" << m_cpuId;
        qDebug() << "================================================================================";
#else
        Q_UNUSED(cycle);
#endif
    }

private:
    AXP_HOT AXP_ALWAYS_INLINE auto commitPending(PipelineSlot& slot) noexcept -> void
    {
        if (slot.m_pending.intValid)
        {

#if AXP_INSTRUMENTATION_TRACE
            EXECTRACE_COMMIT_PENDING(                            // Instrumentation Trace
                m_cpuId,
                slot.m_pending.destReg,
                slot.m_pending.value,
                slot.m_pending.instrPC);
#endif

            slot.writeIntReg(slot.m_pending.intReg, slot.m_pending.intValue);
            if (slot.m_pending.intClearDirty)
                m_eBox->clearDirty(slot.m_pending.intReg);
            slot.m_pending.intValid = false;
        }
        if (slot.m_pending.fpValid)
        {
            slot.writeFpReg(slot.m_pending.fpReg, slot.m_pending.fpValue);
            if (slot.m_pending.fpClearDirty)
                m_fBox->clearDirty(slot.m_pending.fpReg);
            slot.m_pending.fpValid = false;
        }
    }


    // ====================================================================
    // RING BUFFER ACCESSORS
    // ====================================================================
    // These functions abstract the ring buffer indirection
    // stage(N) always returns the slot at logical stage N, regardless of
    // where it is physically stored in m_slots[]

    // Const version (read-only access)

    AXP_HOT AXP_ALWAYS_INLINE auto stage(int logicalIndex) const noexcept -> const PipelineSlot&
    {
        // Calculate physical index using modulo arithmetic
        // Example: m_head=2, logicalIndex=0 (IF stage)
        //   -> physical index = (2 + 0) % 6 = 2
        //   -> returns m_slots[2]
        return m_slots[(m_head - logicalIndex + STAGE_COUNT) % STAGE_COUNT];
    }

    // ========================================================================
    // RING BUFFER ACCESS
    // ========================================================================

    /**
     * @brief Access pipeline stage by logical index
     *
     * Maps logical stage index to physical ring buffer slot:
     *   stage(5) = slot[m_head]           (WB, oldest)
     *   stage(4) = slot[(m_head-1+6) % 6] (MEM)
     *   stage(3) = slot[(m_head-2+6) % 6] (EX)
     *   stage(2) = slot[(m_head-3+6) % 6] (IS)
     *   stage(1) = slot[(m_head-4+6) % 6] (DE)
     *   stage(0) = slot[(m_head-5+6) % 6] (IF, youngest)
     *
     * @param stageIndex Logical stage (0=IF, 1=DE, 2=IS, 3=EX, 4=MEM, 5=WB)
     * @return Reference to pipeline slot
     */
    AXP_HOT AXP_ALWAYS_INLINE auto stage(int logicalIndex) noexcept -> PipelineSlot&
    {
        return m_slots[(m_head - logicalIndex + STAGE_COUNT) % STAGE_COUNT];
    }

    /**
     * @brief Advance ring buffer (rotate head pointer)
     *
     * Called at end of tick() after all stages execute.
     * Increments m_head: m_head = (m_head + 1) % 6
     *
     * Effect: Each instruction flows forward one stage
     */
    AXP_HOT AXP_ALWAYS_INLINE auto advanceRing() noexcept -> void
    {
        m_head = (m_head + 1) % STAGE_COUNT;
    }

    // ====================================================================
    // FAULT HANDLING
    // ====================================================================
    // ----------------------------------------------------------------
    // faultDetected() - Check if ANY slot has a fault
    // ----------------------------------------------------------------
    AXP_HOT AXP_ALWAYS_INLINE auto faultDetected() const -> bool
    {
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            if (stage(i).faultPending) return true;
        }
        return false;
    }


    // ----------------------------------------------------------------
    // flushYoungerSlots() - Clear all instructions younger than fault
    // ----------------------------------------------------------------
    // Preserves older instructions (they may have already committed)
    // Clears younger instructions (they must not affect architectural state)
    //
    // Why? Precise exception semantics require:
    //   - All instructions older than fault must complete
    //   - All instructions younger than fault must be discarded

    AXP_HOT AXP_ALWAYS_INLINE auto flushYoungerSlots(int faultingPhysicalIndex) -> void
    {
        // Calculate logical age of faulting instruction
        int faultAge = (faultingPhysicalIndex - m_head + STAGE_COUNT) % STAGE_COUNT;

        // Flush all younger instructions (age < faultAge)
        for (int age = 0; age < faultAge; age++)
        {
            int physicalIndex = (m_head + age) % STAGE_COUNT;
            m_slots[physicalIndex].clear();  // Total clear
        }
    }


    // Check if instructions are on aligned quadword boundary
    AXP_HOT AXP_ALWAYS_INLINE auto isQuadwordAligned(quint64 pc1, quint64 pc2) const -> bool
    {
        return (pc1 & 0xFFFFFFFFFFFFFFF8ULL) == (pc2 & 0xFFFFFFFFFFFFFFF8ULL);
    }

    AXP_HOT AXP_ALWAYS_INLINE auto evaluateBranchCondition(const DecodedInstruction& di, quint64 regValue) -> bool
    {
        // Branch opcodes:
        // 0x30 = BR (unconditional)
        // 0x31 = BSR (unconditional)
        // 0x38 = BLBC (branch if low bit clear)
        // 0x39 = BEQ (branch if equal zero)
        // 0x3A = BLT (branch if less than zero)
        // 0x3B = BLE (branch if less than or equal zero)
        // 0x3C = BLBS (branch if low bit set)
        // 0x3D = BNE (branch if not equal zero)
        // 0x3E = BGE (branch if greater than or equal zero)
        // 0x3F = BGT (branch if greater than zero)

        quint32 raw = getRaw(di);
        switch (extractOpcode(raw))
        {
        case 0x30: // BR
        case 0x34: // BSR
            return true;

        case 0x38: // BLBC
            return (regValue & 1) == 0;

        case 0x39: // BEQ
            return regValue == 0;

        case 0x3A: // BLT
            return static_cast<qint64>(regValue) < 0;

        case 0x3B: // BLE
            return static_cast<qint64>(regValue) <= 0;

        case 0x3C: // BLBS
            return (regValue & 1) != 0;

        case 0x3D: // BNE
            return regValue != 0;

        case 0x3E: // BGE
            return static_cast<qint64>(regValue) >= 0;

        case 0x3F: // BGT
            return static_cast<qint64>(regValue) > 0;

        default:
            return false;
        }
    }


    AXP_HOT AXP_ALWAYS_INLINE auto evaluateBranchCondition(PipelineSlot& slot) const noexcept -> bool
    {
        const auto& di = slot.di;

        // Unconditional branch
        if (isBranchFormat(di))
            return true;

        // Conditional branch uses integer register result
        // slot.resultInt holds RA or RB depending on DI
        const quint64 rv = slot.payLoad;

        switch (getBranchCode(getOpcodeFromPacked(di)))
        {
        case BranchCode::BEQ: return (rv == 0);
        case BranchCode::BNE: return (rv != 0);
        case BranchCode::BLT: return (static_cast<qint64>(rv) < 0);
        case BranchCode::BGE: return (static_cast<qint64>(rv) >= 0);
        case BranchCode::BLE: return (static_cast<qint64>(rv) <= 0);
        case BranchCode::BGT: return (static_cast<qint64>(rv) > 0);

        // BLBC, BLBS use bit tests
        case BranchCode::BLBC: return ((rv & 1) == 0);
        case BranchCode::BLBS: return ((rv & 1) != 0);

        // Floating branches use fp compare result passed in slot.resultInt
        case BranchCode::FBEQ: return (rv == 0);
        case BranchCode::FBNE: return (rv != 0);
        case BranchCode::FBLT: return (rv < 0);
        case BranchCode::FBGE: return (rv >= 0);
        case BranchCode::FBLE: return (rv <= 0);
        case BranchCode::FBGT: return (rv > 0);

        default:
            return false;
        }
    }

    // ========================================================================
    // Dual Issue Logic
    // ========================================================================

    // ========================================================================
    // Execution unit classification helper
    // ========================================================================
    // Group execution boxes into resource classes so dual-issue logic can
    // enforce "different physical units" rather than just different enums.
    //
    // Reference: Alpha AXP implementation issue rules (EV4/EV5 integer +
    // floating-point units can execute in parallel; loads/stores share the
    // address/data path).

    AXP_HOT AXP_ALWAYS_INLINE auto execUnitClass(ExecutionBox box) const noexcept -> quint8
    {
        switch (box)
        {
        case ExecutionBox::EBox:
        case ExecutionBox::IBox:
            // Integer / branch cluster
            return 0;

        case ExecutionBox::MBox:
            // Address / memory cluster
            return 1;

        case ExecutionBox::FBox:
            // Floating-point cluster
            return 2;

        default:
            // PAL / misc / unknown treat conservatively as its own class
            return 3;
        }
    }

    // Two instructions can only dual-issue if they use *different* resource
    // classes (e.g., integer + FP, or integer + memory), not the same one.

    AXP_HOT AXP_ALWAYS_INLINE auto targetDifferentUnits(ExecutionBox a, ExecutionBox b) const noexcept -> bool
    {
        // Same class -> contend for same hardware -> cannot dual-issue
        return execUnitClass(a) != execUnitClass(b);
    }

    // ========================================================================
    // Dual Issue Logic
    // ========================================================================
    //
    // Check if two instructions can be dual-issued in the same cycle.
    //
    // Rules (simplified, EV4/EV5-style):
    //  - The two instructions must be 8-byte (quadword) aligned as a pair.
    //  - They must target different execution unit *classes*
    //    (integer vs FP vs memory).
    //  - The second unit must be available this cycle.
    //  - No RAW/WAW hazards between the pair.
    //
    // Reference: Alpha AXP Architecture / implementation notes on
    // super-scalar issue (integer + FP, or integer + memory).
    // =========================================================================

    AXP_HOT AXP_ALWAYS_INLINE auto canDualIssue(const DecodedInstruction& di1,
                                                const DecodedInstruction& di2) const -> bool
    {
        // 1) Must be on the same quadword boundary (PC pair alignment)
        if (!isQuadwordAligned(di1.pc, di2.pc))
        {
            return false;
        }

        quint16 opCode1 = getOpcodeFromPacked(di1);
        quint16 opCode2 = getOpcodeFromPacked(di2);
        // 2) Must target different execution unit classes
        const ExecutionBox box1 = executionBoxDecoder(opCode1);
        const ExecutionBox box2 = executionBoxDecoder(opCode2);

        if (!targetDifferentUnits(box1, box2))
        {
            return false;
        }

        // 3) Second unit must be available
        if (!isUnitAvailable(di2))
        {
            return false;
        }

        // 4) Check register dependencies
        //
        // RAW: di2 reads what di1 writes
        if (di1.rc == di2.ra || di1.rc == di2.rb)
        {
            return false;
        }

        // WAW: both write the same register (ignore R31)
        if (di1.rc == di2.rc && di1.rc != 31)
        {
            return false;
        }

        return true;
    }

    auto flushAndReturn() -> void
    {
        int faultingSlot = findFaultingSlot();
        flushYoungerSlots(faultingSlot);
    }

    auto findFaultingSlot() const -> int
    {
        for (int i = 0; i < 6; i++)
        {
            if (stage(i).faultPending) return i;
        }
        return -1;
    }


    AXP_HOT AXP_ALWAYS_INLINE auto clearStagedMemoryOps() noexcept -> void
    {
        // Iterate through all pipeline slots in the circular queue  
        for (int i = 0; i < STAGE_COUNT; ++i)
        {
            PipelineSlot slot = stage(i);

            if (slot.valid)
            {
                // ====================================================================
                // 1. Clear PTE/TLB Cache State
                // ====================================================================
                // Moved to MBox

                // ====================================================================
                // 2. Clear Address State
                // ====================================================================
                slot.va           = 0;          // <- Clear virtual address
                slot.pa           = 0;          // <- Clear physical address  
                slot.physicalAddr = 0;          // <- Clear physical address (duplicate field?)
                slot.outPAData    = 0;          // <- Clear PA output data
                slot.faultVA      = 0;          // <- Clear fault VA

                // ====================================================================
                // 3. Clear Memory Operation State  
                // ====================================================================
                slot.memResultValid         = false;  // <- Clear memory result validity
                slot.memoryBarrierCompleted = false;  // <- Clear barrier state
                slot.writeBufferDrained     = false;  // <- Clear write buffer state

                // ====================================================================
                // 4. Clear Fault State (if memory-related)
                // ====================================================================
                if (isMemoryFormat(slot.di))
                {
                    slot.faultPending = false;          // <- Clear memory faults
                    slot.stalled      = false;               // <- Clear memory stalls
                }

                DEBUG_LOG(QString("CPU %1: Cleared staged memory ops for slot %2 (stage=%3)")
                    .arg(m_cpuId).arg(i).arg(static_cast<int>(slot.stage)));
            }
        }

        DEBUG_LOG(QString("CPU %1: Cleared all staged memory operations in pipeline").arg(m_cpuId));
    }

    /*
        Scans all slots
        Returns index of faulting slot
        Returns false if no faults
    */
    AXP_HOT AXP_ALWAYS_INLINE auto detectFaultInAnySlot(int& faultingSlotIndex) -> bool
    {
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            if (stage(i).faultPending)
            {
                faultingSlotIndex = i;
                return true;
            }
        }
        return false;
    }


    AXP_HOT AXP_ALWAYS_INLINE auto logicalAge(int physicalIndex) const -> int
    {
        return (physicalIndex - m_head + STAGE_COUNT) % STAGE_COUNT;
    }


#pragma region Hazards

    // No stalls in any slot
    // At least one instruction moved or retired
    // All conditions must be true

    AXP_HOT AXP_ALWAYS_INLINE auto canAdvanceRing() const -> bool
    {
        // No faults
        if (faultDetected()) return false;

        // No stalls
        for (int i = 0; i < STAGE_COUNT; i++)
        {
            if (stage(i).stalled) return false;
        }

        // Forward progress occurred
        //if (!forwardProgress) return false;

        return true;
    }


    // Issue / dual-issue hazard logic


    // Stage 3: Issue Logic (checks resources and dual-issue)
    // Returns: 0 = stall, 1 = single issue, 2 = dual issue
    AXP_HOT AXP_ALWAYS_INLINE auto tryIssue(const DecodedInstruction& di1, const DecodedInstruction& di2) const -> int
    {
        // Check if first instruction can issue
        if (!isUnitAvailable(di1))
        {
            return 0;  // Stall - first instruction can't issue
        }

        // First instruction can issue - try dual issue
        if (canDualIssue(di1, di2))
        {
            return 2;  // Dual issue both
        }

        return 1;  // Single issue first only
    }


    // Check if execution unit is available
    AXP_HOT AXP_ALWAYS_INLINE auto isUnitAvailable(const DecodedInstruction& di) const -> bool
    {
        quint16 opCode = getOpcodeFromPacked(di);
        switch (executionBoxDecoder(opCode))
        {
        case ExecutionBox::EBox:
            return !m_eBox->isBusy();
        case ExecutionBox::IBox:
        //	return !IBox::instance()->isBusy();
        case ExecutionBox::MBox:
            return !m_mBox->isBusy();
        case ExecutionBox::FBox:
            return !m_fBox->isBusy();
        default:
            return true;
        }
    }

    // Serialization / barrier hazards

    AXP_HOT AXP_ALWAYS_INLINE auto checkBarrierRelease(PipelineSlot& slot) noexcept -> void
    {
        if (slot.serializeType == SerializationType::Barrier_EXC)
        {
            // Check if prior stages are clear
            bool allClear = true;
            for (int j = 0; j < slot.currentStage; ++j)
            {
                if (stage(j).valid) allClear = false;
            }

            if (allClear && !slot.m_faultDispatcher->eventPending())
            {
                slot.stalled = false;  // Release!
            }
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto advance(PipelineSlot& slot) const -> void
    {
        if (slot.needsWriteBufferDrain)
        {
            m_cBox->drainWriteBuffers(&slot);
            slot.needsWriteBufferDrain = false;
        }

        if (slot.needsMemoryBarrier)
        {
            m_cBox->RequestMemoryBarrier(slot, slot.barrierKind);
            slot.needsMemoryBarrier = false;
        }
    }

    AXP_HOT AXP_ALWAYS_INLINE auto drainWriteBuffers(PipelineSlot& slot) const noexcept -> void
    {
        // Drain all buffers for the specific execution unit
        switch (slot.execUnit)
        {
        case ExecUnit::EBOX:
        case ExecUnit::IBOX:
        case ExecUnit::MBOX:
        case ExecUnit::FBOX:
        case ExecUnit::CBOX:
            m_cBox->drainWriteBuffers(&slot);
            break;

        case ExecUnit::PALBOX:
        default:
            // No write buffers to drain for this unit
            break;
        case ExecUnit::None:
            break;
        }
    }

#pragma endregion Hazards
};

#endif // ALPHA_PIPELINE_H
