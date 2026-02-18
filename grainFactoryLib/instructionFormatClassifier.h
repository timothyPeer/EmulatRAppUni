// ============================================================================
// Instruction Format Classification
// ============================================================================
// Add this to DecodedInstruction_inl.h

/**
 * @brief Alpha instruction encoding formats
 *
 * Based on Alpha Architecture Reference Manual, Chapter 3
 */
enum class InstructionFormat : quint8 {
    Unknown = 0,
    Memory,          // LDA, LDQ, STQ, LDL, STL, etc. (opcode 0x08-0x0F, 0x20-0x2F)
    MemoryFC,        // Floating point load/store (LDF, LDG, LDS, LDT, STF, STG, STS, STT)
    Branch,          // BR, BSR, BEQ, BNE, BLT, BLE, BGT, BGE, BLBC, BLBS (opcode 0x30-0x3F)
    Operate,         // Integer arithmetic/logical (ADDL, SUBL, AND, OR, etc.)
    FloatingPoint,   // FP arithmetic (ADDF, ADDT, MULF, MULT, DIVF, DIVT, SQRT, etc.)
    Jump,            // JMP, JSR, RET, JSR_COROUTINE (opcode 0x1A)
    PALcode,         // CALL_PAL, HW_MFPR, HW_MTPR, HW_REI, HW_LD, HW_ST (opcode 0x00, 0x19, 0x1B, 0x1D, 0x1E, 0x1F)
    Miscellaneous    // TRAPB, EXCB, MB, WMB, FETCH, FETCH_M, RC, RS, etc.
};

/**
 * @brief Get instruction format from DecodedInstruction
 *
 * @param di Decoded instruction
 * @return InstructionFormat The encoding format of this instruction
 *
 * @note This is a fast lookup based on opcode and semantic flags.
 *       Cost: ~3-5 cycles (one load, one shift, one branch/switch)
 */
static AXP_HOT AXP_ALWAYS_INLINE InstructionFormat getInstructionFormat(const DecodedInstruction& di) noexcept
{
    // Fast path: check semantic flags first (already decoded)
    if (isMemoryFormat(di)) {
        // Distinguish integer vs floating-point memory ops
        if (isFloatFormat(di)) {
            return InstructionFormat::MemoryFC;
        }
        return InstructionFormat::Memory;
    }

    if (isBranchFormat(di)) {
        return InstructionFormat::Branch;
    }

    if (isJumpFormat(di)) {
        return InstructionFormat::Jump;
    }

    if (isOperateFormat(di)) {
        return InstructionFormat::Operate;
    }

    if (isFloatFormat(di)) {
        return InstructionFormat::FloatingPoint;
    }

    // Fallback: extract opcode from raw bits
    const quint8 opcode = getOpcodeFromPacked(di);

    // PALcode instructions
    if (opcode == 0x00 ||  // CALL_PAL
        opcode == 0x19 ||  // HW_MFPR
        opcode == 0x1B ||  // HW_LD
        opcode == 0x1D ||  // HW_MTPR
        opcode == 0x1E ||  // HW_REI
        opcode == 0x1F) {  // HW_ST
        return InstructionFormat::PALcode;
    }

    // Miscellaneous instructions (memory barriers, etc.)
    if (opcode == 0x18) {  // MISC group (TRAPB, EXCB, MB, WMB, FETCH, etc.)
        return InstructionFormat::Miscellaneous;
    }

    return InstructionFormat::Unknown;
}


/**
 * @brief Get instruction format from opcode only (for decode stage)
 *
 * @param opcode 6-bit opcode (bits 26-31 of instruction)
 * @return InstructionFormat The encoding format
 *
 * @note This version doesn't have access to semantic flags, so it's
 *       slightly less precise but useful during initial decode.
 */
