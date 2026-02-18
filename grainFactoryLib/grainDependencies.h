#ifndef GRAINDEPENDENCIES_H
#define GRAINDEPENDENCIES_H
#include <QtCore>

// enum class enumGrainPlatform {
// 	Alpha,
// 	Tru64,
// 	Linux,
// 	Tru64AndLinux
// };
enum class InstructionType : quint16 {
  ADDF,
  ADDG,
  ADDL,
  ADDL_V,
  ADDQ,
  ADDQ_V,
  ADDS,
  ADDS_C,
  ADDS_D,
  ADDS_M,
  ADDS_SU,
  ADDS_SUC,
  ADDS_SUD,
  ADDS_SUI,
  ADDS_SUIC,
  ADDS_SUID,
  ADDS_SUIM,
  ADDS_SUM,
  ADDS_U,
  ADDS_UC,
  ADDS_UD,
  ADDS_UM,
  ADDT,
  ADDT_C,
  ADDT_D,
  ADDT_M,
  ADDT_SU,
  ADDT_SUC,
  ADDT_SUD,
  ADDT_SUI,
  ADDT_SUIC,
  ADDT_SUID,
  ADDT_SUIM,
  ADDT_SUM,
  ADDT_U,
  ADDT_UC,
  ADDT_UD,
  ADDT_UM,
  AMASK,
  AMOVRM,
  AMOVRR,
  AND,
  BEQ,
  BGE,
  BGT,
  BIC,
  BIS,
  BLBC,
  BLBS,
  BLE,
  BLT,
  BNE,
  BPT,
  BR,
  BSR,
  BUGCHK,
  CALLKD,
  CALL_PAL,
  CFLUSH,
  CHME,
  CHMK,
  CHMS,
  CHMU,
  CLRFEN,
  CMOVEQ,
  CMOVGE,
  CMOVGT,
  CMOVLBC,
  CMOVLBS,
  CMOVLE,
  CMOVLT,
  CMOVNE,
  CMPBGE,
  CMPEQ,
  CMPGEQ,
  CMPGLE,
  CMPGLT,
  CMPLE,
  CMPLT,
  CMPTEQ,
  CMPTEQ_SU,
  CMPTLE,
  CMPTLE_SU,
  CMPTLT,
  CMPTLT_SU,
  CMPTUN,
  CMPTUN_SU,
  CMPULE,
  CMPULT,
  CPYS,
  CPYSE,
  CPYSN,
  CSERVE,
  CTPOP,
  CTTZ,
  CVTDG,
  CVTGD,
  CVTGF,
  CVTGQ,
  CVTLQ,
  CVTQF,
  CVTQG,
  CVTQL,
  CVTQS,
  CVTQS_C,
  CVTQS_D,
  CVTQS_M,
  CVTQS_SU,
  CVTQS_SUC,
  CVTQS_SUD,
  CVTQS_SUI,
  CVTQS_SUIC,
  CVTQS_SUID,
  CVTQS_SUIM,
  CVTQS_SUM,
  CVTQT,
  CVTQT_C,
  CVTQT_D,
  CVTQT_ID,
  CVTQT_M,
  CVTQT_SUC,
  CVTQT_SUD,
  CVTQT_SUI,
  CVTQT_SUIC,
  CVTQT_SUID,
  CVTQT_SUIM,
  CVTQT_SUM,
  CVTST,
  CVTST_S,
  CVTTQ,
  CVTTQ_C,
  CVTTQ_D,
  CVTTQ_M,
  CVTTQ_SV,
  CVTTQ_SVC,
  CVTTQ_SVD,
  CVTTQ_SVI,
  CVTTQ_SVIC,
  CVTTQ_SVID,
  CVTTQ_SVIM,
  CVTTQ_SVM,
  CVTTQ_V,
  CVTTQ_VC,
  CVTTQ_VD,
  CVTTQ_VM,
  CVTTS,
  CVTTST,
  CVTTS_C,
  CVTTS_D,
  CVTTS_M,
  CVTTS_SU,
  CVTTS_SUC,
  CVTTS_SUD,
  CVTTS_SUI,
  CVTTS_SUIC,
  CVTTS_SUID,
  CVTTS_SUIM,
  CVTTS_SUM,
  CVTTS_U,
  CVTTS_UC,
  CVTTS_UD,
  CVTTS_UM,
  DIVF,
  DIVG,
  DIVS,
  DIVS_C,
  DIVS_D,
  DIVS_M,
  DIVS_SU,
  DIVS_SUC,
  DIVS_SUD,
  DIVS_SUI,
  DIVS_SUIC,
  DIVS_SUID,
  DIVS_SUIM,
  DIVS_SUM,
  DIVS_U,
  DIVS_UC,
  DIVS_UD,
  DIVS_UM,
  DIVT,
  DIVT_C,
  DIVT_D,
  DIVT_M,
  DIVT_SU,
  DIVT_SUC,
  DIVT_SUD,
  DIVT_SUI,
  DIVT_SUIC,
  DIVT_SUID,
  DIVT_SUIM,
  DIVT_SUM,
  DIVT_U,
  DIVT_UC,
  DIVT_UD,
  DIVT_UM,
  DRAINA,
  ECB,
  EQV,
  EXCB,
  EXTBL,
  EXTLH,
  EXTLL,
  EXTQH,
  EXTQL,
  EXTWH,
  EXTWL,
  FBEQ,
  FBGE,
  FBGT,
  FBLE,
  FBLT,
  FBNE,
  FCMOVEQ,
  FCMOVGE,
  FCMOVGT,
  FCMOVLE,
  FCMOVLT,
  FCMOVNE,
  FETCH,
  FETCH_M,
  FTOIS,
  FTOIT,
  GENTRAP,
  HALT,
  IFOFF,
  IFOFT,
  ILLEGAL_UNDEFINED,
  IMB,
  IMPLVER,
  INSBL,
  INSLH,
  INSLL,
  INSQH,
  INSQHIL,
  INSQHILR,
  INSQHIQ,
  INSQHIQR,
  INSQL,
  INSQTIL,
  INSQTILR,
  INSQTIQ,
  INSQTIQR,
  INSQUEL,
  INSQUEL_D,
  INSQUEQ,
  INSQUEQ_D,
  INSWH,
  INSWL,
  ITOFF,
  ITOFS,
  JMP,
  JSR,
  JSR_COROUTINE,
  KBPT,
  LDA,
  LDAH,
  LDBU,
  LDF,
  LDG,
  LDL,
  LDL_L,
  LDQ,
  LDQP,
  LDQ_L,
  LDQ_U,
  LDS,
  LDT,
  LDWU,
  MAXSB8,
  MAXSW4,
  MAXUB8,
  MAXUW4,
  MB,
  MFCR,
  MFPR_ASN,
  MFPR_ASTEN,
  MFPR_ASTSR,
  MFPR_ESP,
  MFPR_FEN,
  MFPR_IPL,
  MFPR_MCES,
  MFPR_PCBB,
  MFPR_PRBR,
  MFPR_PTBR,
  MFPR_SCBB,
  MFPR_SISR,
  MFPR_SSP,
  MFPR_SYSPTBR,
  MFPR_TBCHK,
  MFPR_USP,
  MFPR_VIRBND,
  MFPR_VPTB,
  MFPR_WHAMI,
  MF_FPCR,
  MINSB8,
  MINSW4,
  MINUB8,
  MINUW4,
  MSKBL,
  MSKLH,
  MSKLL,
  MSKQH,
  MSKQL,
  MSKWH,
  MSKWL,
  MTCR,
  MTPR_ASTEN,
  MTPR_ASTSR,
  MTPR_DATFX,
  MTPR_ESP,
  MTPR_FEN,
  MTPR_IPIR,
  MTPR_IPL,
  MTPR_MCES,
  MTPR_PERFMON,
  MTPR_PRBR,
  MTPR_SCBB,
  MTPR_SIRR,
  MTPR_SSP,
  MTPR_SYSPTBR,
  MTPR_TBIA,
  MTPR_TBIAP,
  MTPR_TBIS,
  MTPR_TBISD,
  MTPR_TBISI,
  MTPR_USP,
  MTPR_VIRBND,
  MTPR_VPTB,
  MT_FPCR,
  MULF,
  MULG,
  MULL,
  MULL_V,
  MULQ,
  MULQV,
  MULS,
  MULS_C,
  MULS_D,
  MULS_M,
  MULS_SU,
  MULS_SUC,
  MULS_SUD,
  MULS_SUI,
  MULS_SUIC,
  MULS_SUID,
  MULS_SUIM,
  MULS_SUM,
  MULS_U,
  MULS_UC,
  MULS_UD,
  MULT,
  MULT_C,
  MULT_D,
  MULT_M,
  MULT_SU,
  MULT_SUC,
  MULT_SUD,
  MULT_SUI,
  MULT_SUIC,
  MULT_SUID,
  MULT_SUIM,
  MULT_SUM,
  MULT_U,
  MULT_UC,
  MULT_UD,
  MULT_UM,
  ORNOT,
  PERR,
  PKLB,
  PKWB,
  PREFETCH,
  PREFETCH_EN,
  PREFETCH_M,
  PREFETCH_MEN,
  PROBER,
  PROBEW,
  RC,
  RDESP,
  RDPRBR,
  RDSSP,
  RDTEB,
  RDUSP,
  RDVAL,
  RD_PS,
  RDKSP,
  RDMCES,
  RDPCBB,
  READ_UNQ,
  REBOOT,
  REI,
  REMQEQ,
  REMQEQ_D,
  REMQHIL,
  REMQHILR,
  REMQHIQ,
  REMQHIQR,
  REMQTIL,
  REMQTILR,
  REMQTIQ,
  REMQTIQR,
  REMQUEL,
  REMQUEL_D,
  REMQUEQ,
  REMQUEQ_D,
  RET,
  RFE,
  RPCC,
  RS,
  RSCC,
  S4ADDL,
  S4ADDQ,
  S4SUBL,
  S4SUBQ,
  S8ADDL,
  S8ADDQ,
  S8SUBL,
  S8SUBQ,
  SEXTB,
  SEXTW,
  SLL,
  SLL_SRA,
  SQRTF,
  SQRTF_S,
  SQRTF_SC,
  SQRTF_SU,
  SQRTF_SUC,
  SQRTF_U,
  SQRTF_UC,
  SQRTG,
  SQRTG_D,
  SQRTG_S,
  SQRTG_SC,
  SQRTG_SU,
  SQRTG_SUC,
  SQRTG_U,
  SQRTG_UC,
  SQRTS,
  SQRTS_M,
  SQRTS_SU,
  SQRTS_SUC,
  SQRTS_SUD,
  SQRTS_SUI,
  SQRTS_SUIC,
  SQRTS_SUID,
  SQRTS_SUIM,
  SQRTS_SUM,
  SQRTS_U,
  SQRTS_UC,
  SQRTS_UD,
  SQRTS_UM,
  SQRTT,
  SQRTT_C,
  SQRTT_D,
  SQRTT_M,
  SQRTT_SU,
  SQRTT_SUC,
  SQRTT_SUD,
  SQRTT_SUI,
  SQRTT_SUIC,
  SQRTT_SUID,
  SQRTT_SUIM,
  SQRTT_SUM,
  SQRTT_U,
  SQRTT_UC,
  SQRTT_UD,
  SQRTT_UM,
  SRA,
  SRL,
  STB,
  STF,
  STG,
  STL,
  STL_C,
  STQ,
  STQP,
  STQ_C,
  STQ_U,
  STS,
  STT,
  STW,
  SUBF,
  SUBG,
  SUBL,
  SUBL_V,
  SUBQ,
  SUBQ_V,
  SUBS,
  SUBS_C,
  SUBS_D,
  SUBS_M,
  SUBS_SU,
  SUBS_SUC,
  SUBS_SUD,
  SUBS_SUI,
  SUBS_SUIC,
  SUBS_SUID,
  SUBS_SUIM,
  SUBS_SUM,
  SUBS_U,
  SUBS_UC,
  SUBS_UD,
  SUBS_UM,
  SUBT,
  SUBT_C,
  SUBT_D,
  SUBT_M,
  SUBT_SU,
  SUBT_SUC,
  SUBT_SUD,
  SUBT_SUI,
  SUBT_SUIC,
  SUBT_SUID,
  SUBT_SUIM,
  SUBT_SUM,
  SUBT_U,
  SUBT_UC,
  SUBT_UD,
  SUBT_UM,
  SWASTEN,
  SWPCTX,
  SWPIPL,
  SWPPAL,
  TRAPB,
  TRU64_RDPS,
  TRU64_RDUSP,
  TRU64_RDVAL,
  TRU64_RETSYS,
  TRU64_SWPCTX,
  TRU64_TBI,
  TRU64_WHAMI,
  TRU64_WRVPTPTR,
  TRU64_SWPIPL,
  UMULH,
  UNPKBL,
  UNPKBW,
  WMB,
  WRENT,
  WRITE_UNQ,
  WRKGP,
  WRMCES,
  WRPERFMON,
  WRPRBR,
  WRPR_PTBR,
  WRSYSPTB,
  WRUSP,
  WRVAL,
  WRVIRBND,
  WR_PS_SW,
  WTINT,
  XOR,
  ZAP,
  ZAPNOT,
  COUNT
	

   
};

