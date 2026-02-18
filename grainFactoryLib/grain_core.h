#ifndef GRAIN_CORE_H
#define GRAIN_CORE_H
#include <QtGlobal>
#include "../coreLib/Axp_Attributes_core.h"

// ============================================================================
// Bit Layout Analysis
// ============================================================================
//
// semantics (64 bits):
//   Bits 0-23:  Semantic flags (S_Load, S_Store, etc.)
//   Bits 24-31: RESERVED (was literal, now in separate byte)
//   Bits 32-63: RAW INSTRUCTION BITS <- PACK HERE!
//
// This works because:
//   - Semantic flags only use ~24 bits
//   - We have 40 bits unused in upper portion
//   - Raw instruction is exactly 32 bits
//   - Perfect fit!

// ============================================================================
// Raw Instruction Packing
// ============================================================================

static constexpr quint64 RAW_SHIFT = 32;
static constexpr quint64 RAW_MASK = 0xFFFFFFFF00000000ULL;
static constexpr quint64 SEMANTICS_MASK = 0x00000000FFFFFFFFULL;



// Integers  Opcode 10
enum class IntOperationKind10 : quint8
{
    ADDQ,       // ADDQ   (function 0x60)  – 64–bit add
    CMPBGE,     // CMPBGE (function 0x0F)  – compare byte, greater or equal
    CMPEQ,      // CMPEQ  (function 0x2D)  – compare equal
    CMPLE,      // CMPLE  (function 0x6D)  – compare less or equal (signed)
    CMPLT,      // CMPLT  (function 0x4D)  – compare less than (signed)
    CMPULE,     // CMPULE (function 0x3D)  – compare unsigned less or equal
    CMPULT,     // CMPULT (function 0x1D)  – compare unsigned less than

    S4ADDL,     // S4ADDL (function 0x02)  – add longword, shift left by 2
    S4ADDQ,     // S4ADDQ (function 0x22)  – add quadword, shift left by 2
    S4SUBL,     // S4SUBL (function 0x0B)  – subtract longword, shift left by 2
    S4SUBQ,     // S4SUBQ (function 0x2B)  – subtract quadword, shift left by 2

    S8ADDL,     // S8ADDL (function 0x12)  – add longword, shift left by 3
    S8ADDQ,     // S8ADDQ (function 0x32)  – add quadword, shift left by 3
    S8SUBL,     // S8SUBL (function 0x1B)  – subtract longword, shift left by 3
    S8SUBQ,     // S8SUBQ (function 0x3B)  – subtract quadword, shift left by 3

    SUBL,       // SUBL   (function 0x09)  – subtract longword
    SUBL_V,     // SUBL/V (function 0x49)  – subtract longword with overflow trap
    SUBQ,       // SUBQ   (function 0x29)  – subtract quadword
    SUBQ_V      // SUBQ/V (function 0x69)  – subtract quadword with overflow trap
};

AXP_HOT AXP_ALWAYS_INLINE 
    quint16 intOperate10_functionCode(IntOperationKind10 kind) noexcept
{
    switch (kind)
    {
    case IntOperationKind10::ADDQ:      return 0x60; // ADDQ
    case IntOperationKind10::CMPBGE:    return 0x0F; // CMPBGE
    case IntOperationKind10::CMPEQ:     return 0x2D; // CMPEQ
    case IntOperationKind10::CMPLE:     return 0x6D; // CMPLE
    case IntOperationKind10::CMPLT:     return 0x4D; // CMPLT
    case IntOperationKind10::CMPULE:    return 0x3D; // CMPULE
    case IntOperationKind10::CMPULT:    return 0x1D; // CMPULT

    case IntOperationKind10::S4ADDL:    return 0x02; // S4ADDL
    case IntOperationKind10::S4ADDQ:    return 0x22; // S4ADDQ
    case IntOperationKind10::S4SUBL:    return 0x0B; // S4SUBL
    case IntOperationKind10::S4SUBQ:    return 0x2B; // S4SUBQ

    case IntOperationKind10::S8ADDL:    return 0x12; // S8ADDL
    case IntOperationKind10::S8ADDQ:    return 0x32; // S8ADDQ
    case IntOperationKind10::S8SUBL:    return 0x1B; // S8SUBL
    case IntOperationKind10::S8SUBQ:    return 0x3B; // S8SUBQ

    case IntOperationKind10::SUBL:      return 0x09; // SUBL
    case IntOperationKind10::SUBL_V:    return 0x49; // SUBL/V
    case IntOperationKind10::SUBQ:      return 0x29; // SUBQ
    case IntOperationKind10::SUBQ_V:    return 0x69; // SUBQ/V
    }

    // Should never reach here – but keep the compiler happy.
    return 0;
}

// Integer Opcode 11

enum class IntOperationKind11 : quint8
{
	And, Bic, CMOVLBS, CMOVLBC,
	BIS, CMOVEQ, CMOVNE, ORNOT,
	XOR, CMOVLT, CMOVGE, EQV,
	CMOVLE, CMOVGT
};

static inline quint8 intOperate11_functionCode(IntOperationKind11 kind) noexcept
{
	switch (kind)
	{
	case IntOperationKind11::And:      return 0x00;
	case IntOperationKind11::Bic:      return 0x08;
	case IntOperationKind11::CMOVLBS:  return 0x14;
	case IntOperationKind11::CMOVLBC:  return 0x16;
	case IntOperationKind11::BIS:      return 0x20;
	case IntOperationKind11::CMOVEQ:   return 0x24;
	case IntOperationKind11::CMOVNE:   return 0x26;
	case IntOperationKind11::ORNOT:    return 0x28;
	case IntOperationKind11::XOR:      return 0x40;
	case IntOperationKind11::CMOVLT:   return 0x44;
	case IntOperationKind11::CMOVGE:   return 0x46;
	case IntOperationKind11::EQV:      return 0x48;
	case IntOperationKind11::CMOVLE:   return 0x64;
	case IntOperationKind11::CMOVGT:   return 0x66;
	default:                           return 0x00;
	}
}

// Opcode 12

enum class IntOperationKind12 : quint8
{
	MSKBL, EXTBL, MSKWL, EXTWL, MSKLL, EXTLL,
	ZAP, ZAPNOT, MSKQL, EXTQL,
	SRL, SLL, SRA,
	MSKWH, INSWH, MSKLH, INSLH, MSKQH, INSQH,
	INSBL, INSWL, INSLL, INSQL,
	EXTWH, EXTLH, EXTQH
};

static inline quint8 intOperate12_functionCode(IntOperationKind12 k) noexcept
{
	switch (k)
	{
	case IntOperationKind12::MSKBL:   return 0x02;
	case IntOperationKind12::EXTBL:   return 0x06;
	case IntOperationKind12::MSKWL:   return 0x12;
	case IntOperationKind12::EXTWL:   return 0x16;
	case IntOperationKind12::MSKLL:   return 0x22;
	case IntOperationKind12::EXTLL:   return 0x26;
	case IntOperationKind12::ZAP:     return 0x30;
	case IntOperationKind12::ZAPNOT:  return 0x31;
	case IntOperationKind12::MSKQL:   return 0x32;
	case IntOperationKind12::EXTQL:   return 0x36;
	case IntOperationKind12::SRL:     return 0x34;
	case IntOperationKind12::SLL:     return 0x39;
	case IntOperationKind12::SRA:     return 0x3C;
	case IntOperationKind12::MSKWH:   return 0x52;
	case IntOperationKind12::INSWH:   return 0x57;
	case IntOperationKind12::MSKLH:   return 0x62;
	case IntOperationKind12::INSLH:   return 0x67;
	case IntOperationKind12::MSKQH:   return 0x72;
	case IntOperationKind12::INSQH:   return 0x77;
	case IntOperationKind12::INSBL:   return 0x0B;
	case IntOperationKind12::INSWL:   return 0x1B;
	case IntOperationKind12::INSLL:   return 0x2B;
	case IntOperationKind12::INSQL:   return 0x3B;
	case IntOperationKind12::EXTWH:   return 0x5A;
	case IntOperationKind12::EXTLH:   return 0x6A;
	case IntOperationKind12::EXTQH:   return 0x7A;
	default: return 0x00;
	}
}

// Opcode 13

enum class IntOperationKind13 : quint8
{
	MULL, MULQ, UMULH, MULLN, MULQN
};

static inline quint8 intOperate13_functionCode(IntOperationKind13 k) noexcept
{
	switch (k)
	{
	case IntOperationKind13::MULL:   return 0x00;
	case IntOperationKind13::MULQ:   return 0x20;
	case IntOperationKind13::UMULH:  return 0x30;
	case IntOperationKind13::MULLN:  return 0x40;
	case IntOperationKind13::MULQN:  return 0x60;
	default:                         return 0x00;
	}
}



// Opcode 14

