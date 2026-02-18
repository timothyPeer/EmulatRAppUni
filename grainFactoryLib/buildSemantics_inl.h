#ifndef BUILDSEMANTICS_INL_H
#define BUILDSEMANTICS_INL_H

#include "DecodedInstruction_inl.h"
#include "InstructionSemantics_mask.h"
#include "iGrain_helper_inl.h"

// ============================================================================
// buildSemantics - Construct Complete Semantic Flags
// ============================================================================
// Called after decoding to populate di.semantics with all format flags.
// This is the master function that combines format detection with semantics.
//
// Semantics already set before this call:
//   - Raw instruction (upper 32 bits of di.semantics)
//   - Memory size (if isMemoryFormat)
//   - Load/Store flags (if isMemoryFormat)
//   - LoadLocked/StoreConditional (if LL/SC)
//
// This function adds:
//   - Format flags (S_OperFmt, S_MemFmt, S_BranchFmt, etc.)
//   - Type flags (S_IntFormat, S_FloatFormat)
//   - Control flow flags (S_ChangesPC, S_Branch)
//   - Special flags (S_Privileged, S_MustStall, S_OverflowTrap)
// ============================================================================

AXP_FLATTEN
    static void buildSemantics(DecodedInstruction& di) noexcept
{
    if (!di.grain) {
        // Invalid instruction - no semantics
        di.semantics &= ~SEMANTICS_MASK;  // Clear semantic flags, keep raw
        return;
    }

    const quint32 raw = getRaw(di);
    const quint8 opcode = extractOpcode(raw);

    // Start with just the raw bits (already in di.semantics upper 32 bits)
    InstrSemantics sem = S_None;

    // ════════════════════════════════════════════════════════════
    // 1. CLASSIFY FORMAT
    // ════════════════════════════════════════════════════════════
    InstrFormat fmt = classifyFormat(opcode);

    switch (fmt) {
		
    case InstrFormat::Operate:
        sem = static_cast<InstrSemantics>(sem | S_OperFmt | S_IntFormat);

        // Check for overflow trap variant (/V bit)
        if (opcode == 0x10) {  // Integer arithmetic
            const quint16 func = extractFunction(raw);
            if (func & 0x40) {  // Bit 6 = trap enable
                sem = static_cast<InstrSemantics>(sem | S_OverflowTrap);
            }
        }
        break;

    case InstrFormat::Memory:
        sem = static_cast<InstrSemantics>(sem | S_MemFmt);
        // Load/Store flags already set by decodeMemSize()
        break;

    case InstrFormat::Branch:
        sem = static_cast<InstrSemantics>(sem | S_BranchFmt | S_Branch | S_ChangesPC);

        // Check if branch writes link register
        if (opcode == 0x30 || opcode == 0x34) {  // BR, BSR
            sem = static_cast<InstrSemantics>(sem | S_BranchWriteLink);
        }
        break;

    case InstrFormat::Float:
        sem = static_cast<InstrSemantics>(sem | S_FloatFormat);

        // Floating-point operates (not memory)
        if (opcode >= 0x14 && opcode <= 0x17) {
            sem = static_cast<InstrSemantics>(sem | S_OperFmt);
        }
        break;

    case InstrFormat::Pal:
        sem = static_cast<InstrSemantics>(sem | S_PalFormat | S_Privileged | S_ChangesPC);
        break;

    case InstrFormat::MemoryMB:
        // Memory barriers, FETCH, etc.
        sem = static_cast<InstrSemantics>(sem | S_OperFmt);

        if (opcode == 0x18) {
            const quint16 func = extractFunction(raw);

            // Memory barriers
            if (func == 0x4000) {  // MB
                sem = static_cast<InstrSemantics>(sem | S_Barrier | S_SideEffect);
            }
            else if (func == 0x4400) {  // WMB
                sem = static_cast<InstrSemantics>(sem | S_Barrier | S_SideEffect);
            }
            else if (func == 0x8000 || func == 0xA000) {  // FETCH, FETCH_M
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
            else if (func == 0xC000) {  // RPCC
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
            else if (func == 0xE000) {  // RC
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
            else if (func == 0xE800) {  // ECB
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
            else if (func == 0xF000) {  // RS
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
            else if (func == 0xF800) {  // WH64
                sem = static_cast<InstrSemantics>(sem | S_SideEffect);
            }
        }
       	if (opcode == 0x1A) {  // JMP, JSR, RET, JSR_COROUTINE
			sem = static_cast<InstrSemantics>(sem | S_JumpFmt | S_ChangesPC | S_Branch | S_Uncond);

			const quint16 func = extractFunction(raw);
			if (func == 0x01 || func == 0x02 || func == 0x03) {  // JSR variants
				sem = static_cast<InstrSemantics>(sem | S_BranchWriteLink);
			}
		}
        break;

    default:
        // Unknown format - no additional semantics
        break;
    }

    // ════════════════════════════════════════════════════════════
    // 2. CHECK GRAIN FLAGS
    // ════════════════════════════════════════════════════════════
    if (!di.grain->canDualIssue()) {
        sem = static_cast<InstrSemantics>(sem | S_MustStall);
    }

    if (di.grain->needsStall()) {
        sem = static_cast<InstrSemantics>(sem | S_MustStall);
    }

    // ════════════════════════════════════════════════════════════
    // 3. MERGE WITH EXISTING SEMANTICS
    // ════════════════════════════════════════════════════════════
    // Preserve:
    //   - Raw instruction (bits 32-63)
    //   - Memory size (bits 21-23)
    //   - Load/Store/LL/SC flags (already set by decodeMemSize)

    // Clear old format flags, keep raw + memsize + load/store
    constexpr quint64 PRESERVE_MASK =
        (0xFFFFFFFFULL << 32) |  // Raw instruction
        S_MEMSIZE_MASK |         // Memory size
        S_Load |                 // Load flag
        S_Store |                // Store flag
        S_LoadLocked |           // LL flag
        S_StoreConditional |     // SC flag
        S_ZeroExtend |           // Zero-extend flag
        S_SignExtend;            // Sign-extend flag

    di.semantics = (di.semantics & PRESERVE_MASK) | static_cast<quint64>(sem);
}

// ============================================================================
// ALTERNATIVE: buildSemantics with opcode optimization
// ============================================================================
// For hot paths where opcode is already extracted

AXP_FLATTEN
    static void buildSemanticsWithOpcode(DecodedInstruction& di, quint8 opcode) noexcept
{
    if (!di.grain) {
        di.semantics &= ~SEMANTICS_MASK;
        return;
    }

    const quint32 raw = getRaw(di);
    InstrSemantics sem = S_None;

    // Fast path format detection using pre-extracted opcode
    if (opcode >= 0x14 && opcode <= 0x17) {
        // Float
        sem = static_cast<InstrSemantics>(sem | S_FloatFormat | S_OperFmt);
    }
    else if (opcode == 0x00) {
        // PAL
        sem = static_cast<InstrSemantics>(sem | S_PalFormat | S_Privileged | S_ChangesPC);
    }
    else if (opcode == 0x30 || opcode == 0x31) {
        // Branch
        sem = static_cast<InstrSemantics>(sem | S_BranchFmt | S_Branch | S_ChangesPC);
        if (opcode == 0x30 || opcode == 0x34) {
            sem = static_cast<InstrSemantics>(sem | S_BranchWriteLink);
        }
    }
    else if (opcode >= 0x20 && opcode <= 0x2F) {
        // Memory
        sem = static_cast<InstrSemantics>(sem | S_MemFmt);
    }
    else if (opcode == 0x18) {
        // MemoryMB
        sem = static_cast<InstrSemantics>(sem | S_OperFmt);
        const quint16 func = extractFunction(raw);
        if (func == 0x4000 || func == 0x4400) {
            sem = static_cast<InstrSemantics>(sem | S_Barrier | S_SideEffect);
        }
    }
    else if ((opcode >= 0x10 && opcode <= 0x17) || (opcode >= 0x18 && opcode <= 0x1F)) {
        // Operate
        sem = static_cast<InstrSemantics>(sem | S_OperFmt | S_IntFormat);
        if (opcode == 0x10) {
            const quint16 func = extractFunction(raw);
            if (func & 0x40) {
                sem = static_cast<InstrSemantics>(sem | S_OverflowTrap);
            }
        }
    }

    // Grain flags
    if (!di.grain->canDualIssue() || di.grain->needsStall()) {
        sem = static_cast<InstrSemantics>(sem | S_MustStall);
    }

    // Merge with existing semantics
    constexpr quint64 PRESERVE_MASK =
        (0xFFFFFFFFULL << 32) | S_MEMSIZE_MASK | S_Load | S_Store |
        S_LoadLocked | S_StoreConditional | S_ZeroExtend | S_SignExtend;

    di.semantics = (di.semantics & PRESERVE_MASK) | static_cast<quint64>(sem);
}

#endif // BUILDSEMANTICS_INL_H
