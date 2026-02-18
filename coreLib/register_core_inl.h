#ifndef REGISTER_CORE_INL_H
#define REGISTER_CORE_INL_H
#include "coreLib/Axp_Attributes_core.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include <QtGlobal>

AXP_HOT AXP_ALWAYS_INLINE quint8 destRegister(const DecodedInstruction& di)  noexcept
{
    const quint8 opcode = extractOpcode(di.rawBits());

    // ================================================================
    // MEMORY FORMAT INSTRUCTIONS
    // ================================================================
    if (isMemoryFormat(di)) {
        if (di.semantics & S_Load) {
            return di.ra;  // Loads write to Ra (destination)
        }
        else {
            return 31;  // Stores don't write any register
        }
    }

    // ================================================================
    // ADDRESS COMPUTATION (LDA, LDAH)
    // Memory-format encoding but NOT memory operations
    // ================================================================
    if (opcode == 0x08 || opcode == 0x09) {  // LDA, LDAH
        return di.ra;
    }

    // ================================================================
    // INTEGER OPERATE FORMAT
    // ================================================================
    if (isOperateFormat(di)) {
        return di.rc;  // Result goes to Rc
    }

    // ================================================================
    // FLOATING-POINT OPERATE
    // ================================================================
    if (opcode >= 0x14 && opcode <= 0x17) {  // FP operate (0x14-0x17)
        return di.rc;  // Fc is in same position as Rc
    }

    // ================================================================
    // BRANCH WITH LINK (BSR, JSR)
    // ================================================================
    if (isBranchFormat(di)) {
        if (isWritesLinkRegister(di)) {
            return di.ra;  // Return address written to Ra
        }
        return 31;  // Conditional branches don't write
    }

    // ================================================================
    // JUMP FORMAT (JMP, JSR, RET, JSR_COROUTINE)
    // ================================================================
    if (isJumpFormat(di)) {
        if (isWritesLinkRegister(di)) {
            return di.ra;  // JSR, JSR_COROUTINE write return address
        }
        return 31;  // JMP, RET don't write
    }

    // ================================================================
    // SPECIAL INSTRUCTIONS
    // ================================================================
    switch (opcode) {
    case 0x18:  // Misc/HW instructions
    {
        const quint16 func = getFunctionCode(di);
        // HW_MFPR - Move from processor register to Ra
        if (func == 0x19) return di.ra;  // HW_MFPR
        // RPCC - Read process cycle counter to Ra
        if (func == 0xC000) return di.ra;  // RPCC
        // RC - Read and clear to Ra
        if (func == 0xE000) return di.ra;  // RC
        // RS - Read and set to Ra  
        if (func == 0xF000) return di.ra;  // RS
        break;
    }

    case 0x1A:  // JMP format (already handled above if isJumpFormat works)
        break;

    case 0x00:  // CALL_PAL
        return 31;  // PAL code handles any writes
    }

    // ================================================================
    // NO WRITEBACK
    // - Memory barriers (MB, WMB)
    // - Trap barrier (TRAPB)
    // - Prefetch (FETCH, FETCH_M)
    // - Conditional branches without link
    // - Stores
    // - NOP, etc.
    // ================================================================
    return 31;
}


AXP_HOT	AXP_ALWAYS_INLINE	bool destIsFloat(const DecodedInstruction& di) noexcept
{
    // FP register writes for FP ops and FP loads
    return isFloatFormat(di);
}

AXP_HOT	AXP_ALWAYS_INLINE	bool writesRegister(const DecodedInstruction& di) noexcept
{
    // Loads write a register
    if (isMemoryFormat(di) && isLoad(di)) return true;

    // Operate-format ALU/FP ops typically write Rc
    if (isOperateFormat(di)) return true;

    // Branch/jump family: only those that write a link register
    // NOTE: replace these predicates with your real decode helpers.
    if (isBranchFormat(di) && isWritesLinkRegister(di)) return true;

    return false;
}



#endif // REGISTER_CORE_INL_H