enum class FPOperationKind14 : quint8
{
    CVTGQ,
    SQRTF_UC, SQRTS_UC, SQRTG_UC, SQRTT_UC,
    SQRTS_UM, SQRTT_UM,
    SQRTF_U, SQRTS_U, SQRTG_U, SQRTT_U,
    SQRTS_UD, SQRTT_UD,
    SQRTF_SC, SQRTG_SC,
    SQRTF_S, SQRTG_S,
    SQRTF_SUC, SQRTS_SUC, SQRTG_SUC, SQRTT_SUC,
    SQRTS_SUM, SQRTT_SUM,
    SQRTF_SU, SQRTS_SU, SQRTG_SU, SQRTT_SU,
    SQRTS_SUD, SQRTT_SUD,
    SQRTS_SUIC, SQRTT_SUIC,
    SQRTS_SUIM, SQRTT_SUIM,
    SQRTS_SUI, SQRTT_SUI,
    SQRTS_SUID, SQRTT_SUID,
    SQRTS, SQRTT, SQRTG, SQRTF
};

static inline quint16 fpOperate14(FPOperationKind14 k) noexcept
{
    switch (k)
    {
    case FPOperationKind14::CVTGQ:       return 0x0AF;
    case FPOperationKind14::SQRTF_UC:    return 0x10A;
    case FPOperationKind14::SQRTS_UC:    return 0x10B;
    case FPOperationKind14::SQRTG_UC:    return 0x12A;
    case FPOperationKind14::SQRTT_UC:    return 0x12B;
    case FPOperationKind14::SQRTS_UM:    return 0x14B;
    case FPOperationKind14::SQRTF_U:     return 0x16B;
    case FPOperationKind14::SQRTS_U:     return 0x18A;
    case FPOperationKind14::SQRTG_U:     return 0x1AA;
    case FPOperationKind14::SQRTT_U:     return 0x1AB;
    case FPOperationKind14::SQRTT_UD:    return 0x1CB;
    case FPOperationKind14::SQRTS_UD:    return 0x1EB;
    case FPOperationKind14::SQRTF_SC:    return 0x40A;
    case FPOperationKind14::SQRTG_SC:    return 0x42A;
    case FPOperationKind14::SQRTF_S:     return 0x48A;
    case FPOperationKind14::SQRTG_S:     return 0x4AA;
    case FPOperationKind14::SQRTF_SUC:   return 0x50A;
    case FPOperationKind14::SQRTS_SUC:   return 0x50B;
    case FPOperationKind14::SQRTG_SUC:   return 0x52A;
    case FPOperationKind14::SQRTT_SUC:   return 0x52B;
    case FPOperationKind14::SQRTT_SUM:   return 0x54B;
    case FPOperationKind14::SQRTS_SUM:   return 0x56B;
    case FPOperationKind14::SQRTF_SU:    return 0x58A;
    case FPOperationKind14::SQRTS_SU:    return 0x58B;
    case FPOperationKind14::SQRTG_SU:    return 0x5AA;
    case FPOperationKind14::SQRTT_SU:    return 0x5AB;
    case FPOperationKind14::SQRTT_SUD:   return 0x5CB;
    case FPOperationKind14::SQRTS_SUD:   return 0x5EB;
    case FPOperationKind14::SQRTT_SUIC:  return 0x70B;
    case FPOperationKind14::SQRTS_SUIC:  return 0x72B;
    case FPOperationKind14::SQRTT_SUIM:  return 0x74B;
    case FPOperationKind14::SQRTS_SUIM:  return 0x76B;
    case FPOperationKind14::SQRTT_SUI:   return 0x78B;
    case FPOperationKind14::SQRTS_SUI:   return 0x7AB;
    case FPOperationKind14::SQRTT_SUID:  return 0x7CB;
    case FPOperationKind14::SQRTS_SUID:  return 0x7EB;
	case FPOperationKind14::SQRTT:  return 0x0AB;
	case FPOperationKind14::SQRTS:  return 0x08B;
	case FPOperationKind14::SQRTG:  return 0x0AB;
	case FPOperationKind14::SQRTF:  return 0x08A;
    default: return 0x0000;
    }
}

//Opcode 15
// ============================================================================
//  FPBaseOpKind - Base operation classification
// ============================================================================
enum class FPBaseOpKind15 : quint8
{
    Add, Sub, Mul, Div, Cmp, Cvt
};

// ============================================================================
//  FPOperationKind15 - All opcode 15 variants
// ============================================================================
enum class FPOperationKind15 : quint8
{
    ADDF, SUBF, MULF, DIVF,
    ADDF_IC, SUBF_IC, MULF_IC, DIVF_IC,
    CVTDG_IC, ADDG_IC, SUBG_IC, MULG_IC, DIVG_IC,
    CVTGF_IC, CVTGD_IC, CVTGQ_IC, CVTBQ_C,
    CVTQF_IC, CVTQG_IC,
    CVTDG, ADDG, SUBG, MULG, DIVG,
    CMPGEQ, CMPGLT, CMPGLE,
    CVTGF, CVTGD, CVTGQ, CVTQF, CVTQG,
    ADDF_UC, SUBF_UC, MULF_UC, DIVF_UC,
    CVTDG_UC, ADDG_UC, SUBG_UC, MULG_UC, DIVG_UC,
    CVTGF_UC, CVTGD_UC, CVTGQ_NC,
    ADDF_U, SUBF_U, MULF_U, DIVF_U,
    CVTDG_U, ADDG_U, SUBG_U, MULG_U, DIVG_U,
    CVTGF_U, CVTGD_U, CVTGQ_V,
    ADDF_SC, SUBF_SC, MULF_SC, DIVF_SC,
    CVTDG_SC, ADDG_SC, SUBG_SC, MULG_SC, DIVG_SC,
    CVTGF_SC, CVTGD_SC, CVTGQ_SC,
    ADDF_IS, SUBF_IS, MULF_IS, DIVF_IS,
    CVTDG_IS, ADDG_IS, SUBG_IS, MULG_IS, DIVG_IS,
    CMPGEQ_IC, CMPGLT_IC, CMPGLE_IC,
    CVTGF_IS, CVTGD_IS, CVTGQ_IS,
    ADDF_SUC, SUBF_SUC, MULF_SUC, DIVF_SUC,
    CVTDG_SUC, ADDG_SUC, SUBG_SUC, MULG_SUC, DIVG_SUC,
    CVTGF_SUC, CVTGD_SUC, CVTGQ_SVC,
    ADDF_SU, SUBF_SU, MULF_SU, DIVF_SU,
    CVTDG_SU, ADDG_SU, SUBG_SU, MULG_SU, DIVG_SU,
    CVTGF_SU, CVTGD_SU, CVTGQ_SV, CVTQG_C, CVTGQ_VC, CVTQF_C
};

