// ============================================================================
//  EA_Calculation_Helpers.h
//
//  Purpose: Effective Address calculation with automatic fault handling
//
//  Eliminates code duplication across memory instruction execution
// ============================================================================

#ifndef EA_CALCULATION_HELPERS_H
#define EA_CALCULATION_HELPERS_H

#include "../pteLib/calculateEffectiveAddress.h"
#include "../EBoxLib/VA_core.h"
#include "../machineLib/PipelineSlot.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "../exceptionLib/ExceptionFactory.h"
#include "../faultLib/PendingEvent_Refined.h"
#include "../coreLib/types_core.h"

// ============================================================================
// WRAPPER: Calculate EA with Automatic Fault Handling
// ============================================================================

/**
 * @brief Calculate effective address with automatic exception handling.
 *
 * Wrapper around calculateEffectiveAddress() that automatically creates
 * and dispatches exceptions on failure (unaligned, invalid instruction).
 *
 * Use this instead of raw calculateEffectiveAddress() in memory instructions.
 *
 * @param slot Pipeline slot (for instruction, fault sink)
 * @param ctx CPU context
 * @param[out] va Output virtual address (only valid if returns true)
 * @return true if EA calculation succeeded, false if fault raised
 */
AXP_FLATTEN bool calculateEA_withFaultHandling( PipelineSlot& slot,   quint64& va) noexcept
{
    CPUIdType cpuId = slot.apc()->cpuId();
    // Try to calculate EA
    if (!calculateEffectiveAddress(slot , va)) {
        // EA calculation failed - determine fault type

        AccessKind accessKind = isStore(slot.di) ?
            AccessKind::WRITE : AccessKind::READ;

        // Check if it's an alignment fault
        if (!ev6CheckAlignment(va, accessKind)) {
            // Unaligned access
            auto ev = makeUnalignedEvent(
                cpuId,
                va,
                accessKind == AccessKind::WRITE
            );
            slot.faultSink->setPendingEvent(ev);
            slot.faultPending = true;
        }
        else {
            // Invalid instruction or register encoding
            auto ev = makeIllegalOpcodeEvent(
                cpuId,
                slot.di.pc,
                slot.di.grain->rawBits
            );
            slot.faultSink->setPendingEvent(ev);
            slot.faultPending = true;
        }

        slot.needsWriteback = false;
        return false;  // Fault raised
    }

    return true;  // Success - va is valid
}


// ============================================================================
// SIMPLIFIED USAGE IN MEMORY INSTRUCTIONS
// ============================================================================

/*
// BEFORE (20+ lines of boilerplate):

quint64 va;
if (!calculateEffectiveAddress(slot.di, m_ctx, va)) {
    AccessKind accessKind = isStore(slot.di) ?
        AccessKind::WRITE : AccessKind::READ;
    if (!ev6CheckAlignment(va, accessKind)) {
        auto ev = makeUnalignedEvent(
            m_ctx->cpuId(),
            va,
            accessKind == AccessKind::WRITE
        );
        slot.faultSink->setPendingEvent(ev);
    }
    else {
        auto ev = makeIllegalOpcodeEvent(
            m_ctx->cpuId(),
            slot.di.pc,
            slot.di.grain->rawBits
        );
        slot.faultSink->setPendingEvent(ev);
    }
    slot.needsWriteback = false;
    return;
}


// AFTER (2 lines):

quint64 va;
if (!calculateEA_withFaultHandling(slot, m_ctx, va)) {
    return;  // Fault already handled
}

// Continue with memory operation...
*/


// ============================================================================
// VARIANT: For Instructions That Don't Need Fault Sink
// ============================================================================

/**
 * @brief Calculate EA with fault handling (without slot).
 *
 * Use when you have DecodedInstruction but not a full PipelineSlot.
 *
 * @param di Decoded instruction
 * @param ctx CPU context
 * @param faultSink Fault sink for exception dispatch
 * @param[out] va Output virtual address
 * @return true if successful, false if fault raised
 */
