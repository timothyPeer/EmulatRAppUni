// ============================================================================
// PalVectorId_refined.h - Map ExceptionClass_EV6 to PAL Vector ID
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef PALVECTORID_REFINED_H
#define PALVECTORID_REFINED_H

#include <QtGlobal>

#include "exceptionLib/ExceptionClass_EV6.h"

// ============================================================================
// PalVectorId - Alpha AXP PAL Exception Vector Offsets
// ============================================================================
// Represents offsets from PAL_BASE for hardware exception entry points.
// Based on Alpha AXP Architecture Reference Manual Table 5-8.
//
// Entry PC = PAL_BASE + vector_offset
//
// CALL_PAL vectors are CALCULATED, not enumerated here.
// Use calculateCallPalEntryPC() for CALL_PAL instruction handling.
//




enum class PalVectorId_EV5 : quint16 {
	// === Hardware Exception Vectors (Table 5-8) ===
	RESET = 0x0000,  // Reset/wakeup
	IACCVIO = 0x0080,  // Instruction ACV
	INTERRUPT = 0x0100,  // Interrupts (HW/SW/AST)
	ITB_MISS = 0x0180,  // Instruction TB miss
	DTB_MISS_SINGLE = 0x0200,  // Single-level DTB miss
	DTB_MISS_DOUBLE = 0x0280,  // 3-level page table walk
	UNALIGN = 0x0300,  // Unaligned access
	DFAULT = 0x0380,  // Data fault/ACV/sign check
	MCHK = 0x0400,  // Machine check
	OPCDEC = 0x0480,  // Illegal opcode
	ARITH = 0x0500,  // Arithmetic trap
	FEN = 0x0580,  // Floating-point disabled

	// === SMP Extension Vectors ===
	IPI_INTERRUPT = 0x0600,  // Inter-processor interrupt (SMP systems)

	// ------------------------------------------------------------------------
	// CALL_PAL Vectors (128 entries: 0x00-0x7F)
	//
	//   vectorOffset = 0x2000 + (index * 0x40)
	//
	// These represent PALcode CALL_PAL entrypoints 00-7F.
	//   CALL_PAL are calculated offsets. 
	//   CFLUSH
	//   DRAINA
	//   BPT
	//	 BUGCHECK
	//   GENTRAP
	//   HALT
	//   IMB
	//   READ_UNQ
	//   SWPPAL
	//	 WRITE_UNQ
	// ------------------------------------------------------------------------
	CALL_CENTRY_BEG = 0x2000,
	CallPal_01 = 0x2040,        // BUGCHECK
	CallPal_02 = 0x2080,        // GENTRAP
	CallPal_03 = 0x20C0,        // (Reserved/Implementation-specific)
	CallPal_04 = 0x2100,
	CallPal_05 = 0x2140,
	CallPal_06 = 0x2180,
	CallPal_07 = 0x21C0,
	CallPal_08 = 0x2200,
	CallPal_09 = 0x2240,        // CSERVE (Console Service - OpenVMS)
	CallPal_0A = 0x2280,
	CallPal_0B = 0x22C0,
	CallPal_0C = 0x2300,
	CallPal_0D = 0x2340,
	CallPal_0E = 0x2380,
	CallPal_0F = 0x23C0,

	CallPal_10 = 0x2400,
	CallPal_11 = 0x2440,
	CallPal_12 = 0x2480,
	CallPal_13 = 0x24C0,
	CallPal_14 = 0x2500,
	CallPal_15 = 0x2540,
	CallPal_16 = 0x2580,
	CallPal_17 = 0x25C0,
	CallPal_18 = 0x2600,
	CallPal_19 = 0x2640,
	CallPal_1A = 0x2680,
	CallPal_1B = 0x26C0,
	CallPal_1C = 0x2700,
	CallPal_1D = 0x2740,
	CallPal_1E = 0x2780,
	CallPal_1F = 0x27C0,

	CallPal_20 = 0x2800,
	CallPal_21 = 0x2840,
	CallPal_22 = 0x2880,
	CallPal_23 = 0x28C0,
	CallPal_24 = 0x2900,
	CallPal_25 = 0x2940,
	CallPal_26 = 0x2980,
	CallPal_27 = 0x29C0,
	CallPal_28 = 0x2A00,
	CallPal_29 = 0x2A40,
	CallPal_2A = 0x2A80,
	CallPal_2B = 0x2AC0,
	CallPal_2C = 0x2B00,
	CallPal_2D = 0x2B40,
	CallPal_2E = 0x2B80,
	CallPal_2F = 0x2BC0,