// ============================================================================
//  Function code mapping (unchanged from your version)
// ============================================================================
static inline quint16 fpOperate15(FPOperationKind15 k) noexcept
{
    switch (k)
    {
    case FPOperationKind15::ADDF:        return 0x080;
    case FPOperationKind15::SUBF:        return 0x081;
    case FPOperationKind15::MULF:        return 0x082;
    case FPOperationKind15::DIVF:        return 0x083;
    case FPOperationKind15::ADDF_IC:     return 0x000;
    case FPOperationKind15::SUBF_IC:     return 0x001;
    case FPOperationKind15::MULF_IC:     return 0x002;
    case FPOperationKind15::DIVF_IC:     return 0x003;
    case FPOperationKind15::CVTDG_IC:    return 0x01E;
    case FPOperationKind15::ADDG_IC:     return 0x020;
    case FPOperationKind15::SUBG_IC:     return 0x021;
    case FPOperationKind15::MULG_IC:     return 0x022;
    case FPOperationKind15::DIVG_IC:     return 0x023;
    case FPOperationKind15::CVTGF_IC:    return 0x02C;
    case FPOperationKind15::CVTGD_IC:    return 0x02D;
    case FPOperationKind15::CVTGQ_IC:    return 0x02F;
    case FPOperationKind15::CVTBQ_C:     return 0x02F;
    case FPOperationKind15::CVTQF_IC:    return 0x03C;
    case FPOperationKind15::CVTQG_IC:    return 0x03E;
    case FPOperationKind15::CVTDG:       return 0x09E;
    case FPOperationKind15::ADDG:        return 0x0A0;
    case FPOperationKind15::SUBG:        return 0x0A1;
    case FPOperationKind15::MULG:        return 0x0A2;
    case FPOperationKind15::DIVG:        return 0x0A3;
    case FPOperationKind15::CMPGEQ:      return 0x0A5;
    case FPOperationKind15::CMPGLT:      return 0x0A6;
    case FPOperationKind15::CMPGLE:      return 0x0A7;
    case FPOperationKind15::CVTGF:       return 0x0AC;
    case FPOperationKind15::CVTGD:       return 0x0AD;
    case FPOperationKind15::CVTGQ:       return 0x0AF;
    case FPOperationKind15::CVTQF:       return 0x0BC;
    case FPOperationKind15::CVTQG:       return 0x0BE;
    case FPOperationKind15::ADDF_UC:     return 0x100;
    case FPOperationKind15::SUBF_UC:     return 0x101;
    case FPOperationKind15::MULF_UC:     return 0x102;
    case FPOperationKind15::DIVF_UC:     return 0x103;
    case FPOperationKind15::CVTDG_UC:    return 0x11E;
    case FPOperationKind15::ADDG_UC:     return 0x120;
    case FPOperationKind15::SUBG_UC:     return 0x121;
    case FPOperationKind15::MULG_UC:     return 0x122;
    case FPOperationKind15::DIVG_UC:     return 0x123;
    case FPOperationKind15::CVTGF_UC:    return 0x12C;
    case FPOperationKind15::CVTGD_UC:    return 0x12D;
    case FPOperationKind15::CVTGQ_NC:    return 0x12F;
    case FPOperationKind15::ADDF_U:      return 0x180;
    case FPOperationKind15::SUBF_U:      return 0x181;
    case FPOperationKind15::MULF_U:      return 0x182;
    case FPOperationKind15::DIVF_U:      return 0x183;
    case FPOperationKind15::CVTDG_U:     return 0x19E;
    case FPOperationKind15::ADDG_U:      return 0x1A0;
    case FPOperationKind15::SUBG_U:      return 0x1A1;
    case FPOperationKind15::MULG_U:      return 0x1A2;
    case FPOperationKind15::DIVG_U:      return 0x1A3;
    case FPOperationKind15::CVTGF_U:     return 0x1AC;
    case FPOperationKind15::CVTGD_U:     return 0x1AD;
    case FPOperationKind15::CVTGQ_V:     return 0x1AF;
    case FPOperationKind15::ADDF_SC:     return 0x400;
    case FPOperationKind15::SUBF_SC:     return 0x401;
    case FPOperationKind15::MULF_SC:     return 0x402;
    case FPOperationKind15::DIVF_SC:     return 0x403;
    case FPOperationKind15::CVTDG_SC:    return 0x41E;
    case FPOperationKind15::ADDG_SC:     return 0x420;
    case FPOperationKind15::SUBG_SC:     return 0x421;
    case FPOperationKind15::MULG_SC:     return 0x422;
    case FPOperationKind15::DIVG_SC:     return 0x423;
    case FPOperationKind15::CVTGF_SC:    return 0x42C;
    case FPOperationKind15::CVTGD_SC:    return 0x42D;
    case FPOperationKind15::CVTGQ_SC:    return 0x42F;
    case FPOperationKind15::ADDF_IS:     return 0x480;
    case FPOperationKind15::SUBF_IS:     return 0x481;
    case FPOperationKind15::MULF_IS:     return 0x482;
    case FPOperationKind15::DIVF_IS:     return 0x483;
    case FPOperationKind15::CVTDG_IS:    return 0x49E;
    case FPOperationKind15::ADDG_IS:     return 0x4A0;
    case FPOperationKind15::SUBG_IS:     return 0x4A1;
    case FPOperationKind15::MULG_IS:     return 0x4A2;
    case FPOperationKind15::DIVG_IS:     return 0x4A3;
    case FPOperationKind15::CMPGEQ_IC:   return 0x4A5;
    case FPOperationKind15::CMPGLT_IC:   return 0x4A6;
    case FPOperationKind15::CMPGLE_IC:   return 0x4A7;
    case FPOperationKind15::CVTGF_IS:    return 0x4AC;
    case FPOperationKind15::CVTGD_IS:    return 0x4AD;
    case FPOperationKind15::CVTGQ_IS:    return 0x4AF;
    case FPOperationKind15::ADDF_SUC:    return 0x500;
    case FPOperationKind15::SUBF_SUC:    return 0x501;
    case FPOperationKind15::MULF_SUC:    return 0x502;
    case FPOperationKind15::DIVF_SUC:    return 0x503;
    case FPOperationKind15::CVTDG_SUC:   return 0x51E;
    case FPOperationKind15::ADDG_SUC:    return 0x520;
    case FPOperationKind15::SUBG_SUC:    return 0x521;
    case FPOperationKind15::MULG_SUC:    return 0x522;
    case FPOperationKind15::DIVG_SUC:    return 0x523;
    case FPOperationKind15::CVTGF_SUC:   return 0x52C;
    case FPOperationKind15::CVTGD_SUC:   return 0x52D;
    case FPOperationKind15::CVTGQ_SVC:   return 0x52F;
    case FPOperationKind15::ADDF_SU:     return 0x580;
    case FPOperationKind15::SUBF_SU:     return 0x581;
    case FPOperationKind15::MULF_SU:     return 0x582;
    case FPOperationKind15::DIVF_SU:     return 0x583;
    case FPOperationKind15::CVTDG_SU:    return 0x59E;
    case FPOperationKind15::ADDG_SU:     return 0x5A0;
    case FPOperationKind15::SUBG_SU:     return 0x5A1;
    case FPOperationKind15::MULG_SU:     return 0x5A2;
    case FPOperationKind15::DIVG_SU:     return 0x5A3;
    case FPOperationKind15::CVTGF_SU:    return 0x5AC;
    case FPOperationKind15::CVTGD_SU:    return 0x5AD;
    case FPOperationKind15::CVTGQ_SV:    return 0x5AF;
    default:                             return 0x000;
    }
}

// ============================================================================
//  Classify operation into base type
// ============================================================================
static AXP_ALWAYS_INLINE FPBaseOpKind15 classifyBaseOp(FPOperationKind15 k) noexcept
{
    switch (k)
    {
        // ====================================================================
        // Add family (ADDF/ADDG variants)
        // ====================================================================
    case FPOperationKind15::ADDF:
    case FPOperationKind15::ADDF_IC:
    case FPOperationKind15::ADDF_UC:
    case FPOperationKind15::ADDF_U:
    case FPOperationKind15::ADDF_SC:
    case FPOperationKind15::ADDF_IS:
    case FPOperationKind15::ADDF_SUC:
    case FPOperationKind15::ADDF_SU:
    case FPOperationKind15::ADDG:
    case FPOperationKind15::ADDG_IC:
    case FPOperationKind15::ADDG_UC:
    case FPOperationKind15::ADDG_U:
    case FPOperationKind15::ADDG_SC:
    case FPOperationKind15::ADDG_IS:
    case FPOperationKind15::ADDG_SUC:
    case FPOperationKind15::ADDG_SU:
        return FPBaseOpKind15::Add;

        // ====================================================================
        // Sub family (SUBF/SUBG variants)
        // ====================================================================
    case FPOperationKind15::SUBF:
    case FPOperationKind15::SUBF_IC:
    case FPOperationKind15::SUBF_UC:
    case FPOperationKind15::SUBF_U:
    case FPOperationKind15::SUBF_SC:
    case FPOperationKind15::SUBF_IS:
    case FPOperationKind15::SUBF_SUC:
    case FPOperationKind15::SUBF_SU:
    case FPOperationKind15::SUBG:
    case FPOperationKind15::SUBG_IC:
    case FPOperationKind15::SUBG_UC:
    case FPOperationKind15::SUBG_U:
    case FPOperationKind15::SUBG_SC:
    case FPOperationKind15::SUBG_IS:
    case FPOperationKind15::SUBG_SUC:
    case FPOperationKind15::SUBG_SU:
        return FPBaseOpKind15::Sub;

        // ====================================================================
        // Mul family (MULF/MULG variants)
        // ====================================================================
    case FPOperationKind15::MULF:
    case FPOperationKind15::MULF_IC:
    case FPOperationKind15::MULF_UC:
    case FPOperationKind15::MULF_U:
    case FPOperationKind15::MULF_SC:
    case FPOperationKind15::MULF_IS:
    case FPOperationKind15::MULF_SUC:
    case FPOperationKind15::MULF_SU:
    case FPOperationKind15::MULG:
    case FPOperationKind15::MULG_IC:
    case FPOperationKind15::MULG_UC:
    case FPOperationKind15::MULG_U:
    case FPOperationKind15::MULG_SC:
    case FPOperationKind15::MULG_IS:
    case FPOperationKind15::MULG_SUC:
    case FPOperationKind15::MULG_SU:
        return FPBaseOpKind15::Mul;

        // ====================================================================
        // Div family (DIVF/DIVG variants)
        // ====================================================================
    case FPOperationKind15::DIVF:
    case FPOperationKind15::DIVF_IC:
    case FPOperationKind15::DIVF_UC:
    case FPOperationKind15::DIVF_U:
    case FPOperationKind15::DIVF_SC:
    case FPOperationKind15::DIVF_IS:
    case FPOperationKind15::DIVF_SUC:
    case FPOperationKind15::DIVF_SU:
    case FPOperationKind15::DIVG:
    case FPOperationKind15::DIVG_IC:
    case FPOperationKind15::DIVG_UC:
    case FPOperationKind15::DIVG_U:
    case FPOperationKind15::DIVG_SC:
    case FPOperationKind15::DIVG_IS:
    case FPOperationKind15::DIVG_SUC:
    case FPOperationKind15::DIVG_SU:
        return FPBaseOpKind15::Div;

        // ====================================================================
        // Cmp family (CMPG variants - G_floating compare)
        // ====================================================================
    case FPOperationKind15::CMPGEQ:       // Equal
    case FPOperationKind15::CMPGLT:       // Less than
    case FPOperationKind15::CMPGLE:       // Less than or equal
    case FPOperationKind15::CMPGEQ_IC:
    case FPOperationKind15::CMPGLT_IC:
    case FPOperationKind15::CMPGLE_IC:
        return FPBaseOpKind15::Cmp;

        // ====================================================================
        // Cvt family - VAX FP conversions (opcode 0x15)
        // ====================================================================

        // F_floating (32-bit VAX) conversions
    case FPOperationKind15::CVTDG:        // D_floating to G_floating
    case FPOperationKind15::CVTDG_IC:
    case FPOperationKind15::CVTDG_UC:
    case FPOperationKind15::CVTDG_U:
    case FPOperationKind15::CVTDG_SC:
    case FPOperationKind15::CVTDG_IS:
    case FPOperationKind15::CVTDG_SUC:
    case FPOperationKind15::CVTDG_SU:

        // G_floating to F_floating
    case FPOperationKind15::CVTGF:        // G_floating to F_floating
    case FPOperationKind15::CVTGF_IC:
    case FPOperationKind15::CVTGF_UC:
    case FPOperationKind15::CVTGF_U:
    case FPOperationKind15::CVTGF_SC:
    case FPOperationKind15::CVTGF_IS:
    case FPOperationKind15::CVTGF_SUC:
    case FPOperationKind15::CVTGF_SU:

        // G_floating to Quadword (integer)
    case FPOperationKind15::CVTGQ:        // G_floating to Quadword
    case FPOperationKind15::CVTGQ_IC:
    case FPOperationKind15::CVTGQ_VC:     // /V variant (trap on overflow)
    case FPOperationKind15::CVTGQ_SC:
    case FPOperationKind15::CVTGQ_SVC:    // /SV variant

        // Quadword (integer) to F_floating
    case FPOperationKind15::CVTQF:        // Quadword to F_floating
    case FPOperationKind15::CVTQF_IC:
    case FPOperationKind15::CVTQF_C:

        // Quadword (integer) to G_floating
    case FPOperationKind15::CVTQG:        // Quadword to G_floating
    case FPOperationKind15::CVTQG_IC:
    case FPOperationKind15::CVTQG_C:

        return FPBaseOpKind15::Cvt;

        // ====================================================================
        // Default - shouldn't happen if enum is complete
        // ====================================================================
    default:
        return FPBaseOpKind15::Cvt;
    }
}