enum class Opcode10 : quint8 {
    ADDL = 0x00,      // Add Longword
    S4ADDL = 0x02,    // Scaled 4-bit Add Longword
    S8ADDL = 0x12,    // Scaled 8-bit Add Longword
    ADDQ = 0x20,      // Add Quadword
    S4ADDQ = 0x22,    // Scaled 4-bit Add Quadword
    S8ADDQ = 0x32,    // Scaled 8-bit Add Quadword
    S4SUBL = 0x0B,    // Scaled 4-bit Subtract Longword
    S8SUBL = 0x1B,    // Scaled 8-bit Subtract Longword
    CMPULT = 0x1D,    // Compare Unsigned Less Than
    S4SUBQ = 0x2B,    // Scaled 4-bit Subtract Quadword
    S8SUBQ = 0x3B,    // Scaled 8-bit Subtract Quadword
    CMPULE = 0x3D,    // Compare Unsigned Less Than or Equal
    ADDL_V = 0x40,    // Add Longword with Overflow Trap
    ADDQ_V = 0x60,    // Add Quadword with Overflow Trap
    MAX_CNT
};

// === Logical Operate (Opcode 0x11) Functions ===
enum class Opcode11 : quint8 {
    AND = 0x00,        // Logical AND
    BIC = 0x08,        // Bit Clear
    CMOVLBS = 0x14,    // Conditional Move Low Bit Set
    CMOVLBC = 0x16,    // Conditional Move Low Bit Clear
    BIS = 0x20,        // Logical OR
    CMOVEQ = 0x24,     // Conditional Move Equal
    ORNOT = 0x28,      // OR NOT
    XOR = 0x40,        // Logical XOR
    CMOVLT = 0x44,     // Conditional Move Less Than
    CMOVGE = 0x46,     // Conditional Move Greater or Equal
    EQV = 0x48,        // Logical Equivalence
    CMOVLE = 0x64,     // Conditional Move Less Than or Equal
    CMOVGT = 0x66,     // Conditional Move Greater Than
    MAX_CNT
};

