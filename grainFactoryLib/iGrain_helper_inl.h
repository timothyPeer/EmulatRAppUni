#pragma once
#include <QtGlobal>
#include <QString>
#include "InstructionGrain_core.h"

// enum class InstrFormat {
// 	Operate,       // integer operate (non-MB)
// 	Memory,        // normal LD/ST/LDA/LDAH
// 	Branch,        // BR/BSR/BEQ/etc.
// 	Pal,           // CALL_PAL
// 	Float,         // FP arithmetic / FP compare
// 	MemoryMB,      // memory-with-function: FETCH/FETCH_M/WH64/MB/WMB/etc.
// 	Vec_Format,    // vector instructions (future use)
// 	JMP_JSR_Format, // // JMP, JSR, RET, JSR_COROUTINE
// 	Unknown
// };



/**
 * @brief Convert GrainType enum to ASCII string
 */
inline const char* getGrainTypeName(GrainType type) noexcept {
	switch (type) {
	case GrainType::IntegerOperate:    return "IntOp";
	case GrainType::FloatOperate:      return "FloatOp";
	case GrainType::Branch:            return "Branch";

	case GrainType::Jump:              return "Jump";
	case GrainType::ControlFlow:       return "Control";

	case GrainType::PALcode:           return "PAL";
	case GrainType::Miscellaneous:     return "Misc";

	default:                           return "UNKNOWN";
	}
}

/**
 * @brief Get instruction mnemonic from opcode
 * @param opcode Instruction opcode (bits 31:26)
 * @param functionCode Function code for operate/PAL instructions
 * @return QString mnemonic
 */
