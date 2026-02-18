#ifndef IGRAIN_SEMANICEXAMPLES_H
#define IGRAIN_SEMANICEXAMPLES_H
/*
 * // ============================================================================
// INTEGRATION EXAMPLE - Using 32-byte DecodedInstruction
// ============================================================================
// Shows complete decode-execute pipeline integration
// ============================================================================

#include "DecodedInstruction_Optimized.h"
#include "iGrain-DualLookup_inl.h"
#include "GrainResolver.h"
#include "AlphaProcessorContext.h"

// ============================================================================
// Example 1: Decode and Cache
// ============================================================================

void example_DecodeAndCache(quint32 rawInstruction, quint64 pc,
                             DecodedInstruction* cacheSlot)
{
    // Step 1: Resolve grain from registry
    InstructionGrain* grain = GrainResolver::instance().ResolveGrain(rawInstruction);

    // Step 2: Decode instruction into cache slot
    decodeInstruction(*cacheSlot, rawInstruction, grain, pc);

    // Cache now contains 32-byte decoded instruction
    // Ready for repeated execution with zero decode overhead
}

// ============================================================================
// Example 2: Execute Cached Instruction
// ============================================================================

void example_ExecuteCached(const DecodedInstruction* cachedDI,
                            AlphaProcessorContext* ctx)
{
    // Direct virtual function call - zero overhead
    cachedDI->grain->execute(*cachedDI, ctx);
}

// ============================================================================
// Example 3: Query Semantics (High-Frequency Operations)
// ============================================================================

void example_SemanticQueries(const DecodedInstruction& di)
{
    // All queries are inline bitwise ops - nanosecond cache hits

    if (isLoad(di)) {
        // Handle load instruction
        quint8 size = getMemSizeBytes(di);  // 1, 2, 4, or 8
        quint8 dest = getRA(di);

        if (isZeroExtend(di)) {
            // Zero-extend load (LDBU, LDWU)
        }
    }

    if (isStore(di)) {
        // Handle store instruction
        quint8 size = getMemSizeBytes(di);
        quint8 src = getRA(di);
    }

    if (isBranch(di)) {
        // Handle branch
        qint32 disp = getBranchDisp(di);
        quint64 target = getBranchTarget(di);

        if (isConditionalBranch(di)) {
            // Conditional branch - check condition
        } else if (isUnconditionalBranch(di)) {
            // Unconditional branch - always taken
        }
    }

    if (isOperate(di)) {
        // Integer operate instruction
        quint8 ra = getRA(di);
        quint8 rb = getRB(di);

        if (hasLiteral(di)) {
            // Use literal instead of RC
            quint8 lit = getLiteral(di);
        } else {
            // Use register RC
            quint8 rc = getRC(di);
        }
    }

    if (isFloat(di)) {
        // Floating-point instruction
    }

    if (isPrivileged(di)) {
        // PALcode or privileged instruction
    }

    if (mustStall(di)) {
        // Pipeline stall required
    }
}

// ============================================================================
// Example 4: Register Access Patterns
// ============================================================================

void example_RegisterAccess(const DecodedInstruction& di,
                             AlphaProcessorContext* ctx)
{
    // Direct register field access - no unpacking
    quint8 dest = getRA(di);
    quint8 srcA = getRB(di);
    quint8 srcB = getRC(di);

    // Check for hardwired zero (R31)
    if (isR31(dest)) {
        // Writing to R31 - result discarded
        return;
    }

    // Read source registers
    quint64 valA = ctx.readIntReg(srcA);
    quint64 valB = isR31(srcB) ? 0 : ctx.readIntReg(srcB);

    // Compute result (example: ADD)
    quint64 result = valA + valB;

    // Write destination
    ctx.writeIntReg(dest, result);
}

// ============================================================================
// Example 5: Memory Operation Pattern
// ============================================================================

void example_MemoryOperation(const DecodedInstruction& di,
                              AlphaProcessorContext* ctx)
{
    // Get memory operand address
    quint8 rb = getRB(di);
    quint64 base = ctx.readIntReg(rb);

    // Memory instructions use 16-bit signed displacement
    qint16 disp = extractMemDisp(di.raw);  // Would need raw for this
    quint64 va = base + disp;

    // Get memory size from packed semantics
    quint8 sizeBytes = getMemSizeBytes(di);

    if (isLoad(di)) {
        // Perform load
        quint64 value = ctx.loadMemory(va, sizeBytes);

        // Handle extension
        if (isZeroExtend(di)) {
            // Already zero-extended by load
        } else if (isSignExtend(di)) {
            // Sign-extend based on size
            value = signExtendValue(value, sizeBytes);
        }

        // Write to destination register
        quint8 dest = getRA(di);
        ctx.writeIntReg(dest, value);
    }
    else if (isStore(di)) {
        // Perform store
        quint8 src = getRA(di);
        quint64 value = ctx.readIntReg(src);
        ctx.storeMemory(va, value, sizeBytes);
    }
}

// ============================================================================
// Example 6: Branch Operation Pattern
// ============================================================================

void example_BranchOperation(const DecodedInstruction& di,
                              AlphaProcessorContext* ctx)
{
    if (!isBranch(di)) return;

    // Get branch target
    quint64 target = getBranchTarget(di);

    if (isConditionalBranch(di)) {
        // Check branch condition (example: BEQ)
        quint8 ra = getRA(di);
        quint64 value = ctx.readIntReg(ra);

        bool taken = (value == 0);  // BEQ condition

        if (taken) {
            ctx.setPC(target);
        }
    }
    else if (isUnconditionalBranch(di)) {
        // Always take branch
        ctx.setPC(target);
    }
}

// ============================================================================
// Example 7: ICache Pattern - Multiple Instructions
// ============================================================================

class InstructionCache
{
public:
    static constexpr int CACHE_SIZE = 1024;  // 1024 instructions

    DecodedInstruction cache[CACHE_SIZE];    // 32 KB total (32 bytes * 1024)

    // Decode and cache instruction
    void decode(quint64 pc, quint32 raw)
    {
        int index = (pc >> 2) & (CACHE_SIZE - 1);

        InstructionGrain* grain = GrainResolver::instance().ResolveGrain(raw);
        decodeInstruction(cache[index], raw, grain, pc);
    }

    // Execute cached instruction
    void execute(quint64 pc, AlphaProcessorContext* ctx)
    {
        int index = (pc >> 2) & (CACHE_SIZE - 1);
        const DecodedInstruction& di = cache[index];

        // Zero-overhead execution
        di.grain->execute(di, ctx);
    }

    // Query cached instruction
    const DecodedInstruction* fetch(quint64 pc) const
    {
        int index = (pc >> 2) & (CACHE_SIZE - 1);
        return &cache[index];
    }
};

// ============================================================================
// Example 8: Fast Path Execution Loop
// ============================================================================

void example_ExecutionLoop(AlphaProcessorContext* ctx,
                           InstructionCache& iCache,
                           int instructionCount)
{
    for (int i = 0; i < instructionCount; ++i)
    {
        quint64 pc = ctx.getPC();

        // Fetch cached decoded instruction
        const DecodedInstruction* di = iCache.fetch(pc);

        // Fast semantic checks (inline bitwise ops)
        if (isPrivileged(*di) && !ctx.inPALmode()) {
            ctx.raiseException(PRIVILEGE_VIOLATION);
            continue;
        }

        if (mustStall(*di)) {
            ctx.handlePipelineStall();
        }

        // Execute - direct virtual call
        di->grain->execute(*di, ctx);

        // Advance PC (if not branch)
        if (!changesPC(*di)) {
            ctx.setPC(pc + 4);
        }
    }
}

// ============================================================================
// Helper: Sign-extend value based on size
// ============================================================================

inline quint64 signExtendValue(quint64 value, quint8 sizeBytes)
{
    switch (sizeBytes) {
        case 1: return static_cast<quint64>(static_cast<qint8>(value));
        case 2: return static_cast<quint64>(static_cast<qint16>(value));
        case 4: return static_cast<quint64>(static_cast<qint32>(value));
        case 8: return value;
        default: return value;
    }
}
 *
 */
#endif // IGRAIN_SEMANICEXAMPLES_H