	CallPal_30 = 0x2C00,
	CallPal_31 = 0x2C40,
	CallPal_32 = 0x2C80,
	CallPal_33 = 0x2CC0,
	CallPal_34 = 0x2D00,
	CallPal_35 = 0x2D40,
	CallPal_36 = 0x2D80,
	CallPal_37 = 0x2DC0,
	CallPal_38 = 0x2E00,
	CallPal_39 = 0x2E40,
	CallPal_3A = 0x2E80,
	CallPal_3B = 0x2EC0,
	CallPal_3C = 0x2F00,
	CallPal_3D = 0x2F40,
	CallPal_3E = 0x2F80,
	CallPal_3F = 0x2FC0,
	// Unprivileged  CALL_PAL
	CallPal_40 = 0x3000,
	CallPal_41 = 0x3040,
	CallPal_42 = 0x3080,
	CallPal_43 = 0x30C0,
	CallPal_44 = 0x3100,
	CallPal_45 = 0x3140,
	CallPal_46 = 0x3180,
	CallPal_47 = 0x31C0,
	CallPal_48 = 0x3200,
	CallPal_49 = 0x3240,
	CallPal_4A = 0x3280,
	CallPal_4B = 0x32C0,
	CallPal_4C = 0x3300,
	CallPal_4D = 0x3340,
	CallPal_4E = 0x3380,
	CallPal_4F = 0x33C0,

	CallPal_50 = 0x3400,
	CallPal_51 = 0x3440,
	CallPal_52 = 0x3480,
	CallPal_53 = 0x34C0,
	CallPal_54 = 0x3500,
	CallPal_55 = 0x3540,
	CallPal_56 = 0x3580,
	CallPal_57 = 0x35C0,
	CallPal_58 = 0x3600,
	CallPal_59 = 0x3640,
	CallPal_5A = 0x3680,
	CallPal_5B = 0x36C0,
	CallPal_5C = 0x3700,
	CallPal_5D = 0x3740,
	CallPal_5E = 0x3780,
	CallPal_5F = 0x37C0,

	CallPal_60 = 0x3800,
	CallPal_61 = 0x3840,
	CallPal_62 = 0x3880,
	CallPal_63 = 0x38C0,
	CallPal_64 = 0x3900,
	CallPal_65 = 0x3940,
	CallPal_66 = 0x3980,
	CallPal_67 = 0x39C0,
	CallPal_68 = 0x3A00,
	CallPal_69 = 0x3A40,
	CallPal_6A = 0x3A80,
	CallPal_6B = 0x3AC0,
	CallPal_6C = 0x3B00,
	CallPal_6D = 0x3B40,
	CallPal_6E = 0x3B80,
	CallPal_6F = 0x3BC0,

	CallPal_70 = 0x3C00,
	CallPal_71 = 0x3C40,
	CallPal_72 = 0x3C80,
	CallPal_73 = 0x3CC0,
	CallPal_74 = 0x3D00,
	CallPal_75 = 0x3D40,
	CallPal_76 = 0x3D80,
	CallPal_77 = 0x3DC0,
	CallPal_78 = 0x3E00,
	CallPal_79 = 0x3E40,
	CallPal_7A = 0x3E80,
	CallPal_7B = 0x3EC0,
	CallPal_7C = 0x3F00,
	CallPal_7D = 0x3F40,
	CallPal_7E = 0x3F80,
	CallPal_7F = 0x3FC0,        // Last CALL_PAL entry

	// === Sentinel ===
	INVALID = 0xFFFF
};


enum class PalVectorId_EV6 : quint16 {
	// === EV6 (21264) Hardware Exception Vectors ===
  // Ref: Alpha 21264 Hardware Reference Manual, Table 5-8
	RESET = 0x0000,  // Reset/wakeup
	MCHK = 0x0080,  // Machine check (uncorrectable HW error)
	ARITH = 0x0100,  // Arithmetic exception
	INTERRUPT = 0x0180,  // Interrupts (HW/SW/AST, incl. corrected errors)
	DTB_MISS_SINGLE = 0x0200,  // Single-level DTB miss
	DTB_MISS_DOUBLE = 0x0280,  // Double (3-level page table walk) DTB miss
	ITB_MISS = 0x0300,  // Instruction TB miss
	ITB_ACV = 0x0380,  // I-stream access violation
	DTB_MISS_NATIVE = 0x0400,  // DTB miss (native mode)
	UNALIGN = 0x0480,  // Unaligned access
	OPCDEC = 0x0500,  // Illegal/privileged opcode
	FEN = 0x0580,  // FP disabled
	