// Opcode 16

// ============================================================================
//  FPOperationKind16 - Opcode 16 function variants
//
//  NOTE:
//    - Duplicate enum entries from your original header (ADDS_SU, ADDS_SUIM,
//      ADDS_SUI, ADDS_D, ADDS_UD, ADDS_SUD, ADDS_SUID) have been removed.
//    - All unique names and their function codes (below) are preserved.
//
// ============================================================================

enum class FPOperationKind16 : quint16  // NOLINT(performance-enum-size)
{
    SUBS_C,
    MULS_C,
    DIVS_C,
    SUBT_C,
    MULT_C,
    DIVT_C,
    ADDS_M,
    SUBS_M,
    MULS_M,
    DIVS_M,
    ADDT_M,
    SUBT_M,
    MULT_M,
    DIVT_M,
    ADDS,
    SUBS,
    MULS,
    DIVS,
    ADDS_UC,
    SUBS_UC,
    MULS_UC,
    DIVS_UC,
    ADDT_UC,
    SUBT_UC,
    MULT_UC,
    DIVT_UC,
    ADDS_UM,
    SUBS_UM,
    MULS_UM,
    DIVS_UM,
    ADDT_UM,
    SUBT_UM,
    MULT_UM,
    DIVT_UM,
    ADDS_U,
    SUBS_U,
    MULS_U,
    DIVS_U,
    ADDS_SUC,
    SUBS_SUC,
    MULS_SUC,
    DIVS_SUC,
    ADDT_SUC,
    SUBT_SUC,
    MULT_SUC,
    DIVT_SUC,
    ADDS_SUM,
    SUBS_SUM,
    MULS_SUM,
    DIVS_SUM,
    ADDT_SUM,
    SUBT_SUM,
    MULT_SUM,
    DIVT_SUM,
    ADDS_SU,
    SUBS_SU,
    MULS_SU,
    DIVS_SU,
    ADDS_SUIC,
    SUBS_SUIC,
    MULS_SUIC,
    DIVS_SUIC,
    ADDT_SUIC,
    SUBT_SUIC,
    MULT_SUIC,
    DIVT_SUIC,
    ADDS_SUIM,
    SUBS_SUIM,
    MULS_SUIM,
    DIVS_SUIM,
    ADDT_SUIM,
    SUBT_SUIM,
    MULT_SUIM,
    DIVT_SUIM,
    ADDS_SUI,
    SUBS_SUI,
    MULS_SUI,
    DIVS_SUI,
    ADDS_C,
    ADDT_C,
    CVTTS_C,
    CVTTS_D,
    CVTTQ_C,
    CVTQS_C,
    CVTQT_C,
    CVTTS_M,
    CVTTQ_M,
    CVTQS_M,
    CVTQT_M,
    CVTQT_D,
    CVTQT_SUIC,
    CVTQT_SUIM,
    CVTQT_SUID,
    ADDT,
    SUBT,
    MULT,
    DIVT,
    CMPTUN,
    CMPTEQ,
    CMPTLT,
    CMPTLE,
    CVTTS,
    CVTTQ,
    CVTQS,
    CVTQT,
    CVTQT_SU,
    ADDS_D,
    SUBS_ID,
    MULS_ID,
    DIVS_ID,
    ADDT_D,
    SUBT_D,
    SUBT_ID,
    MULT_D,
    MULT_ID,
    DIVT_D,
    DIVT_ID,
    CVTTS_ID,
    CVTTQ_D,
    CVTQS_D,
    CVTQT_ID,
    CVTTS_UC,
    CVTTQ_VC,
    CVTTS_UM,
    CVTTQ_VM,
    ADDT_U,
    SUBT_U,
    MULT_U,
    DIVT_U,
    CVTTS_U,
    CVTTQ_V,
    CVTBQ_V,
    ADDS_UD,
    SUBS_UD,
    MULS_UD,
    DIVS_UD,
    ADDT_UD,
    SUBT_UD,
    MULT_UD,
    DIVT_UD,
    CVTTS_UD,
    CVTTQ_VD,
    CVTST,
    CVTTS_SUC,
    CVTTQ_SVC,
    CVTTS_SUM,
    CVTTQ_SVM,
    ADDT_SU,
    SUBT_SU,
    MULT_SU,
    DIVT_SU,
    CMPTUN_SU,
    CMPTEQ_SU,
    CMPTLT_SU,
    CMPTLE_SU,
    CVTTS_SU,
    CVTTQ_SV,
    ADDS_SUD,
    SUBS_SUD,
    MULS_SUD,
    DIVS_SUD,
    ADDT_SUD,
    SUBT_SUD,
    MULT_SUD,
    DIVT_SUD,
    CVTTS_SUD,
    CVTTQ_SVD,
    CVTST_S,
    CVTTS_SUIC,
    CVTTQ_SVIC,
    CVTQS_SUC,
    CVTQT_SUC,
    CVTTS_SUIM,
    CVTTQ_SVIM,
    CVTQS_SUM,
    CVTQT_SUM,
    ADDT_SUI,
    SUBT_SUI,
    MULT_SUI,
    DIVT_SUI,
    CVTTS_SUI,
    CVTTQ_SVI,
    CVTQS_SU,
    CVTQT_SUI,
    ADDS_SUID,
    SUBS_SUID,
    MULS_SUID,
    DIVS_SUID,
    ADDT_SUID,
    SUBT_SUID,
    MULT_SUID,
    DIVT_SUID,
    CVTTS_SUID,
    CVTTQ_SVID,
    CVTQS_SUD,
    CVTQT_SUD
};