// === Shift/Mask/Extract/Insert (Opcode 0x12) Functions ===
enum class Opcode12 : quint8 {
    MSKBL = 0x02,      // Mask Byte Low
    EXTBL = 0x06,      // Extract Byte Low
    MSKWL = 0x12,      // Mask Word Low
    EXTWL = 0x16,      // Extract Word Low
    MSKLL = 0x22,      // Mask Long Low
    EXTLL = 0x26,      // Extract Long Low
    ZAP = 0x30,        // Zero and Preserve
    ZAPNOT = 0x31,     // Zero and Preserve Not
    MSKQL = 0x32,      // Mask Quad Low
    EXTQL = 0x36,      // Extract Quad Low
    SLL = 0x39,        // Shift Left Logical
    MSKWH = 0x52,      // Mask Word High
    INSWH = 0x57,      // Insert Word High
    MSKLH = 0x62,      // Mask Long High
    INSLH = 0x67,      // Insert Long High
    MSKQH = 0x72,      // Mask Quad High
    INSQH = 0x77,      // Insert Quad High
    INSBL = 0x0B,      // Insert Byte Low
    INSWL = 0x1B,      // Insert Word Low
    INSLL = 0x2B,      // Insert Long Low
    INSQL = 0x3B,      // Insert Quad Low
    SRA = 0x3C,        // Shift Right Arithmetic
    EXTWH = 0x5A,      // Extract Word High
    EXTLH = 0x6A,      // Extract Long High
    EXTQH = 0x7A,      // Extract Quad High
    MAX_CNT
};

