#ifndef DECODEDINSTRUCTION_INL_H
#define DECODEDINSTRUCTION_INL_H

#include <QtGlobal>
#include "../coreLib/Axp_Attributes_core.h"
#include "DecodedInstruction.h"
#include "InstructionSemantics_mask.h"
#include "iGrain-KeyIdenties.h"
#include "../coreLib/fp_variant_core.h"
#include "palLib_EV6/PAL_core.h"
#include "palLib_EV6/Pal_core_inl.h"
#include "Grain_const_core.h"
#include "grainFactoryLib/grain_core.h"

static_assert(sizeof(DecodedInstruction) == 40, "DecodedInstruction size changed");

// ============================================================================
// MemSize_enum
// NOTE: We store this enum value directly into the MEMSIZE field (3 bits).
// Therefore values MUST be 0..7 for storage.
// ============================================================================
enum class MemSize_enum : quint8
{
    None = 0,
    Byte = 1,
    Word = 2,
    Long = 3,
    Quad = 4,
    FLOAT_F = 5,
    FLOAT_G = 6,
    FLOAT_S = 7,
    // FLOAT_T would be 8 -> cannot fit in 3 bits. Keep FLOAT_T OUT of memsize bits.
    // If you need FLOAT_T distinctly, either:
    //   (A) store float width via a separate flag, or
    //   (B) change memsize field width to 4 bits.
};

// If you do want T distinct, easiest: treat it as Quad width + a float-kind elsewhere.
// For now, FLOAT_T is represented by Quad+S_FloatFormat (8 bytes) in size queries.

enum class FPOperationCategory : quint8
{
    Unknown = 0,
    Arithmetic,
    Comparison,
    Conversion,
    ConditionalMove,
    Utility
};

// ============================================================================
// MemSize conversion helpers
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE quint8 memSizeBytes(MemSize_enum e) noexcept
{
    switch (e)
    {
    case MemSize_enum::Byte:    return 1;
    case MemSize_enum::Word:    return 2;
    case MemSize_enum::Long:    return 4;
    case MemSize_enum::Quad:    return 8;

    case MemSize_enum::FLOAT_F: return 4;
    case MemSize_enum::FLOAT_S: return 4;
    case MemSize_enum::FLOAT_G: return 8;

    default:                    return 0;
    }
}

// ============================================================================
// Raw instruction access
// ============================================================================
static AXP_HOT AXP_ALWAYS_INLINE quint32 getRaw(const DecodedInstruction& di) noexcept
{
    return di.rawBits(); // your struct already has rawBits()
}

static AXP_HOT AXP_ALWAYS_INLINE quint8 extractOpcode(quint32 raw) noexcept
{
    return static_cast<quint8>((raw >> 26) & 0x3F);
}

// Low 16 bits are NOT always a "function". Only Operate formats use func layout.
static AXP_HOT AXP_ALWAYS_INLINE quint16 extractLow16(quint32 raw) noexcept
{
    return static_cast<quint16>(raw & 0xFFFF);
}

static AXP_HOT AXP_ALWAYS_INLINE qint16 extractMemDisp(quint32 raw) noexcept
{
    return static_cast<qint16>(raw & 0xFFFF);
}

AXP_HOT AXP_ALWAYS_INLINE quint8 extractRA(quint32 raw) noexcept { return (raw >> 21) & 0x1F; }
AXP_HOT AXP_ALWAYS_INLINE quint8 extractRB(quint32 raw) noexcept { return (raw >> 16) & 0x1F; }
AXP_HOT AXP_ALWAYS_INLINE quint8 extractRC(quint32 raw) noexcept { return raw & 0x1F; }

AXP_HOT AXP_ALWAYS_INLINE bool extractLBit(quint32 raw) noexcept { return ((raw >> 12) & 1) != 0; }
AXP_HOT AXP_ALWAYS_INLINE quint8 extractLiteral(quint32 raw) noexcept { return (raw >> 13) & 0xFF; }

// ============================================================================
// Semantics / raw packing
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE void setRaw(DecodedInstruction& di, quint32 raw) noexcept
{
    // preserve low 32 bits semantics, overwrite high 32 raw
    di.semantics = (di.semantics & SEMANTICS_MASK) | (quint64(raw) << RAW_SHIFT);
}

