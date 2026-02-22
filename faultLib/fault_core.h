// ============================================================================
// fault_core.h - Maps TranslationResult to TrapCode_Class for instruction fetches
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

#pragma once
#include <QtGlobal>
#include <QString>
#include "../palLib_EV6/PalVectorId_refined.h"
#include "../coreLib/TrapsAndFaults_inl_helpers.h"
#include "../exceptionLib/ExceptionClass_EV6.h"
#include "../exceptionLib/PendingEventKind.h"
#include "../coreLib/VA_types.h"


// ============================================================================
// OPCODE/DECODE SUBTYPE (when MemoryFaultType == OPCODE_FAULT)
// ============================================================================
// Helps distinguish illegal vs reserved vs illegal PAL uses.
// ( Invalid opcode, reserved instruction, illegal PAL)
//
enum class OpcodeFaultKind : quint8
{
	NONE = 0,
	OPCDEC = 1,           // Invalid/reserved opcode (Alpha OPCDEC vector 0x03C0)
	RESOP = 2,            // Reserved operand (Alpha RESOP - if implemented)
	ILLEGAL_PAL = 3,      // Illegal CALL_PAL or PALcode violation
	PRIVILEGED = 4,       // Privileged instruction in user mode
	OPCODE_INVALID,           // Decoded pattern not defined (OPCDEC)
	OPCODE_RESERVED          // Reserved/implementation-defined (RESOP)
};

// ============================================================================
// ARITHMETIC/FP SUBTYPE (when MemoryFaultType == ARITHMETIC_TRAP)
// ============================================================================
// Alpha's arithmetic traps primarily cover FP; keep flags explicit to enable
// FPCR-based behavior (rounding, trap enable, sticky) in your dt_* types.
// ( Floating-point Exceptions & FPCR bits)
//
enum class ArithmeticTrapKind : quint16
{
	NONE = 0,
    INVALID_OPERATION = 0x0001,  // INV (bit 1 in FPCR) - Invalid operation (e.g., 0/0, sqrt(-1))
    DIVIDE_BY_ZERO = 0x0002,     // DZE (bit 2 in FPCR) - Division by zero
    OVER_FLOW = 0x0004,           // OVF (bit 3 in FPCR) - Overflow
    UNDER_FLOW = 0x0008,          // UNF (bit 4 in FPCR) - Underflow
    INEXACT = 0x0010,
	FP_INVALID,               // INV
	FP_DIVIDE_BY_ZERO,        // DZE
	FP_OVERFLOW,              // OVF
	FP_UNDERFLOW,             // UNF
	FP_INEXACT,               // INE
	// Optional: integer divide-by-zero, if you choose to surface it distinctly
	INT_DIVIDE_BY_ZERO,
	// IEEE FP exceptions (standard)
                // INE (bit 5 in FPCR) - Inexact result

	// Alpha-specific
	INTEGER_OVERFLOW = 0x0020   // IOV (bit 6 in FPCR) - Integer overflow (if enabled)
};


// Check if arithmetic trap kind includes a specific exception
inline bool hasArithmeticException(ArithmeticTrapKind kind, ArithmeticTrapKind flag) {
	return (static_cast<quint16>(kind) & static_cast<quint16>(flag)) != 0;
}

struct FaultEventState {
	PendingEventKind kind;
	quint64   va;
	quint64   pc;
	quint32   asn;
	bool      deferred;
};

// ========================================================================
// Map struct to ExceptionClass and PalVectorId
// ========================================================================
struct TrapMapping {
    ExceptionClass_EV6 exceptionClass;
    PalVectorId_EV6 palVectorId;
    PendingEventKind eventKind;
};

enum class FaultType_PTE {
	None,
	TNV,
	FOW,
	FOR_,
	FOE
};

// ============================================================================
// FAULT/TRAP CLASSIFICATION (needs to match your TrapCode_Class enum)
// ============================================================================

enum class TrapCode_Class : quint8 {
	NONE,
	ARITHMETIC_TRAP,
	DTB_MISS,
	DTB_FAULT,
	DTB_ACCESS_VIOLATION,
	FP_DISABLED,
	FP_OVERFLOW,
	ILLEGAL_INSTRUCTION,
	INTEGER_OVERFLOW,
	ITB_ACCESS_VIOLATION,
	ITB_FAULT,
	ITB_MISS,
	ITB_MISALIGN_FAULT,
	MACHINE_CHECK,
	OPCODE_RESERVED,
	PRIVILEGE_VIOLATION,
	ALIGNMENT_FAULT,
	UN_ALIGNED,
	FEN_FAULT,
	TRANSLATION_FAULT
	
};