	// === CALL_PAL Vectors UNCHANGED (same all generations) ===
	// Privileged:   0x2000 + ([5:0] << 6)  functions 0x00-0x3F
	// Unprivileged: 0x3000 + ([5:0] << 6)  functions 0x80-0xBF
	CALL_CENTRY_BEG = 0x2000,
	CallPal_01 = 0x2040,        // BUGCHECK
	CallPal_02 = 0x2080,        // GENTRAP
	CallPal_03 = 0x20C0,        // (Reserved/Implementation-specific)
	CallPal_04 = 0x2100,
	CallPal_05 = 0x2140,
	CallPal_06 = 0x2180,
	CallPal_07 = 0x21C0,
	CallPal_08 = 0x2200,
	CallPal_09 = 0x2240,        // CSERVE (Console Service - OpenVMS)
	CallPal_0A = 0x2280,
	CallPal_0B = 0x22C0,
	CallPal_0C = 0x2300,
	CallPal_0D = 0x2340,
	CallPal_0E = 0x2380,
	CallPal_0F = 0x23C0,

	CallPal_10 = 0x2400,
	CallPal_11 = 0x2440,
	CallPal_12 = 0x2480,
	CallPal_13 = 0x24C0,
	CallPal_14 = 0x2500,
	CallPal_15 = 0x2540,
	CallPal_16 = 0x2580,
	CallPal_17 = 0x25C0,
	CallPal_18 = 0x2600,
	CallPal_19 = 0x2640,
	CallPal_1A = 0x2680,
	CallPal_1B = 0x26C0,
	CallPal_1C = 0x2700,
	CallPal_1D = 0x2740,
	CallPal_1E = 0x2780,
	CallPal_1F = 0x27C0,

	CallPal_20 = 0x2800,
	CallPal_21 = 0x2840,
	CallPal_22 = 0x2880,
	CallPal_23 = 0x28C0,
	CallPal_24 = 0x2900,
	CallPal_25 = 0x2940,
	CallPal_26 = 0x2980,
	CallPal_27 = 0x29C0,
	CallPal_28 = 0x2A00,
	CallPal_29 = 0x2A40,
	CallPal_2A = 0x2A80,
	CallPal_2B = 0x2AC0,
	CallPal_2C = 0x2B00,
	CallPal_2D = 0x2B40,
	CallPal_2E = 0x2B80,
	CallPal_2F = 0x2BC0,

	CallPal_30 = 0x2C00,
	CallPal_31 = 0x2C40,
	CallPal_32 = 0x2C80,
	CallPal_33 = 0x2CC0,
	CallPal_34 = 0x2D00,
	CallPal_35 = 0x2D40,
	CallPal_36 = 0x2D80,
	CallPal_37 = 0x2DC0,
	CallPal_38 = 0x2E00,
	CallPal_39 = 0x2E40,
	CallPal_3A = 0x2E80,
	CallPal_3B = 0x2EC0,
	CallPal_3C = 0x2F00,
	CallPal_3D = 0x2F40,
	CallPal_3E = 0x2F80,
	CallPal_3F = 0x2FC0,
	// Unprivileged  CALL_PAL
	CallPal_40 = 0x3000,
	CallPal_41 = 0x3040,
	CallPal_42 = 0x3080,
	CallPal_43 = 0x30C0,
	CallPal_44 = 0x3100,
	CallPal_45 = 0x3140,
	CallPal_46 = 0x3180,
	CallPal_47 = 0x31C0,
	CallPal_48 = 0x3200,
	CallPal_49 = 0x3240,
	CallPal_4A = 0x3280,
	CallPal_4B = 0x32C0,
	CallPal_4C = 0x3300,
	CallPal_4D = 0x3340,
	CallPal_4E = 0x3380,
	CallPal_4F = 0x33C0,

	CallPal_50 = 0x3400,
	CallPal_51 = 0x3440,
	CallPal_52 = 0x3480,
	CallPal_53 = 0x34C0,
	CallPal_54 = 0x3500,
	CallPal_55 = 0x3540,
	CallPal_56 = 0x3580,
	CallPal_57 = 0x35C0,
	CallPal_58 = 0x3600,
	CallPal_59 = 0x3640,
	CallPal_5A = 0x3680,
	CallPal_5B = 0x36C0,
	CallPal_5C = 0x3700,
	CallPal_5D = 0x3740,
	CallPal_5E = 0x3780,
	CallPal_5F = 0x37C0,