AXP_FLATTEN bool calculateEA_withFaultHandling2(  PipelineSlot &slot,   quint64& va) noexcept
{
    CPUIdType cpuId = slot.apc()->cpuId();
    if (!calculateEffectiveAddress(slot, va)) {
        AccessKind accessKind = isStore(slot.di) ?
            AccessKind::WRITE : AccessKind::READ;

        if (!ev6CheckAlignment(va, accessKind)) {
            auto ev = makeUnalignedEvent(
                cpuId,
                va,
                accessKind == AccessKind::WRITE
            );
            slot.faultSink->setPendingEvent(ev);
            slot.faultPending = true;
        }
        else {
            auto ev = makeIllegalOpcodeEvent(
                cpuId,
                slot.di.pc,
                slot.di.grain->rawBits  // Use di.instruction instead of grain
            );
            slot.faultSink->setPendingEvent(ev);
            slot.faultPending = true;
        }

        return false;
    }

    return true;
}


// ============================================================================
// VARIANT: Return Exception Instead of Dispatching
// ============================================================================

/**
 * @brief Calculate EA and return exception (don't dispatch).
 *
 * Use when caller wants to handle exception differently.
 *
 * @param slot Pipeline slot
 * @param ctx CPU context
 * @param[out] va Output virtual address
 * @param[out] exception Output exception (only valid if returns false)
 * @return true if successful, false if exception created
 */
AXP_FLATTEN bool calculateEA_returnException( PipelineSlot& slot,  quint64& va,  PendingEvent& exception) noexcept
{
    CPUIdType cpuId = slot.apc()->cpuId();
    if (!calculateEffectiveAddress(slot, va)) {
        AccessKind accessKind = isStore(slot.di) ?
            AccessKind::WRITE : AccessKind::READ;

        if (!ev6CheckAlignment(va, accessKind)) {
            exception = makeUnalignedEvent(
                cpuId,
                va,
                accessKind == AccessKind::WRITE
            );
        }
        else {
            exception = makeIllegalOpcodeEvent(
                cpuId,
                slot.di.pc,
                slot.di.grain->rawBits
            );
        }

        return false;
    }

    return true;
}

#endif // EA_CALCULATION_HELPERS_H


// ============================================================================
// EXAMPLE USAGE IN LOAD/STORE INSTRUCTIONS
// ============================================================================

/*
// ============================================================================
// LDQ (Load Quadword)
// ============================================================================

void executeLDQ(PipelineSlot& slot, AlphaProcessorContext* ctx)
{
    // Calculate EA with automatic fault handling
    quint64 va;
    if (!calculateEA_withFaultHandling(slot, ctx, va)) {
        return;  // Exception already dispatched
    }

    // Translate VA -> PA
    quint64 pa;
    AlphaPTE pte{};
    TranslationResult tr = ev6TranslateFastVA(
        ctx, va, AccessKind::READ,
        static_cast<Mode_Privilege>(getCM_Active(ctx->cpuId())),
        pa, &pte
    );

    if (tr != TranslationResult::Success) {
        auto ev = makeTranslationFault(ctx->cpuId(), va, tr, false);
        slot.faultSink->setPendingEvent(ev);
        return;
    }

    // Read from memory
    quint64 value;
    if (global_GuestMemory().read64(pa, value) != MEM_STATUS::Ok) {
        auto ev = makeMemoryFault(ctx->cpuId(), va);
        slot.faultSink->setPendingEvent(ev);
        return;
    }

    // Write to register
    ctx->writeIntReg(slot.di.Ra, value);
}


// ============================================================================
// STQ (Store Quadword)
// ============================================================================

void executeSTQ(PipelineSlot& slot, AlphaProcessorContext* ctx)
{
    // Calculate EA with automatic fault handling
    quint64 va;
    if (!calculateEA_withFaultHandling(slot, ctx, va)) {
        return;  // Exception already dispatched
    }

    // Get value from register
    quint64 value = ctx->readIntReg(slot.di.Ra);

    // Translate VA -> PA
    quint64 pa;
    AlphaPTE pte{};
    TranslationResult tr = ev6TranslateFastVA(
        ctx, va, AccessKind::WRITE,
        static_cast<Mode_Privilege>(getCM_Active(ctx->cpuId())),
        pa, &pte
    );

    if (tr != TranslationResult::Success) {
        auto ev = makeTranslationFault(ctx->cpuId(), va, tr, true);
        slot.faultSink->setPendingEvent(ev);
        return;
    }

    // Write to memory
    if (global_GuestMemory().write64(pa, value) != MEM_STATUS::Ok) {
        auto ev = makeMemoryFault(ctx->cpuId(), va);
        slot.faultSink->setPendingEvent(ev);
        return;
    }
}
*/