inline QString getInstructionMnemonic(quint8 opcode, quint16 functionCode = 0) noexcept
{
    switch (opcode)
    {
        // ========================================================================
        // CALL_PAL (0x00) - Function code determines specific PAL call
        // ========================================================================
    case 0x00:
        switch (functionCode & 0xFF) {
        case 0x00: return "HALT";
        case 0x01: return "CFLUSH";
        case 0x02: return "DRAINA";
        case 0x09: return "CSERVE";
        case 0x0A: return "SWPPAL";
        case 0x0D: return "WRIPIR";
        case 0x10: return "RDMCES";
        case 0x11: return "WRMCES";
        case 0x2B: return "WRFEN";
        case 0x2D: return "WRVPTPTR";
        case 0x30: return "SWPCTX";
        case 0x31: return "WRVAL";
        case 0x32: return "RDVAL";
        case 0x33: return "TBI";
        case 0x34: return "WRENT";
        case 0x35: return "SWPIPL";
        case 0x36: return "RDPS";
        case 0x37: return "WRKGP";
        case 0x38: return "WRUSO";
        case 0x39: return "WRPERFMON";
        case 0x3A: return "RDUSP";
        case 0x3C: return "WHAMI";
        case 0x3D: return "RETSYS";
        case 0x3E: return "WTINT";
        case 0x3F: return "RTI";
        case 0x80: return "BPT";
        case 0x81: return "BUGCHK";
        case 0x82: return "CHME";
        case 0x83: return "CHMK";
        case 0x84: return "CHMS";
        case 0x85: return "CHMU";
        case 0x86: return "IMB";
        case 0x9E: return "RDUNIQUE";
        case 0x9F: return "WRUNIQUE";
        case 0xAA: return "GENTRAP";
        default:   return QString("CALL_PAL_%1").arg(functionCode, 2, 16, QChar('0'));
        }

        // ========================================================================
        // Memory Format (0x08-0x0F, 0x20-0x2F)
        // ========================================================================
    case 0x08: return "LDA";      // Load address
    case 0x09: return "LDAH";     // Load address high
    case 0x0A: return "LDBU";     // Load byte unsigned
    case 0x0B: return "LDQ_U";    // Load quadword unaligned
    case 0x0C: return "LDWU";     // Load word unsigned
    case 0x0D: return "STW";      // Store word
    case 0x0E: return "STB";      // Store byte
    case 0x0F: return "STQ_U";    // Store quadword unaligned

    case 0x20: return "LDF";      // Load F_floating
    case 0x21: return "LDG";      // Load G_floating
    case 0x22: return "LDS";      // Load S_floating
    case 0x23: return "LDT";      // Load T_floating
    case 0x24: return "STF";      // Store F_floating
    case 0x25: return "STG";      // Store G_floating
    case 0x26: return "STS";      // Store S_floating
    case 0x27: return "STT";      // Store T_floating

    case 0x28: return "LDL";      // Load longword
    case 0x29: return "LDQ";      // Load quadword
    case 0x2A: return "LDL_L";    // Load longword locked
    case 0x2B: return "LDQ_L";    // Load quadword locked
    case 0x2C: return "STL";      // Store longword
    case 0x2D: return "STQ";      // Store quadword
    case 0x2E: return "STL_C";    // Store longword conditional
    case 0x2F: return "STQ_C";    // Store quadword conditional

        // ========================================================================
        // Integer Operate (0x10-0x13) - Function code determines operation
        // ========================================================================
    case 0x10:  // Integer arithmetic
        switch (functionCode & 0x7F) {
        case 0x00: return "ADDL";
        case 0x02: return "S4ADDL";
        case 0x09: return "SUBL";
        case 0x0B: return "S4SUBL";
        case 0x0F: return "CMPBGE";
        case 0x12: return "S8ADDL";
        case 0x1B: return "S8SUBL";
        case 0x1D: return "CMPULT";
        case 0x20: return "ADDQ";
        case 0x22: return "S4ADDQ";
        case 0x29: return "SUBQ";
        case 0x2B: return "S4SUBQ";
        case 0x2D: return "CMPEQ";
        case 0x32: return "S8ADDQ";
        case 0x3B: return "S8SUBQ";
        case 0x3D: return "CMPULE";
        case 0x40: return "ADDL/V";
        case 0x49: return "SUBL/V";
        case 0x4D: return "CMPLT";
        case 0x60: return "ADDQ/V";
        case 0x69: return "SUBQ/V";
        case 0x6D: return "CMPLE";
        default:   return QString("INTA_%1").arg(functionCode, 2, 16, QChar('0'));
        }

    case 0x11:  // Integer logical
        switch (functionCode & 0x7F) {
        case 0x00: return "AND";
        case 0x08: return "BIC";
        case 0x14: return "CMOVLBS";
        case 0x16: return "CMOVLBC";
        case 0x20: return "BIS";
        case 0x24: return "CMOVEQ";
        case 0x26: return "CMOVNE";
        case 0x28: return "ORNOT";
        case 0x40: return "XOR";
        case 0x44: return "CMOVLT";
        case 0x46: return "CMOVGE";
        case 0x48: return "EQV";
        case 0x61: return "AMASK";
        case 0x64: return "CMOVLE";
        case 0x66: return "CMOVGT";
        case 0x6C: return "IMPLVER";
        default:   return QString("INTL_%1").arg(functionCode, 2, 16, QChar('0'));
        }

    case 0x12:  // Integer shifts
        switch (functionCode & 0x7F) {
        case 0x02: return "MSKBL";
        case 0x06: return "EXTBL";
        case 0x0B: return "INSBL";
        case 0x12: return "MSKWL";
        case 0x16: return "EXTWL";
        case 0x1B: return "INSWL";
        case 0x22: return "MSKLL";
        case 0x26: return "EXTLL";
        case 0x2B: return "INSLL";
        case 0x30: return "ZAP";
        case 0x31: return "ZAPNOT";
        case 0x32: return "MSKQL";
        case 0x34: return "SRL";
        case 0x36: return "EXTQL";
        case 0x39: return "SLL";
        case 0x3B: return "INSQL";
        case 0x3C: return "SRA";
        case 0x52: return "MSKWH";
        case 0x57: return "INSWH";
        case 0x5A: return "EXTWH";
        case 0x62: return "MSKLH";
        case 0x67: return "INSLH";
        case 0x6A: return "EXTLH";
        case 0x72: return "MSKQH";
        case 0x77: return "INSQH";
        case 0x7A: return "EXTQH";
        default:   return QString("INTS_%1").arg(functionCode, 2, 16, QChar('0'));
        }

    case 0x13:  // Integer multiply
        switch (functionCode & 0x7F) {
        case 0x00: return "MULL";
        case 0x20: return "MULQ";
        case 0x30: return "UMULH";
        case 0x40: return "MULL/V";
        case 0x60: return "MULQ/V";
        default:   return QString("INTM_%1").arg(functionCode, 2, 16, QChar('0'));
        }

        // ========================================================================
        // Floating-Point Operate (0x14-0x17)
        // ========================================================================
    case 0x14: return "ITOFS";    // Integer to FP (simplified)
    case 0x15: return "FLTV";     // VAX floating
    case 0x16: return "FLTI";     // IEEE floating
    case 0x17: return "FLTL";     // FP control

        // ========================================================================
        // Miscellaneous (0x18)
        // ========================================================================
    case 0x18:
        switch (functionCode & 0xFFFF) {
        case 0x0000: return "TRAPB";
        case 0x0400: return "EXCB";
        case 0x4000: return "MB";
        case 0x4400: return "WMB";
        case 0x8000: return "FETCH";
        case 0xA000: return "FETCH_M";
        case 0xC000: return "RPCC";
        case 0xE000: return "RC";
        case 0xE800: return "ECB";
        case 0xF000: return "RS";
        case 0xF800: return "WH64";
        default:     return QString("MISC_%1").arg(functionCode, 4, 16, QChar('0'));
        }

        // ========================================================================
        // Jump Format (0x1A)
        // ========================================================================
    case 0x1A:
        switch ((functionCode >> 14) & 0x3) {
        case 0: return "JMP";
        case 1: return "JSR";
        case 2: return "RET";
        case 3: return "JSR_COROUTINE";
        }
        return "JMP";

        // ========================================================================
        // PAL Format (0x1B-0x1F) - Reserved/Unimplemented
        // ========================================================================
    case 0x1B: return "HW_MFPR";  // PAL19 - Read processor register
    case 0x1D: return "HW_MTPR";  // PAL1D - Write processor register
    case 0x1E: return "HW_REI";   // PAL1E - Return from exception/interrupt
    case 0x1F: return "HW_LD";    // PAL1F - Hardware load

        // ========================================================================
        // Branch Format (0x30-0x3F)
        // ========================================================================
    case 0x30: return "BR";       // Branch
    case 0x31: return "FBEQ";     // FP branch if equal
    case 0x32: return "FBLT";     // FP branch if less than
    case 0x33: return "FBLE";     // FP branch if less or equal
    case 0x34: return "BSR";      // Branch to subroutine
    case 0x35: return "FBNE";     // FP branch if not equal
    case 0x36: return "FBGE";     // FP branch if greater or equal
    case 0x37: return "FBGT";     // FP branch if greater than
    case 0x38: return "BLBC";     // Branch if low bit clear
    case 0x39: return "BEQ";      // Branch if equal
    case 0x3A: return "BLT";      // Branch if less than
    case 0x3B: return "BLE";      // Branch if less or equal
    case 0x3C: return "BLBS";     // Branch if low bit set
    case 0x3D: return "BNE";      // Branch if not equal
    case 0x3E: return "BGE";      // Branch if greater or equal
    case 0x3F: return "BGT";      // Branch if greater than

        // ========================================================================
        // Unknown/Invalid
        // ========================================================================
    default:
        return QString("UNKNOWN_%1").arg(opcode, 2, 16, QChar('0'));
    }
}

/**
 * @brief Get instruction mnemonic from raw instruction bits
 * @param rawBits Raw 32-bit instruction word
 * @return QString instruction mnemonic
 */
 inline   QString getMnemonicFromRaw(const quint32 rawBits)  noexcept
{
    // Extract opcode (bits 31:26)
    quint8 opcode = (rawBits >> 26) & 0x3F;

    // Extract function code based on opcode type
    quint16 functionCode = 0;

    if (opcode >= 0x10 && opcode <= 0x13) {
        // Operate format: function in bits [11:5] (7 bits)
        functionCode = (rawBits >> 5) & 0x7F;
    }
    else if (opcode == 0x00) {
        // CALL_PAL: PAL function in bits [7:0] (8 bits)
        functionCode = rawBits & 0xFF;
    }
    else if (opcode == 0x18) {
        // Misc format: function in bits [15:0] (16 bits)
        functionCode = rawBits & 0xFFFF;
    }
    else if (opcode == 0x1A) {
        // Jump format: hint in bits [15:14] (2 bits)
        functionCode = (rawBits >> 14) & 0x3;
    }

    return getInstructionMnemonic(opcode, functionCode);
}