	CallPal_60 = 0x3800,
	CallPal_61 = 0x3840,
	CallPal_62 = 0x3880,
	CallPal_63 = 0x38C0,
	CallPal_64 = 0x3900,
	CallPal_65 = 0x3940,
	CallPal_66 = 0x3980,
	CallPal_67 = 0x39C0,
	CallPal_68 = 0x3A00,
	CallPal_69 = 0x3A40,
	CallPal_6A = 0x3A80,
	CallPal_6B = 0x3AC0,
	CallPal_6C = 0x3B00,
	CallPal_6D = 0x3B40,
	CallPal_6E = 0x3B80,
	CallPal_6F = 0x3BC0,

	CallPal_70 = 0x3C00,
	CallPal_71 = 0x3C40,
	CallPal_72 = 0x3C80,
	CallPal_73 = 0x3CC0,
	CallPal_74 = 0x3D00,
	CallPal_75 = 0x3D40,
	CallPal_76 = 0x3D80,
	CallPal_77 = 0x3DC0,
	CallPal_78 = 0x3E00,
	CallPal_79 = 0x3E40,
	CallPal_7A = 0x3E80,
	CallPal_7B = 0x3EC0,
	CallPal_7C = 0x3F00,
	CallPal_7D = 0x3F40,
	CallPal_7E = 0x3F80,
	CallPal_7F = 0x3FC0,        // Last CALL_PAL entry

	// === Sentinel ===
	INVALID = 0xFFFF
};
// ============================================================================
// PalVectorEntry - Descriptor for PAL vector table
// ============================================================================
struct PalVectorEntry
{
	PalVectorId_EV5 vectorId_ev5{ PalVectorId_EV5::INVALID };
	PalVectorId_EV6 vectorId{ PalVectorId_EV6::INVALID };
	quint64     entryPC{ 0 };      // Absolute PC (PAL_BASE + offset)
	quint8      targetIPL{ 0 };    // Target IPL for this vector
	quint8      requiredCM{ 0 };   // Required current mode (0=kernel)
	quint32     flags{ 0 };        // Vector behavior flags
	const char* name{ nullptr };   // Vector name (for debugging)

	// Flags
	enum : quint8 {
		NONE = 0,
		SAVES_STATE = (1 << 0),  // Saves PC/PS to HWPCB
		RESTARTABLE = (1 << 1),  // Can restart faulting instruction
		MODIFIES_IPL = (1 << 2),  // Changes IPL
		REQUIRES_KERNEL = (1 << 3)   // Requires kernel mode
	};
};

// ============================================================================
// PalVectorId to String
// ============================================================================
inline const char* palVectorName(PalVectorId_EV6 vec) noexcept
{
	switch (vec)
	{
	case PalVectorId_EV6::DTB_MISS_DOUBLE: return "DTB_MISS_DOUBLE_3";
	case PalVectorId_EV6::FEN:               return "FEN";
	case PalVectorId_EV6::UNALIGN:           return "UNALIGN";
	case PalVectorId_EV6::DTB_MISS_SINGLE:   return "DTB_MISS_SINGLE";
	case PalVectorId_EV6::DTB_MISS_NATIVE:	 return "DTB_MISS_NATIVE";
	case PalVectorId_EV6::OPCDEC:            return "OPCDEC";
	case PalVectorId_EV6::ITB_ACV:           return "IACV";
	case PalVectorId_EV6::MCHK:              return "MCHK";
	case PalVectorId_EV6::ITB_MISS:          return "ITB_MISS";
	case PalVectorId_EV6::ARITH:             return "ARITH";
	case PalVectorId_EV6::INTERRUPT:         return "INTERRUPT"; // IPI_INTERRUPT, HW/SW/AST, Corrected Errors
	case PalVectorId_EV6::RESET:             return "RESET";
	case PalVectorId_EV6::INVALID:           return "INVALID";
	default:                             return "UNKNOWN";
	}
}

/**
 * @brief Map ExceptionClass_EV6 to PAL Vector ID
 *
 * @param exceptionClass CPU exception class
 * @return PalVectorId Corresponding PAL entry vector
 *
 * @note CALL_PAL instructions don't use vector dispatch - they use the
 *       PAL function code to index into a separate dispatch table.
 *       This function returns PalVectorId::CALLSYS as a placeholder.
 *
 * Reference: 21264 HRM Table 4-1, Alpha AXP ARM Section 6.4
 */