AXP_HOT AXP_ALWAYS_INLINE quint8 getOpcodeFromPacked(const DecodedInstruction& di) noexcept
{
    return static_cast<quint8>((di.semantics >> (RAW_SHIFT + 26)) & 0x3F);
}

// ============================================================================
// Semantic flag manipulation
// ============================================================================
static AXP_HOT AXP_ALWAYS_INLINE void addSem(quint64& m, InstrSemantics s) noexcept
{
    m |= static_cast<quint64>(s);
}

static AXP_HOT AXP_ALWAYS_INLINE void removeSem(quint64& m, InstrSemantics s) noexcept
{
    m &= ~static_cast<quint64>(s);
}




static AXP_HOT AXP_ALWAYS_INLINE bool hasSem(quint64 m, InstrSemantics s) noexcept
{
    return (m & static_cast<quint64>(s)) != 0;
}

AXP_HOT AXP_ALWAYS_INLINE bool hasLiteralBit(quint32 raw) noexcept
{
    // Alpha Operate-format literal bit is bit 12
    return ((raw >> 12) & 0x1U) != 0;
}

AXP_HOT AXP_ALWAYS_INLINE bool hasLiteralBit(const DecodedInstruction& di) noexcept
{
    return hasLiteralBit(di.rawBits());
}

static AXP_HOT AXP_ALWAYS_INLINE void clearAllSemanticsPreserveRaw(quint64& m) noexcept
{
    m &= RAW_MASK;
}

static AXP_HOT AXP_ALWAYS_INLINE void toggleSem(quint64& m, InstrSemantics s) noexcept
{
    m ^= static_cast<quint64>(s);
}

static AXP_HOT AXP_ALWAYS_INLINE void setSem(quint64& m, InstrSemantics s, bool enable) noexcept
{
    if (enable) m |= static_cast<quint64>(s);
    else        m &= ~static_cast<quint64>(s);
}