// ============================================================================
//  fpOperate16 - Opcode 16 Function-Code Mapping
//
//  This is the 11-bit function field mapping for opcode 16. It is derived
//  from your header/Excel source (with obvious syntax fixes) and follows
//  the Alpha FP encoding tables for T and S operations and conversions.
//
//  Reference: Alpha AXP Architecture Reference Manual, FP instruction
//             encoding tables for opcode 16 (CVTTS, CVTTQ, CMPTxx, etc.).
// ============================================================================

static inline quint16 fpOperate16(FPOperationKind16 k) noexcept
{
    switch (k)
    {
    case FPOperationKind16::SUBS_C:        return 0x001;
    case FPOperationKind16::MULS_C:        return 0x002;
    case FPOperationKind16::DIVS_C:        return 0x003;
    case FPOperationKind16::SUBT_C:        return 0x021;
    case FPOperationKind16::MULT_C:        return 0x022;
    case FPOperationKind16::DIVT_C:        return 0x023;

    case FPOperationKind16::ADDS_M:        return 0x040;
    case FPOperationKind16::SUBS_M:        return 0x041;
    case FPOperationKind16::MULS_M:        return 0x042;
    case FPOperationKind16::DIVS_M:        return 0x043;
    case FPOperationKind16::ADDT_M:        return 0x060;
    case FPOperationKind16::SUBT_M:        return 0x061;
    case FPOperationKind16::MULT_M:        return 0x062;
    case FPOperationKind16::DIVT_M:        return 0x063;

    case FPOperationKind16::ADDS:          return 0x080;
    case FPOperationKind16::SUBS:          return 0x081;
    case FPOperationKind16::MULS:          return 0x082;
    case FPOperationKind16::DIVS:          return 0x083;

    case FPOperationKind16::ADDS_UC:       return 0x100;
    case FPOperationKind16::SUBS_UC:       return 0x101;
    case FPOperationKind16::MULS_UC:       return 0x102;
    case FPOperationKind16::DIVS_UC:       return 0x103;
    case FPOperationKind16::ADDT_UC:       return 0x120;
    case FPOperationKind16::SUBT_UC:       return 0x121;
    case FPOperationKind16::MULT_UC:       return 0x122;
    case FPOperationKind16::DIVT_UC:       return 0x123;

    case FPOperationKind16::ADDS_UM:       return 0x140;
    case FPOperationKind16::SUBS_UM:       return 0x141;
    case FPOperationKind16::MULS_UM:       return 0x142;
    case FPOperationKind16::DIVS_UM:       return 0x143;
    case FPOperationKind16::ADDT_UM:       return 0x160;
    case FPOperationKind16::SUBT_UM:       return 0x161;
    case FPOperationKind16::MULT_UM:       return 0x162;
    case FPOperationKind16::DIVT_UM:       return 0x163;

    case FPOperationKind16::ADDS_U:        return 0x180;
    case FPOperationKind16::SUBS_U:        return 0x181;
    case FPOperationKind16::MULS_U:        return 0x182;
    case FPOperationKind16::DIVS_U:        return 0x183;

    case FPOperationKind16::ADDS_SUC:      return 0x500;
    case FPOperationKind16::SUBS_SUC:      return 0x501;
    case FPOperationKind16::MULS_SUC:      return 0x502;
    case FPOperationKind16::DIVS_SUC:      return 0x503;
    case FPOperationKind16::ADDT_SUC:      return 0x520;
    case FPOperationKind16::SUBT_SUC:      return 0x521;
    case FPOperationKind16::MULT_SUC:      return 0x522;
    case FPOperationKind16::DIVT_SUC:      return 0x523;

    case FPOperationKind16::ADDS_SUM:      return 0x540;
    case FPOperationKind16::SUBS_SUM:      return 0x541;
    case FPOperationKind16::MULS_SUM:      return 0x542;
    case FPOperationKind16::DIVS_SUM:      return 0x543;
    case FPOperationKind16::ADDT_SUM:      return 0x560;
    case FPOperationKind16::SUBT_SUM:      return 0x561;
    case FPOperationKind16::MULT_SUM:      return 0x562;
    case FPOperationKind16::DIVT_SUM:      return 0x563;

    case FPOperationKind16::ADDS_SU:       return 0x580;
    case FPOperationKind16::SUBS_SU:       return 0x581;
    case FPOperationKind16::MULS_SU:       return 0x582;
    case FPOperationKind16::DIVS_SU:       return 0x583;

    case FPOperationKind16::ADDS_SUIC:     return 0x700;
    case FPOperationKind16::SUBS_SUIC:     return 0x701;
    case FPOperationKind16::MULS_SUIC:     return 0x702;
    case FPOperationKind16::DIVS_SUIC:     return 0x703;
    case FPOperationKind16::ADDT_SUIC:     return 0x720;
    case FPOperationKind16::SUBT_SUIC:     return 0x721;
    case FPOperationKind16::MULT_SUIC:     return 0x722;
    case FPOperationKind16::DIVT_SUIC:     return 0x723;

    case FPOperationKind16::ADDS_SUIM:     return 0x740;
    case FPOperationKind16::SUBS_SUIM:     return 0x741;
    case FPOperationKind16::MULS_SUIM:     return 0x742;
    case FPOperationKind16::DIVS_SUIM:     return 0x743;
    case FPOperationKind16::ADDT_SUIM:     return 0x760;
    case FPOperationKind16::SUBT_SUIM:     return 0x761;
    case FPOperationKind16::MULT_SUIM:     return 0x762;
    case FPOperationKind16::DIVT_SUIM:     return 0x763;

    case FPOperationKind16::ADDS_SUI:      return 0x780;
    case FPOperationKind16::SUBS_SUI:      return 0x781;
    case FPOperationKind16::MULS_SUI:      return 0x782;
    case FPOperationKind16::DIVS_SUI:      return 0x783;

    case FPOperationKind16::ADDS_C:        return 0x000;
    case FPOperationKind16::ADDT_C:        return 0x020;
    case FPOperationKind16::CVTTS_C:       return 0x02C;
    case FPOperationKind16::CVTTQ_C:       return 0x02F;
    case FPOperationKind16::CVTQS_C:       return 0x03C;
    case FPOperationKind16::CVTQT_C:       return 0x03E;

    case FPOperationKind16::CVTTS_M:       return 0x06C;
    case FPOperationKind16::CVTTQ_M:       return 0x06F;
    case FPOperationKind16::CVTQS_M:       return 0x07C;
    case FPOperationKind16::CVTQT_M:       return 0x07E;

    case FPOperationKind16::ADDT:          return 0x0A0;
    case FPOperationKind16::SUBT:          return 0x0A1;
    case FPOperationKind16::MULT:          return 0x0A2;
    case FPOperationKind16::DIVT:          return 0x0A3;
    case FPOperationKind16::CMPTUN:        return 0x0A4;
    case FPOperationKind16::CMPTEQ:        return 0x0A5;
    case FPOperationKind16::CMPTLT:        return 0x0A6;
    case FPOperationKind16::CMPTLE:        return 0x0A7;
    case FPOperationKind16::CVTTS:         return 0x0AC;
    case FPOperationKind16::CVTTQ:         return 0x0AF;
    case FPOperationKind16::CVTQS:         return 0x0BC;
    case FPOperationKind16::CVTQT:         return 0x0BE;

    case FPOperationKind16::ADDS_D:        return 0x0C0;
    case FPOperationKind16::SUBS_ID:       return 0x0C1;
    case FPOperationKind16::MULS_ID:       return 0x0C2;
    case FPOperationKind16::DIVS_ID:       return 0x0C3;
    case FPOperationKind16::ADDT_D:        return 0x0E0;
    case FPOperationKind16::SUBT_D:        return 0x0E1;
    case FPOperationKind16::SUBT_ID:       return 0x0E1;
    case FPOperationKind16::MULT_D:        return 0x0E2;
    case FPOperationKind16::MULT_ID:       return 0x0E2;
    case FPOperationKind16::DIVT_D:        return 0x0E3;
    case FPOperationKind16::DIVT_ID:       return 0x0E3;
    case FPOperationKind16::CVTTS_ID:      return 0x0EC;
    case FPOperationKind16::CVTTQ_D:       return 0x0EF;
    case FPOperationKind16::CVTQS_D:       return 0x0FC;
    case FPOperationKind16::CVTQT_ID:      return 0x0FE;

    case FPOperationKind16::CVTTS_UC:      return 0x12C;
    case FPOperationKind16::CVTTQ_VC:      return 0x12F;
    case FPOperationKind16::CVTTS_UM:      return 0x16C;
    case FPOperationKind16::CVTTQ_VM:      return 0x16F;

    case FPOperationKind16::ADDT_U:        return 0x1A0;
    case FPOperationKind16::SUBT_U:        return 0x1A1;
    case FPOperationKind16::MULT_U:        return 0x1A2;
    case FPOperationKind16::DIVT_U:        return 0x1A3;
    case FPOperationKind16::CVTTS_U:       return 0x1AC;
    case FPOperationKind16::CVTTQ_V:       return 0x1AF;
    case FPOperationKind16::CVTBQ_V:       return 0x1AF;

    case FPOperationKind16::ADDS_UD:       return 0x1C0;
    case FPOperationKind16::SUBS_UD:       return 0x1C1;
    case FPOperationKind16::MULS_UD:       return 0x1C2;
    case FPOperationKind16::DIVS_UD:       return 0x1C3;
    case FPOperationKind16::ADDT_UD:       return 0x1E0;
    case FPOperationKind16::SUBT_UD:       return 0x1E1;
    case FPOperationKind16::MULT_UD:       return 0x1E2;
    case FPOperationKind16::DIVT_UD:       return 0x1E3;
    case FPOperationKind16::CVTTS_UD:      return 0x1EC;
    case FPOperationKind16::CVTTQ_VD:      return 0x1EF;
    case FPOperationKind16::CVTST:         return 0x2AC;

    case FPOperationKind16::CVTTS_SUC:     return 0x52C;
    case FPOperationKind16::CVTTQ_SVC:     return 0x52F;
    case FPOperationKind16::CVTTS_SUM:     return 0x56C;
    case FPOperationKind16::CVTTQ_SVM:     return 0x56F;

    case FPOperationKind16::ADDT_SU:       return 0x5A0;
    case FPOperationKind16::SUBT_SU:       return 0x5A1;
    case FPOperationKind16::MULT_SU:       return 0x5A2;
    case FPOperationKind16::DIVT_SU:       return 0x5A3;
    case FPOperationKind16::CMPTUN_SU:     return 0x5A4;
    case FPOperationKind16::CMPTEQ_SU:     return 0x5A5;
    case FPOperationKind16::CMPTLT_SU:     return 0x5A6;
    case FPOperationKind16::CMPTLE_SU:     return 0x5A7;
    case FPOperationKind16::CVTTS_SU:      return 0x5AC;
    case FPOperationKind16::CVTTQ_SV:      return 0x5AF;

    case FPOperationKind16::ADDS_SUD:      return 0x5C0;
    case FPOperationKind16::SUBS_SUD:      return 0x5C1;
    case FPOperationKind16::MULS_SUD:      return 0x5C2;
    case FPOperationKind16::DIVS_SUD:      return 0x5C3;
    case FPOperationKind16::ADDT_SUD:      return 0x5E0;
    case FPOperationKind16::SUBT_SUD:      return 0x5E1;
    case FPOperationKind16::MULT_SUD:      return 0x5E2;
    case FPOperationKind16::DIVT_SUD:      return 0x5E3;
    case FPOperationKind16::CVTTS_SUD:     return 0x5EC;
    case FPOperationKind16::CVTTQ_SVD:     return 0x5EF;

    case FPOperationKind16::CVTST_S:       return 0x6AC;
    case FPOperationKind16::CVTTS_SUIC:    return 0x72C;
    case FPOperationKind16::CVTTQ_SVIC:    return 0x72F;
    case FPOperationKind16::CVTQS_SUC:     return 0x73C;
    case FPOperationKind16::CVTQT_SUC:     return 0x73E;
    case FPOperationKind16::CVTTS_SUIM:    return 0x76C;
    case FPOperationKind16::CVTTQ_SVIM:    return 0x76F;
    case FPOperationKind16::CVTQS_SUM:     return 0x77C;
    case FPOperationKind16::CVTQT_SUM:     return 0x77E;

    case FPOperationKind16::ADDT_SUI:      return 0x7A0;
    case FPOperationKind16::SUBT_SUI:      return 0x7A1;
    case FPOperationKind16::MULT_SUI:      return 0x7A2;
    case FPOperationKind16::DIVT_SUI:      return 0x7A3;
    case FPOperationKind16::CVTTS_SUI:     return 0x7AC;
    case FPOperationKind16::CVTTQ_SVI:     return 0x7AF;
    case FPOperationKind16::CVTQS_SU:      return 0x7BC;
    case FPOperationKind16::CVTQT_SUI:     return 0x7BE;

    case FPOperationKind16::ADDS_SUID:     return 0x7C0;
    case FPOperationKind16::SUBS_SUID:     return 0x7C1;
    case FPOperationKind16::MULS_SUID:     return 0x7C2;
    case FPOperationKind16::DIVS_SUID:     return 0x7C3;
    case FPOperationKind16::ADDT_SUID:     return 0x7E0;
    case FPOperationKind16::SUBT_SUID:     return 0x7E1;
    case FPOperationKind16::MULT_SUID:     return 0x7E2;
    case FPOperationKind16::DIVT_SUID:     return 0x7E3;
    case FPOperationKind16::CVTTS_SUID:    return 0x7EC;
    case FPOperationKind16::CVTTQ_SVID:    return 0x7EF;
    case FPOperationKind16::CVTQS_SUD:     return 0x7FC;
    case FPOperationKind16::CVTQT_SUD:     return 0x7FE;

    default:
        // TODO: log unexpected FPOperationKind16 in fpOperate16()
        return 0x000;
    }
}

