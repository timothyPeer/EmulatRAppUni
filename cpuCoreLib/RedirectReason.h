#ifndef REDIRECTREASON_H
#define REDIRECTREASON_H
// ============================================================================
// RedirectReason.h
// ============================================================================
// Pipeline Control Flow Redirect Reasons
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================



#include "../coreLib/Axp_Attributes_core.h"
#include <QtGlobal>

/**
 * @brief Reasons for pipeline control flow redirection
 * 
 * RedirectReason classifies why the CPU pipeline needs to redirect execution
 * to a different PC. This is used by CBox (control flow coordinator) and
 * AlphaCPU to handle various control flow events.
 * 
 * Redirects can be:
 * - Architectural (branches, jumps, calls, returns, traps)
 * - Microarchitectural (mispredictions, pipeline flushes)
 * - System-level (interrupts, exceptions, PAL entry/exit)
 * 
 * @note CBox initiates most redirects, but some come from MBox (memory faults),
 *       FBox (FP exceptions), or PalBox (PAL entry/exit).
 */
enum class RedirectReason : quint8
{
    // ========================================================================
    // No Redirect
    // ========================================================================
    None = 0,

    // ========================================================================
    // Branch Redirects (CBox)
    // ========================================================================
    
    /**
     * @brief Branch misprediction detected
     * 
     * The branch predictor predicted incorrectly, and the pipeline must
     * redirect to the correct target.
     * 
     * Metadata: metadata1 = correct target PC, metadata2 = prediction info
     * Source: CBox::resolveBranch_EBox()
     */
    BranchMisprediction,

    /**
     * @brief Conditional branch taken
     * 
     * Used when branch prediction is disabled and branch resolves as taken.
     * 
     * Metadata: metadata1 = target PC
     * Source: CBox branch execution (BEQ, BNE, BLT, etc.)
     */
    BranchTaken,

    /**
     * @brief Unconditional branch
     * 
     * BR, BSR instructions always redirect.
     * 
     * Metadata: metadata1 = target PC, metadata2 = link register value (for BSR)
     * Source: CBox::executeBR(), CBox::executeBSR()
     */
    UnconditionalBranch,

    // ========================================================================
    // Jump Redirects (CBox)
    // ========================================================================
    
    /**
     * @brief JMP instruction
     * 
     * Unconditional computed jump.
     * 
     * Metadata: metadata1 = target PC (from register)
     * Source: CBox::executeJMP()
     */
    Jump,

    /**
     * @brief JSR instruction (jump to subroutine)
     * 
     * Subroutine call with link register update.
     * 
     * Metadata: metadata1 = target PC, metadata2 = return address
     * Source: CBox::executeJSR()
     */
    JumpSubroutine,

    /**
     * @brief RET instruction (return from subroutine)
     * 
     * Return using predicted or actual return address stack.
     * 
     * Metadata: metadata1 = return PC (from register/RAS)
     * Source: CBox::executeRET()
     */
    Return,

    /**
     * @brief JSR_COROUTINE instruction
     * 
     * Coroutine context switch (special JSR variant).
     * 
     * Metadata: metadata1 = target PC, metadata2 = coroutine state
     * Source: CBox::executeJSR_COROUTINE()
     */
    Coroutine,

    // ========================================================================
    // PAL Mode Transitions (PalBox)
    // ========================================================================
    
    /**
     * @brief CALL_PAL instruction
     * 
     * Enter PAL mode via CALL_PAL instruction.
     * 
     * Metadata: metadata1 = PAL function code (0-255), metadata2 = call PC
     * Source: PalBox::executeCallPal()
     */
    PALEntry,

    /**
     * @brief HW_REI instruction (return from PAL)
     * 
     * Exit PAL mode and restore context.
     * 
     * Metadata: metadata1 = return PC (from EXC_ADDR), metadata2 = restored PS
     * Source: PalBox::executeREI()
     */
    PALReturn,

    // ========================================================================
    // Exception/Trap Redirects (MBox, FBox, EBox, IBox)
    // ========================================================================
    
    /**
     * @brief Synchronous trap (precise exception)
     * 
     * Architectural exception caused by instruction execution.
     * 
     * Metadata: metadata1 = ExceptionClass_EV6, metadata2 = faulting PC
     * Source: Any box (EBox for arithmetic, MBox for memory, etc.)
     */
    Trap,

    /**
     * @brief Asynchronous interrupt
     * 
     * External hardware interrupt.
     * 
     * Metadata: metadata1 = interrupt vector, metadata2 = IPL
     * Source: IRQ controller → AlphaCPU::handleInterrupt()
     */
    Interrupt,