// === Multiply/Compare (Opcode 0x13) Functions ===
enum class Opcode13 : quint8 {
    MULL = 0x00,      // Multiply Longword
    MULQ = 0x20,      // Multiply Quadword
    UMULH = 0x30,     // Unsigned Multiply High
    MULL_V = 0x40,    // Multiply Longword with Overflow Trap
    MULQ_V = 0x60,    // Multiply Quadword with Overflow Trap
    MAX_CNT
};


// === Floating Point Operate (Opcode 0x14 - FP Misc) ===
enum class Opcode14 : quint16 {
    CVTST = 0x2C,    // Convert S to T
    CVTTQ = 0x2F,    // Convert T to Q
    CVTQS = 0x3C,    // Convert Q to S
    CVTQT = 0x3E,    // Convert Q to T
    SQRTF_UC = 0x10A,    // Square Root F Unsigned/Control
    SQRTS_UC = 0x10B,    // Square Root S Unsigned/Control
    SQRTG_UC = 0x12A,    // Square Root G Unsigned/Control
    SQRTT_UC = 0x12B,    // Square Root T Unsigned/Control
    SQRTS_UM = 0x14B,    // Square Root S Unsigned/Masked
    SQRTT_UM = 0x16B,    // Square Root T Unsigned/Masked
    SQRTF_U = 0x18A,     // Square Root F Unsigned
    SQRTS_U = 0x18B,     // Square Root S Unsigned
    SQRTG_U = 0x1AA,     // Square Root G Unsigned
    SQRTT_U = 0x1AB,     // Square Root T Unsigned
    SQRTS_UD = 0x1CB,    // Square Root S Unsigned/Denormal
    SQRTT_UD = 0x1EB,    // Square Root T Unsigned/Denormal
    SQRTF_SC = 0x40A,    // Square Root F Signed/Control
    SQRTG_SC = 0x42A,    // Square Root G Signed/Control
    SQRTF_S = 0x48A,     // Square Root F Signed
    SQRTG_S = 0x4AA,     // Square Root G Signed
    SQRTF_SUC = 0x50A,   // Square Root F Signed Unsuppressed/Control
    SQRTS_SUC = 0x50B,   // Square Root S Signed Unsuppressed/Control
    SQRTG_SUC = 0x52A,   // Square Root G Signed Unsuppressed/Control
    SQRTT_SUC = 0x52B,   // Square Root T Signed Unsuppressed/Control
    SQRTS_SUM = 0x54B,   // Square Root S Signed Unsuppressed/Masked
    SQRTT_SUM = 0x56B,   // Square Root T Signed Unsuppressed/Masked
    SQRTF_SU = 0x58A,    // Square Root F Signed Unsuppressed
    SQRTS_SU = 0x58B,    // Square Root S Signed Unsuppressed
    SQRTG_SU = 0x5AA,    // Square Root G Signed Unsuppressed
    SQRTT_SU = 0x5AB,    // Square Root T Signed Unsuppressed
    SQRTS_SUD = 0x5CB,   // Square Root S Signed Unsuppressed/Denormal
    SQRTT_SUD = 0x5EB,   // Square Root T Signed Unsuppressed/Denormal
    SQRTS_SUIC = 0x70B,  // Square Root S Signed Unsuppressed Integer Control
    SQRTT_SUIC = 0x72B,  // Square Root T Signed Unsuppressed Integer Control
    SQRTS_SUIM = 0x74B,  // Square Root S Signed Unsuppressed Integer Masked
    SQRTT_SUIM = 0x76B,  // Square Root T Signed Unsuppressed Integer Masked
    SQRTS_SUI = 0x78B,   // Square Root S Signed Unsuppressed Integer
    SQRTT_SUI = 0x7AB,   // Square Root T Signed Unsuppressed Integer
    SQRTS_SUID = 0x7CB,  // Square Root S Signed Unsuppressed Integer Denormal
    SQRTT_SUID = 0x7EB,  // Square Root T Signed Unsuppressed Integer Denormal
    MAX_CNT
};