// ============================================================================
//  FP16BaseOpKind - high-level classification
// ============================================================================

enum class FP16BaseOpKind : quint8
{
    None,
    Add,
    Sub,
    Mul,
    Div,
    Cvt,
    Cmp
};

// Classify each FPOperationKind16 into a base operation.
static inline FP16BaseOpKind classifyBaseOp16(FPOperationKind16 k) noexcept
{
    switch (k)
    {
        // Add family
    case FPOperationKind16::ADDS_C:
    case FPOperationKind16::ADDS_M:
    case FPOperationKind16::ADDS:
    case FPOperationKind16::ADDS_UC:
    case FPOperationKind16::ADDS_UM:
    case FPOperationKind16::ADDS_U:
    case FPOperationKind16::ADDS_SUC:
    case FPOperationKind16::ADDS_SUM:
    case FPOperationKind16::ADDS_SU:
    case FPOperationKind16::ADDS_SUIC:
    case FPOperationKind16::ADDS_SUIM:
    case FPOperationKind16::ADDS_SUI:
    case FPOperationKind16::ADDS_D:
    case FPOperationKind16::ADDS_UD:
    case FPOperationKind16::ADDS_SUD:
    case FPOperationKind16::ADDS_SUID:
    case FPOperationKind16::ADDT_C:
    case FPOperationKind16::ADDT_M:
    case FPOperationKind16::ADDT:
    case FPOperationKind16::ADDT_UC:
    case FPOperationKind16::ADDT_UM:
    case FPOperationKind16::ADDT_U:
    case FPOperationKind16::ADDT_SUC:
    case FPOperationKind16::ADDT_SUM:
    case FPOperationKind16::ADDT_SU:
    case FPOperationKind16::ADDT_SUIC:
    case FPOperationKind16::ADDT_SUIM:
    case FPOperationKind16::ADDT_SUI:
    case FPOperationKind16::ADDT_D:
    case FPOperationKind16::ADDT_UD:
    case FPOperationKind16::ADDT_SUD:
    case FPOperationKind16::ADDT_SUID:
        return FP16BaseOpKind::Add;

        // Subtract family
    case FPOperationKind16::SUBS_C:
    case FPOperationKind16::SUBS_M:
    case FPOperationKind16::SUBS:
    case FPOperationKind16::SUBS_UC:
    case FPOperationKind16::SUBS_UM:
    case FPOperationKind16::SUBS_U:
    case FPOperationKind16::SUBS_SUC:
    case FPOperationKind16::SUBS_SUM:
    case FPOperationKind16::SUBS_SU:
    case FPOperationKind16::SUBS_SUIC:
    case FPOperationKind16::SUBS_SUIM:
    case FPOperationKind16::SUBS_SUI:
    case FPOperationKind16::SUBS_ID:
    case FPOperationKind16::SUBS_UD:
    case FPOperationKind16::SUBS_SUD:
    case FPOperationKind16::SUBS_SUID:
    case FPOperationKind16::SUBT_C:
    case FPOperationKind16::SUBT_M:
    case FPOperationKind16::SUBT:
    case FPOperationKind16::SUBT_UC:
    case FPOperationKind16::SUBT_UM:
    case FPOperationKind16::SUBT_U:
    case FPOperationKind16::SUBT_SUC:
    case FPOperationKind16::SUBT_SUM:
    case FPOperationKind16::SUBT_SU:
    case FPOperationKind16::SUBT_SUIC:
    case FPOperationKind16::SUBT_SUIM:
    case FPOperationKind16::SUBT_SUI:
    case FPOperationKind16::SUBT_D:
    case FPOperationKind16::SUBT_ID:
    case FPOperationKind16::SUBT_UD:
    case FPOperationKind16::SUBT_SUD:
    case FPOperationKind16::SUBT_SUID:
        return FP16BaseOpKind::Sub;

        // Multiply family
    case FPOperationKind16::MULS_C:
    case FPOperationKind16::MULS_M:
    case FPOperationKind16::MULS:
    case FPOperationKind16::MULS_UC:
    case FPOperationKind16::MULS_UM:
    case FPOperationKind16::MULS_U:
    case FPOperationKind16::MULS_SUC:
    case FPOperationKind16::MULS_SUM:
    case FPOperationKind16::MULS_SU:
    case FPOperationKind16::MULS_SUIC:
    case FPOperationKind16::MULS_SUIM:
    case FPOperationKind16::MULS_SUI:
    case FPOperationKind16::MULS_ID:
    case FPOperationKind16::MULS_UD:
    case FPOperationKind16::MULS_SUD:
    case FPOperationKind16::MULS_SUID:
    case FPOperationKind16::MULT_C:
    case FPOperationKind16::MULT_M:
    case FPOperationKind16::MULT:
    case FPOperationKind16::MULT_UC:
    case FPOperationKind16::MULT_UM:
    case FPOperationKind16::MULT_U:
    case FPOperationKind16::MULT_SUC:
    case FPOperationKind16::MULT_SUM:
    case FPOperationKind16::MULT_SU:
    case FPOperationKind16::MULT_SUIC:
    case FPOperationKind16::MULT_SUIM:
    case FPOperationKind16::MULT_SUI:
    case FPOperationKind16::MULT_D:
    case FPOperationKind16::MULT_ID:
    case FPOperationKind16::MULT_UD:
    case FPOperationKind16::MULT_SUD:
    case FPOperationKind16::MULT_SUID:
        return FP16BaseOpKind::Mul;

        // Divide family
    case FPOperationKind16::DIVS_C:
    case FPOperationKind16::DIVS_M:
    case FPOperationKind16::DIVS:
    case FPOperationKind16::DIVS_UC:
    case FPOperationKind16::DIVS_UM:
    case FPOperationKind16::DIVS_U:
    case FPOperationKind16::DIVS_SUC:
    case FPOperationKind16::DIVS_SUM:
    case FPOperationKind16::DIVS_SU:
    case FPOperationKind16::DIVS_SUIC:
    case FPOperationKind16::DIVS_SUIM:
    case FPOperationKind16::DIVS_SUI:
    case FPOperationKind16::DIVS_ID:
    case FPOperationKind16::DIVS_UD:
    case FPOperationKind16::DIVS_SUD:
    case FPOperationKind16::DIVS_SUID:
    case FPOperationKind16::DIVT_C:
    case FPOperationKind16::DIVT_M:
    case FPOperationKind16::DIVT:
    case FPOperationKind16::DIVT_UC:
    case FPOperationKind16::DIVT_UM:
    case FPOperationKind16::DIVT_U:
    case FPOperationKind16::DIVT_SUC:
    case FPOperationKind16::DIVT_SUM:
    case FPOperationKind16::DIVT_SU:
    case FPOperationKind16::DIVT_SUIC:
    case FPOperationKind16::DIVT_SUIM:
    case FPOperationKind16::DIVT_SUI:
    case FPOperationKind16::DIVT_D:
    case FPOperationKind16::DIVT_ID:
    case FPOperationKind16::DIVT_UD:
    case FPOperationKind16::DIVT_SUD:
    case FPOperationKind16::DIVT_SUID:
        return FP16BaseOpKind::Div;

        // Conversion family
    case FPOperationKind16::CVTTS_C:
    case FPOperationKind16::CVTTQ_C:
    case FPOperationKind16::CVTQS_C:
    case FPOperationKind16::CVTQT_C:
    case FPOperationKind16::CVTTS_M:
    case FPOperationKind16::CVTTQ_M:
    case FPOperationKind16::CVTQS_M:
    case FPOperationKind16::CVTQT_M:
    case FPOperationKind16::CVTTS:
    case FPOperationKind16::CVTTQ:
    case FPOperationKind16::CVTQS:
    case FPOperationKind16::CVTQT:
    case FPOperationKind16::CVTTS_ID:
    case FPOperationKind16::CVTTQ_D:
    case FPOperationKind16::CVTQS_D:
    case FPOperationKind16::CVTQT_ID:
    case FPOperationKind16::CVTTS_UC:
    case FPOperationKind16::CVTTQ_VC:
    case FPOperationKind16::CVTTS_UM:
    case FPOperationKind16::CVTTQ_VM:
    case FPOperationKind16::CVTTS_U:
    case FPOperationKind16::CVTTQ_V:
    case FPOperationKind16::CVTBQ_V:
    case FPOperationKind16::CVTTS_UD:
    case FPOperationKind16::CVTTQ_VD:
    case FPOperationKind16::CVTST:
    case FPOperationKind16::CVTTS_SUC:
    case FPOperationKind16::CVTTQ_SVC:
    case FPOperationKind16::CVTTS_SUM:
    case FPOperationKind16::CVTTQ_SVM:
    case FPOperationKind16::CVTTS_SU:
    case FPOperationKind16::CVTTQ_SV:
    case FPOperationKind16::CVTTS_SUD:
    case FPOperationKind16::CVTTQ_SVD:
    case FPOperationKind16::CVTST_S:
    case FPOperationKind16::CVTTS_SUIC:
    case FPOperationKind16::CVTTQ_SVIC:
    case FPOperationKind16::CVTQS_SUC:
    case FPOperationKind16::CVTQT_SUC:
    case FPOperationKind16::CVTTS_SUIM:
    case FPOperationKind16::CVTTQ_SVIM:
    case FPOperationKind16::CVTQS_SUM:
    case FPOperationKind16::CVTQT_SUM:
    case FPOperationKind16::CVTTS_SUI:
    case FPOperationKind16::CVTTQ_SVI:
    case FPOperationKind16::CVTQS_SU:
    case FPOperationKind16::CVTQT_SUI:
    case FPOperationKind16::CVTTS_SUID:
    case FPOperationKind16::CVTTQ_SVID:
    case FPOperationKind16::CVTQS_SUD:
    case FPOperationKind16::CVTQT_SUD:
        return FP16BaseOpKind::Cvt;

        // Compare family
    case FPOperationKind16::CMPTUN:
    case FPOperationKind16::CMPTEQ:
    case FPOperationKind16::CMPTLT:
    case FPOperationKind16::CMPTLE:
    case FPOperationKind16::CMPTUN_SU:
    case FPOperationKind16::CMPTEQ_SU:
    case FPOperationKind16::CMPTLT_SU:
    case FPOperationKind16::CMPTLE_SU:
        return FP16BaseOpKind::Cmp;

    default:
        // TODO: log unexpected FPOperationKind16 in classifyBaseOp16()
        return FP16BaseOpKind::None;
    }
}