    /**
     * @brief Machine check exception
     * 
     * Fatal hardware error requiring immediate PAL entry.
     * 
     * Metadata: metadata1 = MCHK code, metadata2 = error address
     * Source: System hardware monitoring
     */
    MachineCheck,

    // ========================================================================
    // Memory Management Redirects (MBox)
    // ========================================================================
    
    /**
     * @brief ITB miss (instruction TLB miss)
     * 
     * Instruction fetch missed in ITB, PAL must fill.
     * 
     * Metadata: metadata1 = virtual address, metadata2 = 0
     * Source: MBox translation
     */
    ITBMiss,

    /**
     * @brief DTB miss (data TLB miss)
     * 
     * Data access missed in DTB, PAL must fill.
     * 
     * Metadata: metadata1 = virtual address, metadata2 = access type
     * Source: MBox translation
     */
    DTBMiss,

    // ========================================================================
    // Pipeline Control (CBox, IBox)
    // ========================================================================
    
    /**
     * @brief Pipeline flush requested
     * 
     * Full pipeline flush (e.g., for serialization, MB, TRAPB).
     * 
     * Metadata: metadata1 = flush PC, metadata2 = flush reason code
     * Source: CBox::executeMB(), CBox::executeTRAPB()
     */
    PipelineFlush,

    /**
     * @brief Context switch
     * 
     * ASN/process context switch requires pipeline flush.
     * 
     * Metadata: metadata1 = new PC, metadata2 = new ASN
     * Source: PalBox (SWPCTX)
     */
    ContextSwitch,

    /**
     * @brief System reset
     * 
     * Complete system reset.
     * 
     * Metadata: metadata1 = reset vector, metadata2 = reset type
     * Source: System controller
     */
    Reset,

    // ========================================================================
    // Special Cases
    // ========================================================================
    
    /**
     * @brief Debugger breakpoint
     * 
     * Software breakpoint hit.
     * 
     * Metadata: metadata1 = breakpoint PC, metadata2 = breakpoint ID
     * Source: Debug subsystem
     */
    Breakpoint,

    /**
     * @brief Single-step debug
     * 
     * Single-step execution for debugging.
     * 
     * Metadata: metadata1 = next PC
     * Source: Debug subsystem
     */
    SingleStep,

    /**
     * @brief CPU halt
     * 
     * CPU entering halt state (CALL_PAL HALT).
     * 
     * Metadata: metadata1 = halt code, metadata2 = halt PC
     * Source: PalBox::halt()
     */
    Halt
};

/**
 * @brief Get human-readable name for redirect reason
 * 
 * @param reason Redirect reason
 * @return const char* String name
 */
static constexpr const char* getRedirectReasonName(RedirectReason reason) noexcept
{
    switch (reason)
    {
        case RedirectReason::None:                  return "None";
        case RedirectReason::BranchMisprediction:   return "BranchMisprediction";
        case RedirectReason::BranchTaken:           return "BranchTaken";
        case RedirectReason::UnconditionalBranch:   return "UnconditionalBranch";
        case RedirectReason::Jump:                  return "Jump";
        case RedirectReason::JumpSubroutine:        return "JumpSubroutine";
        case RedirectReason::Return:                return "Return";
        case RedirectReason::Coroutine:             return "Coroutine";
        case RedirectReason::PALEntry:              return "PALEntry";
        case RedirectReason::PALReturn:             return "PALReturn";
        case RedirectReason::Trap:                  return "Trap";
        case RedirectReason::Interrupt:             return "Interrupt";
        case RedirectReason::MachineCheck:          return "MachineCheck";
        case RedirectReason::ITBMiss:               return "ITBMiss";
        case RedirectReason::DTBMiss:               return "DTBMiss";
        case RedirectReason::PipelineFlush:         return "PipelineFlush";
        case RedirectReason::ContextSwitch:         return "ContextSwitch";
        case RedirectReason::Reset:                 return "Reset";
        case RedirectReason::Breakpoint:            return "Breakpoint";
        case RedirectReason::SingleStep:            return "SingleStep";
        case RedirectReason::Halt:                  return "Halt";
        default:                                    return "Unknown";
    }
}

/**
 * @brief Check if redirect requires pipeline flush
 * 
 * @param reason Redirect reason
 * @return bool true if full pipeline flush required
 */