// === Floating Point Operate (Opcode 0x15 - F, G format) ===
// === Floating Point Operate (Opcode 0x15 - F, G format) ===
enum class Opcode15 : quint16 {
    ADDF = 0x00,      // Add F
    SUBF = 0x01,      // Subtract F
    MULF = 0x02,      // Multiply F
    DIVF = 0x03,      // Divide F
    SQRTF = 0x0C,     // Square Root F
    CVTDG = 0x1E,     // Convert D to G
    ADDG = 0x20,      // Add G
    SUBG = 0x21,      // Subtract G
    MULG = 0x22,      // Multiply G
    DIVG = 0x23,      // Divide G
    CMPGEQ = 0x25,    // Compare G Equal
    CMPGLT = 0x26,    // Compare G Less Than
    CMPGLE = 0x27,    // Compare G Less Than or Equal
    SQRTG = 0x2C,     // Square Root G
    CVTGF = 0x2C,     // Convert G to F
    CVTGD = 0x2D,     // Convert G to D
    CVTGQ = 0x2F,     // Convert G to Q
    CVTQF = 0x3C,     // Convert Q to F
    CVTQG = 0x3E,     // Convert Q to G
    ADDF_SU = 0x180,  // Add F Suppress Underflow
    SUBF_SU = 0x181,  // Subtract F Suppress Underflow
    MULF_SU = 0x182,  // Multiply F Suppress Underflow
    DIVF_SU = 0x183,  // Divide F Suppress Underflow
    ADDG_SU = 0x1A0,  // Add G Suppress Underflow
    SUBG_SU = 0x1A1,  // Subtract G Suppress Underflow
    MULG_SU = 0x1A2,  // Multiply G Suppress Underflow
    DIVG_SU = 0x1A3,  // Divide G Suppress Underflow
    MAX_CNT
};
// === Floating Point Operate (Opcode 0x16 - S, T, CVT, and IEEE Extended Variants) ===
enum class Opcode16 : quint16 {
    ADDS = 0x80,      // Add S
    SUBS = 0x81,      // Subtract S
    MULS = 0x82,      // Multiply S
    DIVS = 0x83,      // Divide S
    ADDT = 0x0A0,     // Add T
    SUBT = 0x0A1,     // Subtract T
    MULT = 0x0A2,     // Multiply T
    DIVT = 0x0A3,     // Divide T
    CMPTUN = 0x0A4,   // Compare T Unordered
    CMPTEQ = 0x0A5,   // Compare T Equal
    CMPTLT = 0x0A6,   // Compare T Less Than
    CMPTLE = 0x0A7,   // Compare T Less Than or Equal
    CVTTS = 0x0AC,    // Convert T to S
    CVTTQ = 0x0AF,    // Convert T to Q
    CVTQS = 0x0BC,    // Convert Q to S
    CVTQT = 0x0BE,    // Convert Q to T
    SQRTS = 0x0C,     // Square Root S
    ADDS_U = 0x180,   // Add S Suppress Underflow
    SUBS_U = 0x181,   // Subtract S Suppress Underflow
    MULS_U = 0x182,   // Multiply S Suppress Underflow
    DIVS_U = 0x183,   // Divide S Suppress Underflow
    ADDT_U = 0x1A0,   // Add T Suppress Underflow
    SUBT_U = 0x1A1,   // Subtract T Suppress Underflow
    MULT_U = 0x1A2,   // Multiply T Suppress Underflow
    DIVT_U = 0x1A3,   // Divide T Suppress Underflow
    ADDT_SUD = 0x5E0, // Add T Suppress Underflow + Denormal
    SUBT_SUD = 0x5E1, // Subtract T Suppress Underflow + Denormal
    MULT_SUD = 0x5E2, // Multiply T Suppress Underflow + Denormal
    DIVT_SUD = 0x5E3, // Divide T Suppress Underflow + Denormal
    ADDT_SUIM = 0x760, // Add T Suppress Underflow Integer Masked
    SUBT_SUIM = 0x761, // Subtract T Suppress Underflow Integer Masked
    MULT_SUIM = 0x762, // Multiply T Suppress Underflow Integer Masked
    DIVT_SUIM = 0x763, // Divide T Suppress Underflow Integer Masked
    ADDT_SUI = 0x7A0,  // Add T Suppress Underflow Integer
    SUBT_SUI = 0x7A1,  // Subtract T Suppress Underflow Integer
    MULT_SUI = 0x7A2,  // Multiply T Suppress Underflow Integer
    DIVT_SUI = 0x7A3,  // Divide T Suppress Underflow Integer
    MAX_CNT
};