// ============================================================================
// Format checks (read from semantics)
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE bool isLoad(const DecodedInstruction& di) noexcept { return (di.semantics & S_Load) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isStore(const DecodedInstruction& di) noexcept { return (di.semantics & S_Store) != 0; }

AXP_HOT AXP_ALWAYS_INLINE bool isJumpFormat(const DecodedInstruction& di) noexcept { return (di.semantics & S_JumpFmt) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isBranchFormat(const DecodedInstruction& di) noexcept { return (di.semantics & S_BranchFmt) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isMemoryFormat(const DecodedInstruction& di) noexcept
{

    return (di.semantics & S_MemFmt) != 0;
}
AXP_HOT AXP_ALWAYS_INLINE bool isOperateFormat(const DecodedInstruction& di) noexcept { return (di.semantics & S_OperFmt) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isFloatFormat(const DecodedInstruction& di) noexcept { return (di.semantics & S_FloatFormat) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isPalFormat(const DecodedInstruction& di) noexcept { return (di.semantics & S_PalFormat) != 0; }

AXP_HOT AXP_ALWAYS_INLINE bool changesPC(const DecodedInstruction& di) noexcept { return (di.semantics & S_ChangesPC) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isUnconditional(const DecodedInstruction& di) noexcept { return (di.semantics & S_Uncond) != 0; }

AXP_HOT AXP_ALWAYS_INLINE bool isLoadLocked(const DecodedInstruction& di) noexcept { return (di.semantics & S_LoadLocked) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isStoreConditional(const DecodedInstruction& di) noexcept { return (di.semantics & S_StoreConditional) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isLLSC(const DecodedInstruction& di) noexcept { return (di.semantics & (S_LoadLocked | S_StoreConditional)) != 0; }

AXP_HOT AXP_ALWAYS_INLINE bool hasSideEffects(const DecodedInstruction& di) noexcept { return (di.semantics & S_SideEffect) != 0; }
AXP_HOT AXP_ALWAYS_INLINE bool isBarrier(const DecodedInstruction& di) noexcept { return (di.semantics & S_Barrier) != 0; }

AXP_HOT AXP_ALWAYS_INLINE quint8 extractJumpHint(quint32 raw) noexcept { return (raw >> 14) & 0x3; }
/*
 *Will this control-transfer instruction write a link value into Ra?
 */
AXP_HOT AXP_ALWAYS_INLINE bool isWritesLinkRegister(const DecodedInstruction& di) noexcept {
    quint8 opcode = extractOpcode(di.rawBits());

    // BSR always writes link
    if (opcode == 0x34) return true;

    // BR writes link (often to R31, which gets discarded)
    if (opcode == 0x30) return true;

    // JSR and JSR_COROUTINE write link
    if (opcode == 0x1A) {
        quint8 hint = extractJumpHint(di.rawBits());
        return (hint == 0x1 || hint == 0x3);  // JSR or JSR_COROUTINE
    }

    return false;
}



// ============================================================================
// Memsize field get/set
// ============================================================================

AXP_HOT AXP_ALWAYS_INLINE void setMemSize(DecodedInstruction& di, MemSize_enum sz) noexcept
{
    // just set the field directly
    di.memSize = static_cast<quint8>(sz);

    // No longer needed - semantics stays untouched!
    // di.semantics = (di.semantics & ~S_MEMSIZE_MASK) | (quint64(sz) << S_MEMSIZE_SHIFT);
}

AXP_HOT AXP_ALWAYS_INLINE MemSize_enum getMemSize(const DecodedInstruction& di) noexcept
{
    return static_cast<MemSize_enum>(di.memSize);
}


// ============================================================================
// Opcode helpers
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE quint8 getOpcode(const DecodedInstruction& di) noexcept
{
    return extractOpcode(getRaw(di));
}

// ============================================================================
// Jump format helpers (opcode 0x1A) - hint bits [15:14]
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE bool isJumpOpcodeFamily(quint8 opcode) noexcept { return opcode == 0x1A; }


AXP_HOT AXP_ALWAYS_INLINE bool isJMP(const DecodedInstruction& di) noexcept
{
    if (getOpcode(di) != 0x1A) return false;
    return extractJumpHint(di.rawBits()) == 0x0;
}
AXP_HOT AXP_ALWAYS_INLINE bool isJSR(const DecodedInstruction& di) noexcept
{
    if (getOpcode(di) != 0x1A) return false;
    return extractJumpHint(di.rawBits()) == 0x1;
}
AXP_HOT AXP_ALWAYS_INLINE bool isRET(const DecodedInstruction& di) noexcept
{
    if (getOpcode(di) != 0x1A) return false;
    return extractJumpHint(di.rawBits()) == 0x2;
}
AXP_HOT AXP_ALWAYS_INLINE bool isJSR_COROUTINE(const DecodedInstruction& di) noexcept
{
    if (getOpcode(di) != 0x1A) return false;
    return extractJumpHint(di.rawBits()) == 0x3;
}

AXP_HOT AXP_ALWAYS_INLINE bool isJumpWithLink(const DecodedInstruction& di) noexcept
{
    if (getOpcode(di) != 0x1A) return false;
    const quint8 hint = extractJumpHint(di.rawBits());
    return (hint == 0x1 || hint == 0x3);
}

// ============================================================================
// Branch displacement extraction (21-bit signed in bits [20:0])
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE qint32 extractBranchDisplacement(quint32 instruction) noexcept
{
    quint32 disp = instruction & 0x1FFFFF;
    if (disp & 0x100000) disp |= 0xFFE00000;
    return static_cast<qint32>(disp);
}


AXP_HOT AXP_ALWAYS_INLINE qint32 extractDisp21(quint32 raw) noexcept
{
    quint32 d = raw & 0x001FFFFF;          // bits 20:0
    if (d & 0x00100000) d |= 0xFFE00000;   // sign-extend from bit 20
    return static_cast<qint32>(d);
}

AXP_HOT AXP_ALWAYS_INLINE quint64 branchTarget(quint64 pc, quint32 raw) noexcept
{
    const qint32 disp21 = extractDisp21(raw);
    return (pc + 4) + (static_cast<qint64>(disp21) << 2);
}

// ============================================================================
// Memory displacement extraction (signed 16-bit)
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE qint16 getMemDisp(const DecodedInstruction& di) noexcept
{
    return extractMemDisp(getRaw(di));
}

// ============================================================================
// Function code extraction for Operate formats
// - integer operate: func7 = bits [11:5]
// - fp operate:      func11 = bits [15:5]
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE quint8  getFunctionCode7(const DecodedInstruction& di) noexcept
{
    const quint32 raw = getRaw(di);
    return static_cast<quint8>((raw >> 5) & 0x7F);
}

AXP_HOT AXP_ALWAYS_INLINE quint32 getFunctionCode11(const DecodedInstruction& di) noexcept
{
    const quint32 raw = getRaw(di);
    return (raw >> 5) & 0x7FF;
}


/**
 * @brief Get function code from instruction (format-aware)
 *
 * Automatically determines instruction format and extracts appropriate function code:
 * - Integer operate (0x10-0x13): 7-bit func [11:5]
 * - Float operate (0x14-0x17): 11-bit func [15:5]
 * - CALL_PAL (0x00): 8-bit PAL function [7:0]
 * - Misc (0x18): 16-bit function [15:0]
 * - Jump (0x1A): 2-bit hint [15:14]
 * - Other formats: 0 (no function code)
 *
 * @param di DecodedInstruction
 * @return Function code (width depends on instruction format)
 */
AXP_HOT AXP_ALWAYS_INLINE quint16 getFunctionCode(const DecodedInstruction& di) noexcept
{
    const quint32 raw = di.rawBits();
    const quint8 opcode = getOpcodeFromPacked(di);

    // Integer operate format: 7-bit function [11:5]
    if (opcode >= 0x10 && opcode <= 0x13) {
        return (raw >> 5) & 0x7F;
    }

    // Float operate format: 11-bit function [15:5]
    if (opcode >= 0x14 && opcode <= 0x17) {
        return (raw >> 5) & 0x7FF;
    }

    // CALL_PAL: 8-bit PAL function [7:0]
    if (opcode == 0x00) {
        return raw & 0xFF;
    }

    // Misc format: 16-bit function [15:0]
    if (opcode == 0x18) {
        return raw & 0xFFFF;
    }

    // Jump format: 2-bit hint [15:14]
    if (opcode == 0x1A) {
        return (raw >> 14) & 0x3;
    }

    // Memory format, branch format, etc. - no function code
    return 0;
}
// ============================================================================
// CALL_PAL detection (opcode 0x00)
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE bool isCallPal(const DecodedInstruction& di) noexcept
{
    return getOpcode(di) == 0x00;
}

AXP_HOT AXP_ALWAYS_INLINE PalCallPalFunction decodedCallPalFunction(const DecodedInstruction& di) noexcept
{
    return static_cast<PalCallPalFunction>(palFunction(di.rawBits()));
}

AXP_HOT AXP_ALWAYS_INLINE bool isBranchOpcodeFamily(quint8 opcode) noexcept
{
    // Alpha branch family opcodes are 0x30..0x3F
    return (opcode & 0xF0) == 0x30;
}

AXP_HOT AXP_ALWAYS_INLINE bool isOverflowTrapInstruction(const DecodedInstruction& di) noexcept
{
    // Only integer operate format (opcode 0x11) can generate integer overflow traps (/V)
    if (getOpcode(di) != 0x11) {
        return false;
    }

    // Integer operate function is bits 11:5 (7-bit)
    const quint8 func7 = getFunctionCode7(di);

    switch (func7) {
    case 0x40: // ADDL/V
    case 0x60: // ADDQ/V
    case 0x49: // SUBL/V
    case 0x69: // SUBQ/V
    case 0x42: // MULL/V
    case 0x62: // MULQ/V
        return true;
    default:
        return false;
    }
}


// ============================================================================
// FP variant convenience
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE FPVariant getFPVariant(const DecodedInstruction& di) noexcept
{
    return extractFPVariantFromBits(getRaw(di));
}

// ============================================================================
// Decode memory size + set related semantics.
// This is the ONLY authoritative memsize decode.
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE void decodeMemSize(DecodedInstruction& di) noexcept
{
    if ((di.semantics & S_MemFmt) == 0) {
        setMemSize(di, MemSize_enum::None);
        return;
    }

    const quint8 opcode = getOpcode(di);

    // ================================================================
    // LDA/LDAH are computational (not memory operations)
    // ================================================================
    if (opcode == 0x08 || opcode == 0x09) {
        setMemSize(di, MemSize_enum::None);
        // Clear any memory-related flags
        di.semantics &= ~(S_MemFmt | S_Load | S_Store);
        return;
    }

    // Check if this is actually a memory instruction
    if ((di.semantics & S_MemFmt) == 0) {
        setMemSize(di, MemSize_enum::None);
        return;
    }

    MemSize_enum sz = MemSize_enum::None;

    switch (opcode)
    {
        // Integer byte/word
    case 0x0A: sz = MemSize_enum::Byte; di.semantics |= (S_Load | S_ZeroExtend); break; // LDBU
    case 0x0C: sz = MemSize_enum::Word; di.semantics |= (S_Load | S_ZeroExtend); break; // LDWU
    case 0x0E: sz = MemSize_enum::Byte; di.semantics |= S_Store; break;                // STB
    case 0x0D: sz = MemSize_enum::Word; di.semantics |= S_Store; break;                // STW (verify in your opcode table)

        // Integer long/quad + LL/SC
    case 0x28: sz = MemSize_enum::Long; di.semantics |= S_Load; break;                   // LDL
    case 0x29: sz = MemSize_enum::Quad; di.semantics |= S_Load; break;                   // LDQ
    case 0x2A: sz = MemSize_enum::Long; di.semantics |= (S_Load | S_LoadLocked); break;  // LDL_L
    case 0x2B: sz = MemSize_enum::Quad; di.semantics |= (S_Load | S_LoadLocked); break;  // LDQ_L
    case 0x2C: sz = MemSize_enum::Long; di.semantics |= S_Store; break;                  // STL
    case 0x2D: sz = MemSize_enum::Quad; di.semantics |= S_Store; break;                  // STQ
    case 0x2E: sz = MemSize_enum::Long; di.semantics |= (S_Store | S_StoreConditional); break; // STL_C
    case 0x2F: sz = MemSize_enum::Quad; di.semantics |= (S_Store | S_StoreConditional); break; // STQ_C

        // Unaligned quad
    case 0x0B: sz = MemSize_enum::Quad; di.semantics |= S_Load; break;                   // LDQ_U
    case 0x0F: sz = MemSize_enum::Quad; di.semantics |= S_Store; break;                  // STQ_U

        // Floating memory ops (encode as FLOAT_* but byte size comes from memSizeBytes())
    case 0x20: sz = MemSize_enum::FLOAT_F; di.semantics |= (S_Load | S_FloatFormat); break; // LDF 4B
    case 0x21: sz = MemSize_enum::FLOAT_G; di.semantics |= (S_Load | S_FloatFormat); break; // LDG 8B
    case 0x22: sz = MemSize_enum::FLOAT_S; di.semantics |= (S_Load | S_FloatFormat); break; // LDS 4B
        // LDT is 8B; we represent as Quad+FloatFormat (since FLOAT_T doesn't fit in 3 bits)
    case 0x23: sz = MemSize_enum::Quad;    di.semantics |= (S_Load | S_FloatFormat); break; // LDT 8B

    case 0x24: sz = MemSize_enum::FLOAT_F; di.semantics |= (S_Store | S_FloatFormat); break; // STF
    case 0x25: sz = MemSize_enum::FLOAT_G; di.semantics |= (S_Store | S_FloatFormat); break; // STG
    case 0x26: sz = MemSize_enum::FLOAT_S; di.semantics |= (S_Store | S_FloatFormat); break; // STS
    case 0x27: sz = MemSize_enum::Quad;    di.semantics |= (S_Store | S_FloatFormat); break; // STT 8B

    default:
        sz = MemSize_enum::None;
        break;
    }

    setMemSize(di, sz);
}

// ============================================================================
// Misc helpers
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE bool writesToR31(const DecodedInstruction& di) noexcept
{
    return di.rc == 31;
}

AXP_HOT AXP_ALWAYS_INLINE quint8 getIPRIndex(const DecodedInstruction& di) noexcept
{
    return static_cast<quint8>((di.rawBits() >> 0) & 0xFF);
}

AXP_HOT AXP_ALWAYS_INLINE quint8 getRbNumber(const DecodedInstruction& di) noexcept
{
    return static_cast<quint8>((di.rawBits() >> 16) & 0x1F);
}

#endif // DECODEDINSTRUCTION_INL_H