static AXP_HOT AXP_ALWAYS_INLINE InstructionFormat getInstructionFormatFromOpcode(quint8 opcode) noexcept
{
    // Memory format - integer loads/stores
    if (opcode >= 0x08 && opcode <= 0x0F) {  // LDA, LDAH, LDBU, LDQ_U, LDWU, STW, STB, STQ_U
        return InstructionFormat::Memory;
    }
    if (opcode >= 0x20 && opcode <= 0x2F) {  // LDF, LDG, LDS, LDT, STF, STG, STS, STT, LDL, LDQ, LDL_L, LDQ_L, STL, STQ, STL_C, STQ_C
        // Opcodes 0x20-0x23 are FP loads, 0x24-0x27 are FP stores
        if (opcode <= 0x27) {
            return InstructionFormat::MemoryFC;
        }
        // Opcodes 0x28-0x2F are integer loads/stores
        return InstructionFormat::Memory;
    }

    // Branch format
    if (opcode >= 0x30 && opcode <= 0x3F) {  // BR, FBEQ, FBLT, FBLE, BSR, FBNE, FBGE, FBGT, BLBC, BEQ, BLT, BLE, BLBS, BNE, BGE, BGT
        return InstructionFormat::Branch;
    }

    // Operate format
    if (opcode >= 0x10 && opcode <= 0x13) {  // INTA (0x10), INTL (0x11), INTS (0x12), INTM (0x13)
        return InstructionFormat::Operate;
    }

    // Floating-point operate
    if (opcode >= 0x14 && opcode <= 0x17) {  // ITFP (0x14), FLTV (0x15), FLTI (0x16), FLTL (0x17)
        return InstructionFormat::FloatingPoint;
    }

    // Jump format
    if (opcode == 0x1A) {  // JMP, JSR, RET, JSR_COROUTINE
        return InstructionFormat::Jump;
    }

    // PALcode format
    if (opcode == 0x00) {  // CALL_PAL
        return InstructionFormat::PALcode;
    }
    if (opcode == 0x19 ||  // HW_MFPR
        opcode == 0x1B ||  // HW_LD
        opcode == 0x1D ||  // HW_MTPR
        opcode == 0x1E ||  // HW_REI
        opcode == 0x1F) {  // HW_ST
        return InstructionFormat::PALcode;
    }

    // Miscellaneous
    if (opcode == 0x18) {  // MISC (TRAPB, EXCB, MB, WMB, FETCH, FETCH_M, RPCC, RC, RS)
        return InstructionFormat::Miscellaneous;
    }

    return InstructionFormat::Unknown;
}

/**
 * @brief Get human-readable name of instruction format
 *
 * @param format Instruction format enum
 * @return const char* String name of format
 */
static constexpr const char* getInstructionFormatName(InstructionFormat format) noexcept
{
    switch (format) {
    case InstructionFormat::Memory:         return "Memory";
    case InstructionFormat::MemoryFC:       return "Memory-FP";
    case InstructionFormat::Branch:         return "Branch";
    case InstructionFormat::Operate:        return "Operate";
    case InstructionFormat::FloatingPoint:  return "Floating-Point";
    case InstructionFormat::Jump:           return "Jump";
    case InstructionFormat::PALcode:        return "PALcode";
    case InstructionFormat::Miscellaneous:  return "Miscellaneous";
    case InstructionFormat::Unknown:
    default:                                return "Unknown";
    }
}

// ============================================================================
// Usage Examples
// ============================================================================

/*

// Example 1: Get format from decoded instruction
DecodedInstruction di = ...;
InstructionFormat fmt = getInstructionFormat(di);

switch (fmt) {
    case InstructionFormat::Memory:
        handleMemoryInstruction(di);
        break;
    case InstructionFormat::Branch:
        handleBranchInstruction(di);
        break;
    // ... etc
}

// Example 2: Get format during decode (before semantic flags set)
quint32 raw = fetchInstruction(pc);
quint8 opcode = extractOpcode(raw);
InstructionFormat fmt = getInstructionFormatFromOpcode(opcode);

// Example 3: Logging/debugging
DEBUG_LOG(QString("Instruction at PC=0x%1 has format: %2")
    .arg(di.pc, 16, 16, QChar('0'))
    .arg(getInstructionFormatName(getInstructionFormat(di))));

// Example 4: Fast dispatch based on format
static void (*const dispatchTable[])(PipelineSlot&) = {
    [InstructionFormat::Memory]        = &executeMemory,
    [InstructionFormat::Branch]        = &executeBranch,
    [InstructionFormat::Operate]       = &executeOperate,
    [InstructionFormat::FloatingPoint] = &executeFloat,
    // ...
};

InstructionFormat fmt = getInstructionFormat(slot.di);
dispatchTable[static_cast<quint8>(fmt)](slot);

*/