// === Floating Point Operate (Opcode 0x17 - Extended FP Misc) ===
enum class Opcode17 : quint8 {
    CVTLQ = 0x10,    // Convert Longword to Quad
    CPYS = 0x20,     // Copy Sign
    CPYSN = 0x21,    // Copy Sign Negated
    CPYSE = 0x22,    // Copy Sign Extended
    MTCR = 0x24,  // Move To FPCR
    MFCR = 0x25,  // Move From FPCR
    FCMOVEQ = 0x44,  // FP Conditional Move EQ
    FCMOVNE = 0x45,  // FP Conditional Move NE
    FCMOVLT = 0x46,  // FP Conditional Move LT
    FCMOVGE = 0x47,  // FP Conditional Move GE
    FCMOVLE = 0x48,  // FP Conditional Move LE
    FCMOVGT = 0x49,  // FP Conditional Move GT
    MAX_CNT
};
// === Memory Barrier / Synchronization (Opcode 0x18) ===
enum class Opcode18 : quint16 {
    TRAPB = 0x0000,    // Trap Barrier
    EXCB = 0x0400,     // Exception Barrier
    MB = 0x4000,       // Memory Barrier
    WMB = 0x4400,      // Write Memory Barrier
    FETCH = 0x8000,    // Fetch
    FETCH_M = 0xA000,  // Fetch and Modify
    RPCC = 0xC000,     // Read Processor Cycle Counter
    RC = 0xE000,       // Read Count Register
    ECB = 0xE800,      // Event Count Barrier
    RS = 0xF000,       // Read Status Register
    MAX_CNT
};

