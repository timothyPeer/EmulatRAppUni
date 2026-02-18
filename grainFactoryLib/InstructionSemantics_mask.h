#ifndef INSTRUCTIONSEMANTICS_MASK_H
#define INSTRUCTIONSEMANTICS_MASK_H

#include <QtGlobal>

// ============================================================================
// 32-bit Semantics Layout (low 32 bits of DecodedInstruction::semantics)
//
// Bits  0.. 3 : Memory behavior
// Bits  4.. 9 : Format classification
// Bits 10..13 : Branch/control semantics
// Bits 14..15 : Operand encoding semantics
// Bits 16..18 : Privilege/trap semantics
// Bits 19..21 : Pipeline/ordering semantics
// Bit      22 : PAL format
// Bits 23..24 : LL/SC semantics
// Bits 25..27 : MemSize enum (3 bits)
// Bit      28 : Writes link register (Ra) semantics
// Bit      29 : Can dual issue
// Bit      30 : Needs stall
// Bit      31 : Reserved
// ============================================================================

enum InstrSemantics : quint64
{
    S_None = 0u,

    // ---- Memory behavior (0..3)
    S_Load = 1u << 0,
    S_Store = 1u << 1,
    S_ZeroExtend = 1u << 2,
    S_SignExtend = 1u << 3,

    // ---- Formats (4..9)
    S_FloatFormat = 1u << 4,
    S_IntFormat = 1u << 5,
    S_BranchFmt = 1u << 6,
    S_MemFmt = 1u << 7,
    S_OperFmt = 1u << 8,
    S_JumpFmt = 1u << 9,

    // ---- Branch/control semantics (10..13)
    S_Branch = 1u << 10,
    S_Cond = 1u << 11,
    S_Uncond = 1u << 12,
    S_ChangesPC = 1u << 13,

    // ---- Operand encoding (14..15)
    S_UsesLiteral = 1u << 14,
    S_UsesIndex = 1u << 15,

    // ---- Privilege & traps (16..18)
    S_Privileged = 1u << 16,
    S_Trap = 1u << 17,
    S_OverflowTrap = 1u << 18,

    // ---- Pipeline/ordering (19..21)
    S_SideEffect = 1u << 19,
    S_Barrier = 1u << 20,
    S_NeedsFence = 1u << 21,   // optional “ordering required” bit (distinct from Barrier)

    // ---- PAL format (22)
    S_PalFormat = 1u << 22,

    // ---- LL/SC (23..24)
    S_LoadLocked = 1u << 23,
    S_StoreConditional = 1u << 24,

    // ---- Link semantics (28)
    S_BranchWriteLink = 1u << 28,

    // ---- Scheduling (29..30)
    S_CanDualIssue = 1u << 29,
    S_NeedsStall = 1u << 30,
};



#endif // INSTRUCTIONSEMANTICS_MASK_H