// Opcode 17

enum class FPOperationKind17 : quint8
{
    CVTLQ, CPYS, CPYSN, CPYSE,
    MT_FPCR, MF_FPCR, CVTQL,
    FCMOVEQ, FCMOVNE, FCMOVLT, FCMOVGE, FCMOVLE, FCMOVGT,
    CVTBQ_VC
};

static inline quint16 fpOperate17(FPOperationKind17 k) noexcept
{
    switch (k)
    {
    case FPOperationKind17::CVTLQ:     return 0x010;
    case FPOperationKind17::CPYS:      return 0x020;
    case FPOperationKind17::CPYSN:     return 0x021;
    case FPOperationKind17::CPYSE:     return 0x022;
    case FPOperationKind17::MT_FPCR:   return 0x024;
    case FPOperationKind17::MF_FPCR:   return 0x025;
    case FPOperationKind17::CVTQL:     return 0x030;
    case FPOperationKind17::FCMOVEQ:   return 0x02A;
    case FPOperationKind17::FCMOVNE:   return 0x02B;
    case FPOperationKind17::FCMOVLT:   return 0x02C;
    case FPOperationKind17::FCMOVGE:   return 0x02D;
    case FPOperationKind17::FCMOVLE:   return 0x02E;
    case FPOperationKind17::FCMOVGT:   return 0x02F;
    default:                           return 0x000;
    }
}

enum class FPOperationKind18 : quint8
{
    TRAPB, EXCB, MB, WMB, FETCH, CVTBQ_S,
    FETCH_M, RPCC, RC, ECB, RS
};

static inline quint16 fpOperate18(FPOperationKind18 k) noexcept
{
    switch (k)
    {
    case FPOperationKind18::TRAPB:    return 0x0000;
    case FPOperationKind18::EXCB:     return 0x0400;
    case FPOperationKind18::MB:       return 0x4000;
    case FPOperationKind18::WMB:      return 0x4400;
    case FPOperationKind18::FETCH:    return 0x8000;
    case FPOperationKind18::CVTBQ_S:  return 0x04A4;
    case FPOperationKind18::FETCH_M:  return 0xA000;
    case FPOperationKind18::RPCC:     return 0xC000;
    case FPOperationKind18::RC:       return 0xE000;
    case FPOperationKind18::ECB:      return 0xE800;
    case FPOperationKind18::RS:       return 0xF000;
    default:                          return 0x0000;
    }
}