static AXP_HOT AXP_ALWAYS_INLINE bool requiresPipelineFlush(RedirectReason reason) noexcept
{
    switch (reason)
    {
        // Always flush
        case RedirectReason::PALEntry:
        case RedirectReason::PALReturn:
        case RedirectReason::Trap:
        case RedirectReason::Interrupt:
        case RedirectReason::MachineCheck:
        case RedirectReason::ITBMiss:
        case RedirectReason::DTBMiss:
        case RedirectReason::PipelineFlush:
        case RedirectReason::ContextSwitch:
        case RedirectReason::Reset:
        case RedirectReason::Halt:
            return true;

        // Selective flush (discard instructions after branch point)
        case RedirectReason::BranchMisprediction:
        case RedirectReason::BranchTaken:
        case RedirectReason::UnconditionalBranch:
        case RedirectReason::Jump:
        case RedirectReason::JumpSubroutine:
        case RedirectReason::Return:
        case RedirectReason::Coroutine:
            return true;  // In-order Alpha always flushes on redirect

        // No flush
        case RedirectReason::None:
        default:
            return false;
    }
}

/**
 * @brief Check if redirect originated from CBox
 * 
 * @param reason Redirect reason
 * @return bool true if CBox-initiated redirect
 */
static AXP_HOT AXP_ALWAYS_INLINE bool isCBoxRedirect(RedirectReason reason) noexcept
{
    switch (reason)
    {
        case RedirectReason::BranchMisprediction:
        case RedirectReason::BranchTaken:
        case RedirectReason::UnconditionalBranch:
        case RedirectReason::Jump:
        case RedirectReason::JumpSubroutine:
        case RedirectReason::Return:
        case RedirectReason::Coroutine:
        case RedirectReason::PipelineFlush:
            return true;

        default:
            return false;
    }
}

#endif // REDIRECTREASON_H

// ============================================================================
// Usage Examples
// ============================================================================

/*

// Example 1: CBox branch misprediction
void CBox::resolveBranch_EBox(quint64 pc, bool taken, quint64 target) noexcept
{
    BranchResolutionResult result = m_branchPredictor.resolve(pc, taken, target);
    
    if (result.mispredicted) {
        // Signal redirect to AlphaCPU
        requestRedirect(RedirectReason::BranchMisprediction, result.correctTarget, pc);
    }
}

// Example 2: PalBox CALL_PAL
BoxResult PalBox::executeCallPal(const PipelineSlot& slot) noexcept
{
    quint8 palFunction = extractFunction(slot.di.rawBits());
    
    return BoxResult()
        .requestRedirect(RedirectReason::PALEntry, palFunction, slot.di.pc + 4);
}

// Example 3: AlphaCPU redirect handler (your existing code, enhanced)
void AlphaCPU::handleRedirect(RedirectReason reason, quint64 metadata1, quint64 metadata2) noexcept
{
    quint64 vectorPC = 0;
    quint64 currentPC = getPC_Active(m_cpuId);
    
    switch (reason) {
        case RedirectReason::PALEntry:
            // metadata1 = palFunction, metadata2 = callPC
            vectorPC = computePalEntryPC(m_cpuId, static_cast<quint8>(metadata1));
            enterPalMode(PalEntryReason::CALL_PAL_INSTRUCTION, vectorPC, currentPC);
            break;
            
        case RedirectReason::Trap:
            // metadata1 = ExceptionClass_EV6
            vectorPC = computeTrapVector(static_cast<ExceptionClass_EV6>(metadata1));
            enterPalMode(PalEntryReason::TRAP, vectorPC, currentPC);
            break;
            
        case RedirectReason::Interrupt:
            // metadata1 = interrupt vector
            vectorPC = computeInterruptVector(static_cast<quint32>(metadata1));
            enterPalMode(PalEntryReason::INTERRUPT, vectorPC, currentPC);
            break;
            
        case RedirectReason::BranchMisprediction:
        case RedirectReason::BranchTaken:
        case RedirectReason::Jump:
        case RedirectReason::Return:
            // metadata1 = target PC
            setPC_Active(m_cpuId, metadata1);
            break;
            
        case RedirectReason::PALReturn:
            // metadata1 = return PC
            restoreCompleteContext(m_cpuId, m_savedContext);
            break;
            
        default:
            WARN_LOG(QString("CPU %1: Unknown redirect reason: %2")
                .arg(m_cpuId)
                .arg(getRedirectReasonName(reason)));
            return;
    }
    
    // Flush pipeline if needed
    if (requiresPipelineFlush(reason)) {
        m_alphaPipeline->flush();
    }
    
    DEBUG_LOG(QString("CPU %1: Redirect - %2 -> PC=0x%3")
        .arg(m_cpuId)
        .arg(getRedirectReasonName(reason))
        .arg(vectorPC ? vectorPC : metadata1, 16, 16, QChar('0')));
}

// Example 4: CBox unconditional branch
void CBox::executeBR(PipelineSlot& slot) noexcept
{
    quint64 target = calculateBranchTarget(slot.di);
    
    // Mark redirect
    slot.redirect = true;
    slot.redirectReason = RedirectReason::UnconditionalBranch;
    slot.redirectTarget = target;
}

*/