enum class LoadStoreFP : quint8 {
    LDF = 0x20,      // Load F
    LDG = 0x21,      // Load G
    LDS = 0x22,      // Load S
    LDT = 0x23,      // Load T
    STF = 0x24,      // Store F
    STG = 0x25,      // Store G
    STS = 0x26,      // Store S
    STT = 0x27,      // Store T
    MAX_CNT
};
enum class LoadStoreInt : quint8 {
    LDL = 0x28,      // Load Longword
    LDQ = 0x29,      // Load Quadword
    LDL_L = 0x2A,    // Load Longword Locked
    LDQ_L = 0x2B,    // Load Quadword Locked
    STL = 0x2C,      // Store Longword
    STQ = 0x2D,      // Store Quadword
    STL_C = 0x2E,    // Store Longword Conditional
    STQ_C = 0x2F,    // Store Quadword Conditional
    MAX_CNT
};



// Memory Load/Store Opcodes (0x08 - 0x0F)
enum class LoadStoreByteWord : quint8 {
    LDA = 0x08,      // Load Address
    LDAH = 0x09,     // Load Address High
    LDQ_U = 0x0B,    // Load Quadword Unaligned
    STQ_U = 0x0F,    // Store Quadword Unaligned
    MAX_CNT
};


enum class BranchOpCode : quint8 {
    BR = 0x30,       // Branch
    FBEQ = 0x31,     // Floating Branch Equal
    FBLT = 0x32,     // Floating Branch Less Than
    FBLE = 0x33,     // Floating Branch Less Than or Equal
    BSR = 0x34,      // Branch to Subroutine
    FBNE = 0x35,     // Floating Branch Not Equal
    FBGE = 0x36,     // Floating Branch Greater or Equal
    FBGT = 0x37,     // Floating Branch Greater Than
    BLBC = 0x38,     // Branch Low Bit Clear
    BEQ = 0x39,      // Branch Equal
    BLT = 0x3A,      // Branch Less Than
    BLE = 0x3B,      // Branch Less Than or Equal
    BLBS = 0x3C,     // Branch Low Bit Set
    BNE = 0x3D,      // Branch Not Equal
    BGE = 0x3E,      // Branch Greater or Equal
    BGT = 0x3F,      // Branch Greater Than
    MAX_CNT
};


enum class JumpControl : quint8 {
    JMP = 0x00,          // Jump
    JSR = 0x01,          // Jump to Subroutine
    RET = 0x02,          // Return
    JSR_COROUTINE = 0x03,// Jump to Subroutine Coroutine
    MAX_CNT
};



struct InstructionExecutionProfile {
    quint64 decodeCount = 0;
    quint64 executeCount = 0;
    quint64 totalDecodeTimeNs = 0;
    quint64 totalExecuteTimeNs = 0;
    quint64 totalEstimatedAlphaCycles = 0;
};

#endif // GRAINDEPENDENCIES_H