// Mem Load / Store

enum class FpLoadStoreKind : quint8
{
	Ldf, Ldg, Lds, Ldt, Stf, Stg, Sts, Stt
};

static inline quint8 fpLoadStoreOpcode(FpLoadStoreKind kind) noexcept
{
	switch (kind)
	{
	case FpLoadStoreKind::Ldf:  return 0x20;
	case FpLoadStoreKind::Ldg:  return 0x21;
	case FpLoadStoreKind::Lds:  return 0x22;
	case FpLoadStoreKind::Ldt:  return 0x23;
	case FpLoadStoreKind::Stf:  return 0x24;
	case FpLoadStoreKind::Stg:  return 0x25;
	case FpLoadStoreKind::Sts:  return 0x26;
	case FpLoadStoreKind::Stt:  return 0x27;
	default: return 0x22;
	}
}

// ============================================================================
// JumpOperationKind1A
// ============================================================================
enum class JumpOperationKind1A : quint8
{
    JMP,            // function 0
    JSR,            // function 1
    RET,            // function 2
    JSR_COROUTINE   // function 3
};


// ============================================================================
// function code mapping
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE quint16 jumpOperate1A_functionCode(JumpOperationKind1A k) noexcept
{
    switch (k)
    {
    case JumpOperationKind1A::JMP:            return 0;
    case JumpOperationKind1A::JSR:            return 1;
    case JumpOperationKind1A::RET:            return 2;
    case JumpOperationKind1A::JSR_COROUTINE:  return 3;
    }
    return 0;
}



// ============================================================================
// Enum: IntByteMiscKind1C
// ============================================================================
enum class IntByteMiscKind1C : quint8
{
	SEXTW,      // 0x01
	CTPOP,      // 0x30
	PERR,       // 0x31
	CTLZ,       // 0x32
	CTTZ,       // 0x33
	UNPKBW,     // 0x34
	UNPKBL,     // 0x35
	PKWB,       // 0x36
	PKLB,       // 0x37
	MINSB8,     // 0x38
	MINSW4,     // 0x39
	MINUB8,     // 0x3A
	MINUW4,     // 0x3B
	MAXUB8,     // 0x3C
	MAXUW4,     // 0x3D
	MAXSB8,     // 0x3E
	MAXSW4,     // 0x3F
	FTOIT,      // 0x70
	FTOIS       // 0x78
};

// ============================================================================
// Function code mapping for opcode 0x1C
// ============================================================================
AXP_HOT AXP_ALWAYS_INLINE quint16 byteMisc1C_functionCode(IntByteMiscKind1C k) noexcept
{
	switch (k)
	{
	case IntByteMiscKind1C::SEXTW:    return 0x01;
	case IntByteMiscKind1C::CTPOP:    return 0x30;
	case IntByteMiscKind1C::PERR:     return 0x31;
	case IntByteMiscKind1C::CTLZ:     return 0x32;
	case IntByteMiscKind1C::CTTZ:     return 0x33;
	case IntByteMiscKind1C::UNPKBW:   return 0x34;
	case IntByteMiscKind1C::UNPKBL:   return 0x35;
	case IntByteMiscKind1C::PKWB:     return 0x36;
	case IntByteMiscKind1C::PKLB:     return 0x37;
	case IntByteMiscKind1C::MINSB8:   return 0x38;
	case IntByteMiscKind1C::MINSW4:   return 0x39;
	case IntByteMiscKind1C::MINUB8:   return 0x3A;
	case IntByteMiscKind1C::MINUW4:   return 0x3B;
	case IntByteMiscKind1C::MAXUB8:   return 0x3C;
	case IntByteMiscKind1C::MAXUW4:   return 0x3D;
	case IntByteMiscKind1C::MAXSB8:   return 0x3E;
	case IntByteMiscKind1C::MAXSW4:   return 0x3F;
	case IntByteMiscKind1C::FTOIT:    return 0x70;
	case IntByteMiscKind1C::FTOIS:    return 0x78;
	}
	return 0;
}

enum class BranchKind30_3F : quint8
{
	BR,
	FBEQ, FBLT, FBLE,
	BSR,
	FBNE, FBGE, FBGT,
	BLBC, BEQ, BLT, BLE,
	BLBS, BNE, BGE, BGT
};

AXP_HOT AXP_ALWAYS_INLINE  quint8 branch_opcode(BranchKind30_3F k) noexcept
{
	switch (k)
	{
	case BranchKind30_3F::BR:   return 0x30;
	case BranchKind30_3F::FBEQ: return 0x31;
	case BranchKind30_3F::FBLT: return 0x32;
	case BranchKind30_3F::FBLE: return 0x33;
	case BranchKind30_3F::BSR:  return 0x34;
	case BranchKind30_3F::FBNE: return 0x35;
	case BranchKind30_3F::FBGE: return 0x36;
	case BranchKind30_3F::FBGT: return 0x37;
	case BranchKind30_3F::BLBC: return 0x38;
	case BranchKind30_3F::BEQ:  return 0x39;
	case BranchKind30_3F::BLT:  return 0x3A;
	case BranchKind30_3F::BLE:  return 0x3B;
	case BranchKind30_3F::BLBS: return 0x3C;
	case BranchKind30_3F::BNE:  return 0x3D;
	case BranchKind30_3F::BGE:  return 0x3E;
	case BranchKind30_3F::BGT:  return 0x3F;
	}
	return 0x30;
}
#pragma region Branch Predictor




struct BranchResolutionResult
{
	bool   taken;              // actual outcome
	bool   mispredict;         // true if flush is needed
	quint64 actualTarget;      // where PC must go on mispredict
};

struct BranchEval
{
	quint64 nextPC;
	quint64 targetPC;
};



#pragma endregion Branch Predictor

enum class BranchCode : quint8 {
    None = 0,              // Not a branch

    // Unconditional branches
    BR,                    // Branch (0x30)
    BSR,                   // Branch to subroutine (0x34)

    // Conditional integer branches (test Ra)
    BEQ,                   // Branch if equal (0x39)
    BNE,                   // Branch if not equal (0x3D)
    BLT,                   // Branch if less than (0x3A)
    BLE,                   // Branch if less or equal (0x3B)
    BGT,                   // Branch if greater than (0x3F)
    BGE,                   // Branch if greater or equal (0x3E)
    BLBC,                  // Branch if low bit clear (0x38)
    BLBS,                  // Branch if low bit set (0x3C)

    // Conditional FP branches (test Fa)
    FBEQ,                  // FP branch if equal (0x31)
    FBNE,                  // FP branch if not equal (0x35)
    FBLT,                  // FP branch if less than (0x32)
    FBLE,                  // FP branch if less or equal (0x33)
    FBGT,                  // FP branch if greater than (0x37)
    FBGE,                  // FP branch if greater or equal (0x36)

    // Jump format
    JMP,                   // Jump (0x1A, hint=0)
    JSR,                   // Jump to subroutine (0x1A, hint=1)
    RET,                   // Return (0x1A, hint=2)
    JSR_COROUTINE          // JSR coroutine (0x1A, hint=3)
};

AXP_HOT AXP_ALWAYS_INLINE BranchCode getBranchCode(quint8 opcode, quint16 hint = 0) noexcept
{
    switch (opcode) {
    case 0x30: return BranchCode::BR;
    case 0x31: return BranchCode::FBEQ;
    case 0x32: return BranchCode::FBLT;
    case 0x33: return BranchCode::FBLE;
    case 0x34: return BranchCode::BSR;
    case 0x35: return BranchCode::FBNE;
    case 0x36: return BranchCode::FBGE;
    case 0x37: return BranchCode::FBGT;
    case 0x38: return BranchCode::BLBC;
    case 0x39: return BranchCode::BEQ;
    case 0x3A: return BranchCode::BLT;
    case 0x3B: return BranchCode::BLE;
    case 0x3C: return BranchCode::BLBS;
    case 0x3D: return BranchCode::BNE;
    case 0x3E: return BranchCode::BGE;
    case 0x3F: return BranchCode::BGT;

    case 0x1A: // Jump format - check hint
        switch (hint & 0x3) {
        case 0: return BranchCode::JMP;
        case 1: return BranchCode::JSR;
        case 2: return BranchCode::RET;
        case 3: return BranchCode::JSR_COROUTINE;
        }
        break;

    default:
        return BranchCode::None;
    }

    return BranchCode::None;
}

#endif // GRAIN_CORE_H