static AXP_HOT AXP_ALWAYS_INLINE PalVectorId_EV6 mapExceptionToPalVector(ExceptionClass_EV6 exceptionClass) noexcept
{
	switch (exceptionClass)
	{
		// ====================================================================
		// System Reset
		// ====================================================================
	case ExceptionClass_EV6::Reset:
		return PalVectorId_EV6::RESET;

		// ====================================================================
		// Machine Check
		// ====================================================================
	case ExceptionClass_EV6::MachineCheck:
	case ExceptionClass_EV6::BugCheck:
	case ExceptionClass_EV6::Panic:
		return PalVectorId_EV6::MCHK;

		// ====================================================================
		// Arithmetic Exceptions
		// ====================================================================
	case ExceptionClass_EV6::Arithmetic:
	case ExceptionClass_EV6::MT_FPCR:
		return PalVectorId_EV6::ARITH;

		// ====================================================================
		// External Interrupts
		// ====================================================================
	case ExceptionClass_EV6::Interrupt:
	case ExceptionClass_EV6::PerformanceMonitor:
		return PalVectorId_EV6::INTERRUPT;

		// ====================================================================
		// D-stream Faults (general)
		// ====================================================================
	case ExceptionClass_EV6::DStream:
	case ExceptionClass_EV6::Dfault:
	case ExceptionClass_EV6::MemoryFault:
		return PalVectorId_EV6::DTB_MISS_SINGLE;
		// ====================================================================
		// ITB Miss
		// ====================================================================
	case ExceptionClass_EV6::ItbMiss:
		return PalVectorId_EV6::ITB_MISS;

		// ====================================================================
		// ITB Access Violation
		// ====================================================================
	case ExceptionClass_EV6::ItbAcv:
		return PalVectorId_EV6::ITB_ACV;

		// ====================================================================
		// Opcode Decode Faults
		// ====================================================================
	case ExceptionClass_EV6::OpcDec:
	case ExceptionClass_EV6::OpcDecFault:
	case ExceptionClass_EV6::IllegalInstruction:
	case ExceptionClass_EV6::SubsettedInstruction:
	case ExceptionClass_EV6::PrivilegeViolation:
		return PalVectorId_EV6::OPCDEC;

		// ====================================================================
		// Floating-Point Disabled
		// ====================================================================
	case ExceptionClass_EV6::Fen:
		return PalVectorId_EV6::FEN;

		// ====================================================================
		// Unaligned Access
		// ====================================================================
	case ExceptionClass_EV6::Unalign:
		return PalVectorId_EV6::UNALIGN;

		// ====================================================================
		// DTB Miss Native Mode
		// ====================================================================
	case ExceptionClass_EV6::Dtb_miss_native:
		return PalVectorId_EV6::DTB_MISS_NATIVE;
		// ====================================================================
		// DTB Miss - Single
		// ====================================================================
	case ExceptionClass_EV6::Dtb_miss_single:
		return PalVectorId_EV6::DTB_MISS_SINGLE;

		// ====================================================================
		// DTB Miss - Double
		// ====================================================================
	case ExceptionClass_EV6::Dtb_miss_double_4:
		return PalVectorId_EV6::DTB_MISS_DOUBLE;

		// ====================================================================
		// DTB Access Violation
		// ====================================================================
	case ExceptionClass_EV6::DtbAcv:
		return PalVectorId_EV6::DTB_MISS_SINGLE;
		// ====================================================================
		// CALL_PAL (uses function dispatch, not vector table) - these classes are computed
		// ====================================================================
	// case ExceptionClass_EV6::CallPal:
	// case ExceptionClass_EV6::SoftwareTrap:
	// case ExceptionClass_EV6::SystemService:
	// case ExceptionClass_EV6::BreakPoint:
	// 	return PalVectorId::CALLSYS;

		// ====================================================================
		// Default / Unknown
		// ====================================================================
	case ExceptionClass_EV6::None:
	case ExceptionClass_EV6::General:
	default:
		return PalVectorId_EV6::MCHK;  // Safest fallback
	}
}

/**
 * @brief Compute PAL entry PC from vector ID
 *
 * @param vectorId PAL vector identifier
 * @param palBase PAL base address (from IPR SCBB)
 * @return quint64 PAL entry point address
 *
 * @note Standard Alpha convention: each vector is 16 bytes (0x10) apart
 *       Entry PC = PAL_BASE + (vector_id * 0x10)
 *
 * @warning CALL_PAL instructions use a different mechanism - they compute
 *          entry PC as: PAL_BASE + 0x2000 + (function_code * 0x40)
 */
static AXP_HOT AXP_ALWAYS_INLINE quint64 computePalVectorPC(PalVectorId_EV6 vectorId, quint64 palBase) noexcept
{
	// Standard vector offset calculation
	// Each vector entry is 16 bytes (0x10) apart
	const quint64 vectorOffset = static_cast<quint64>(vectorId);

	return palBase + vectorOffset;
}

#endif // PALVECTORID_REFINED_H