// ============================================================================
// Fault Cause to String Conversion
// ============================================================================
inline const char* faultCauseName(FaultCause cause) noexcept
{
	switch (cause)
	{
	case FaultCause::FAULT_UNKNOWN:            return "UNKNOWN";
	case FaultCause::FAULT_NONE:               return "NONE";

		// TLB/MMU faults
	case FaultCause::TLB_MISS_DTLB:            return "TLB_MISS_DTLB";
	case FaultCause::TLB_MISS_ITLB:            return "TLB_MISS_ITLB";
	case FaultCause::TLB_ACCESS_VIOLATION:     return "TLB_ACCESS_VIOLATION";
	case FaultCause::TLB_MOD_FAULT:            return "TLB_MOD_FAULT";
	case FaultCause::PAGE_NOT_PRESENT:         return "PAGE_NOT_PRESENT";
	case FaultCause::PAGE_PROTECTION:          return "PAGE_PROTECTION";
	case FaultCause::PAGE_DIRTY:               return "PAGE_DIRTY";

		// Alignment faults
	case FaultCause::UNALIGNED_LOAD:           return "UNALIGNED_LOAD";
	case FaultCause::UNALIGNED_STORE:          return "UNALIGNED_STORE";
	case FaultCause::UNALIGNED_INSTRUCTION:    return "UNALIGNED_INSTRUCTION";

		// Arithmetic faults
	case FaultCause::INTEGER_OVERFLOW:         return "INTEGER_OVERFLOW";
	case FaultCause::INTEGER_DIVIDE_BY_ZERO:   return "INTEGER_DIVIDE_BY_ZERO";
	case FaultCause::FP_OVERFLOW:              return "FP_OVERFLOW";
	case FaultCause::FP_UNDERFLOW:             return "FP_UNDERFLOW";
	case FaultCause::FP_INEXACT:               return "FP_INEXACT";
	case FaultCause::FP_INVALID_OP:            return "FP_INVALID_OP";
	case FaultCause::FP_DIVIDE_BY_ZERO:        return "FP_DIVIDE_BY_ZERO";
	case FaultCause::FP_DENORMAL:              return "FP_DENORMAL";

		// Opcode/instruction faults
	case FaultCause::ILLEGAL_OPCODE:           return "ILLEGAL_OPCODE";
	case FaultCause::ILLEGAL_OPERAND:          return "ILLEGAL_OPERAND";
	case FaultCause::PRIVILEGED_INSTRUCTION:   return "PRIVILEGED_INSTRUCTION";
	case FaultCause::FEN_DISABLED:             return "FEN_DISABLED";

		// System faults
	case FaultCause::MACHINE_CHECK:            return "MACHINE_CHECK";
	case FaultCause::SYSTEM_RESET:             return "SYSTEM_RESET";
	case FaultCause::BUGCHECK:                 return "BUGCHECK";

		// Software exceptions
	case FaultCause::BREAKPOINT:               return "BREAKPOINT";

	case FaultCause::GENTRAP:                  return "GENTRAP";
	case FaultCause::SOFTWARE_TRAP:            return "SOFTWARE_TRAP";

	default:                                    return "INVALID_FAULT_CAUSE";
	}
}



/**
 * @brief Maps TranslationResult to TrapCode_Class for instruction fetches
 */
inline TrapCode_Class mapITranslationFault(TranslationResult result) noexcept
{
	switch (result) {
	case TranslationResult::Success:
		return TrapCode_Class::NONE;

	case TranslationResult::TlbMiss:
		return TrapCode_Class::ITB_MISS;

	case TranslationResult::NonCanonical:
	case TranslationResult::PageNotPresent:
	case TranslationResult::FaultOnExecute:
	case TranslationResult::BusError:
		return TrapCode_Class::ITB_FAULT;

	case TranslationResult::AccessViolation:
		return TrapCode_Class::ITB_ACCESS_VIOLATION;

	case TranslationResult::Unaligned:
		return TrapCode_Class::ITB_MISALIGN_FAULT;

		// These shouldn't happen for instruction fetch, but handle safely
	case TranslationResult::DlbMiss:
	case TranslationResult::FaultOnRead:
	case TranslationResult::FaultOnWrite:
		return TrapCode_Class::ITB_FAULT;

	default:
		return TrapCode_Class::ITB_FAULT;
	}
}

/**
 * @brief Maps TranslationResult to TrapCode_Class for data accesses
 */
inline TrapCode_Class mapDTranslationFault(TranslationResult result) noexcept
{
	switch (result) {
	case TranslationResult::Success:
		return TrapCode_Class::NONE;

	case TranslationResult::TlbMiss:
	case TranslationResult::DlbMiss:
		return TrapCode_Class::DTB_MISS;

	case TranslationResult::NonCanonical:
	case TranslationResult::PageNotPresent:
	case TranslationResult::FaultOnRead:
	case TranslationResult::FaultOnWrite:
	case TranslationResult::BusError:
		return TrapCode_Class::DTB_FAULT;

	case TranslationResult::AccessViolation:
		return TrapCode_Class::DTB_ACCESS_VIOLATION;

	case TranslationResult::Unaligned:
		return TrapCode_Class::ALIGNMENT_FAULT;

		// This shouldn't happen for data access, but handle safely
	case TranslationResult::FaultOnExecute:
		return TrapCode_Class::DTB_FAULT;

	default:
		return TrapCode_Class::DTB_FAULT;
	}
}

/**
 * @brief Maps TranslationResult to TrapCode_Class with explicit I/D side
 * @param result Translation result
 * @param isInstruction true for I-side, false for D-side
 */
inline TrapCode_Class mapTranslationFault(TranslationResult result, bool isInstruction) noexcept
{
	return isInstruction ? mapITranslationFault(result) : mapDTranslationFault(result);
}

