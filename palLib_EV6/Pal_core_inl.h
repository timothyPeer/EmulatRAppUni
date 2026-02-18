#ifndef PALCORE_INL_H_
#define PALCORE_INL_H_


#include <QtGlobal>
#include "PAL_core.h"
#include "PalVectorId_inl.h"
#include "../faultLib/fault_core.h"
#include "coreLib/BoxRequest.h"

/**
     * @brief Get PAL function code from CALL_PAL instruction
     * @return 26-bit PAL function code (bits 0-25)
     *
     * CALL_PAL format:
     *   31:26 = Opcode (0x00)
     *   25:0  = PAL function code
     */
AXP_HOT AXP_ALWAYS_INLINE   quint32 palFunction(quint32 grainRaw) noexcept {
    return static_cast<quint32>(grainRaw & 0x3FFFFFF);  // Bits 0-25
}

AXP_HOT AXP_ALWAYS_INLINE   PalCallPalFunction decodedCallPalFunction(quint32 grainRaw) noexcept {
	return static_cast<PalCallPalFunction>(palFunction(grainRaw));
}

AXP_HOT AXP_ALWAYS_INLINE   constexpr bool palReturnRegIsValid(const PalReturnReg rr) noexcept
{
	// Valid integer return registers are R0..R3 only (today).
	const quint8 v = static_cast<quint8>(rr);
	return (v <= static_cast<quint8>(PalReturnReg::R3));
}

AXP_HOT AXP_ALWAYS_INLINE   constexpr quint8 palReturnRegToIntReg(const PalReturnReg rr) noexcept
{
	// Returns architectural integer register number for R0..R3.
	// For NONE (or any future non-int regs), returns 31 as a safe "no-op" register
	// (consistent with Alpha R31 behavior as a sink). Caller may also check validity.
	//
	// Rationale:
	//   - In hot paths we sometimes prefer a branchless fallback.
	//   - If you want stricter behavior, wrap with palReturnRegIsValid() and assert.
	const quint8 v = static_cast<quint8>(rr);
	return (v <= static_cast<quint8>(PalReturnReg::R3)) ? v : static_cast<quint8>(31);
}

AXP_HOT AXP_ALWAYS_INLINE   constexpr quint8 palReturnRegToIntRegOr(const PalReturnReg rr,
	const quint8 fallbackReg) noexcept
{
	// Same mapping as palReturnRegToIntReg(), but caller supplies fallback.
	const quint8 v = static_cast<quint8>(rr);
	return (v <= static_cast<quint8>(PalReturnReg::R3)) ? v : fallbackReg;
}

AXP_HOT AXP_ALWAYS_INLINE   constexpr bool palReturnRegIsNone(const PalReturnReg rr) noexcept
{
	return rr == PalReturnReg::NONE;
}


// ============================================================================
// resolvePalEntryPC
// ----------------------------------------------------------------------------
// Single source of truth for PAL entry calculation (EV6).
// Implements Table 5-8 named vectors AND calculated CALL_PAL entries.
// ============================================================================

AXP_HOT AXP_ALWAYS_INLINE quint64 resolvePalEntryPC(	CPUIdType cpuId,	const PendingEvent& ev) noexcept
{
	auto& iprs = globalIPRHotExt(cpuId);
	const quint64 pal_base = iprs.pal_base;

	// ------------------------------------------------------------------------
	// Case 1: CALL_PAL (calculated entry)
	// ------------------------------------------------------------------------
	if (ev.exceptionClass == ExceptionClass_EV6::CallPal)
	{
		// Architectural rule:
		//   PAL entry = PAL_BASE + (pal_function << 6)
		return pal_base + (static_cast<quint64>(ev.palFunc) << 6);
	}

	// ------------------------------------------------------------------------
	// Case 2: Hardware exception / interrupt (named vectors, Table 5-8)
	// ------------------------------------------------------------------------
	quint64 offset = 0;

	switch (ev.exceptionClass)
	{
	case ExceptionClass_EV6::Dtb_miss_double_4: offset = 0x100; break;
	case ExceptionClass_EV6::Fen:               offset = 0x200; break;
	case ExceptionClass_EV6::Unalign:           offset = 0x280; break;
	case ExceptionClass_EV6::Dtb_miss_single:   offset = 0x300; break;
	case ExceptionClass_EV6::Dfault:            offset = 0x380; break;
	case ExceptionClass_EV6::OpcDec:            offset = 0x400; break;
	case ExceptionClass_EV6::ItbAcv:            offset = 0x480; break;
	case ExceptionClass_EV6::MachineCheck:      offset = 0x500; break;
	case ExceptionClass_EV6::ItbMiss:           offset = 0x580; break;
	case ExceptionClass_EV6::Arithmetic:        offset = 0x600; break;
	case ExceptionClass_EV6::Interrupt:         offset = 0x680; break;
	case ExceptionClass_EV6::MT_FPCR:           offset = 0x700; break;
	case ExceptionClass_EV6::Reset:             offset = 0x780; break;

	default:
		// Defensive: unknown exception maps to OPCDEC
		offset = 0x400;
		break;
	}

	return pal_base + offset;
}


// ------------------------------------------------------------------------
   // resolveCallPalVector
   //
   //  Given a 7-bit CALL_PAL index (0..127), compute the corresponding
   //  PAL vector ID for EV6:
   //
   //      offset = 0x2000 + index * 0x40
   //
   //  The enumeration PalVectorId_EV6 must contain entries whose values
   //  match these offsets for indices 0..127 (CallPal_00..CallPal_7F).
   //
   // ------------------------------------------------------------------------
static AXP_HOT AXP_ALWAYS_INLINE   PalVectorId_EV6 resolveCallPalVector(quint8 callPalIndex) noexcept
{
	// Clamp the index to the architectural range 0..127.
	const quint8 idx = static_cast<quint8>(callPalIndex & 0x7F);

	const quint16 offset =
		static_cast<quint16>(0x2000u + static_cast<quint16>(idx) * 0x40u);

	return static_cast<PalVectorId_EV6>(offset);
}


// ------------------------------------------------------------------------
// extractCallPalIndexFromInstruction
//
//  Helper to derive the CALL_PAL index from a 32-bit Alpha instruction:
//
//    The EV6 HRM describes the CALL_PAL vector indexing using
//    instruction bits <7,5..0> to form a 7-bit index.
//
//    Conceptually:
//      index<6> = inst<7>
//      index<5:0> = inst<5:0>
//
//  Parameters:
//    instWord   - 32-bit Alpha instruction word for CALL_PAL.
//
//  Returns:
//    7-bit index in the range 0..127 to be passed to resolveCallPalVector.
//
//  Reference:
//    21264 HRM, Table 4-1 "PALcode Entry Points" (CALL_PAL description).
// ------------------------------------------------------------------------
static inline quint8 extractCallPalIndexFromInstruction(quint16 instWord) noexcept
{
	const quint8 low6 = static_cast<quint8>(instWord & 0x3Fu);
	const quint8 bit7 = static_cast<quint8>((instWord >> 7) & 0x1u);
	const quint8 index =
		static_cast<quint8>((bit7 << 6) | low6);  // index<6> = inst<7>

	return static_cast<quint8>(index & 0x7Fu);
}

// In your PAL service or HWPCB helper:



/**
 * @brief Map TrapCode to PAL entry reason
 * @param trapCode The trap/fault type
 * @return PalEntryReason for routing
 */
AXP_HOT AXP_ALWAYS_INLINE   PalEntryReason mapTrapToPalReason(TrapCode_Class trapCode) noexcept
{
	switch (trapCode) {
		// DTB-related faults
	case TrapCode_Class::DTB_MISS:
	case TrapCode_Class::DTB_FAULT:
	case TrapCode_Class::DTB_ACCESS_VIOLATION:
		return PalEntryReason::FAULT_DTBM;

		// ITB-related faults
	case TrapCode_Class::ITB_MISS:
	case TrapCode_Class::ITB_FAULT:
	case TrapCode_Class::ITB_ACCESS_VIOLATION:
		return PalEntryReason::FAULT_ITB;

		// Arithmetic/FP exceptions
	case TrapCode_Class::ARITHMETIC_TRAP:
	case TrapCode_Class::INTEGER_OVERFLOW:
	case TrapCode_Class::FP_OVERFLOW:
	case TrapCode_Class::FP_DISABLED:
	case TrapCode_Class::FEN_FAULT:
		return PalEntryReason::FAULT_ARITH;

		// Unaligned access
	case TrapCode_Class::UN_ALIGNED:
	case TrapCode_Class::ITB_MISALIGN_FAULT:
		return PalEntryReason::FAULT_UNALIGNED;

		// Illegal instruction (maps to ARITH for now)
	case TrapCode_Class::ILLEGAL_INSTRUCTION:
	case TrapCode_Class::OPCODE_RESERVED:
		return PalEntryReason::FAULT_ARITH;

		// Machine check (critical hardware fault)
	case TrapCode_Class::MACHINE_CHECK:
		return PalEntryReason::FAULT_ARITH;  // Or create FAULT_MCHK

	case TrapCode_Class::NONE:
	default:
		return PalEntryReason::FAULT_ARITH;  // Default fallback
	}
}

AXP_HOT AXP_ALWAYS_INLINE   PalEntryReason getFaultReason(TrapCode_Class trapCode) noexcept {
	return mapTrapToPalReason(trapCode);
}

/**
 * @brief Map ExceptionClass_EV6 to PalEntryReason for PAL entry
 *
 * @param exceptionClass The detailed exception class from pipeline
 * @return PalEntryReason The corresponding PAL entry reason
 *
 * @note This maps the detailed CPU exception taxonomy to the architectural
 *       PAL entry points defined in the Alpha Architecture Reference Manual.
 *
 * Reference: Alpha AXP Architecture Reference Manual, Section 6.4 "PAL Entry Points"
 */
static AXP_HOT AXP_ALWAYS_INLINE PalEntryReason mapExceptionToPalEntry(ExceptionClass_EV6 exceptionClass) noexcept
{
	switch (exceptionClass)
	{
		// ====================================================================
		// CALL_PAL Instruction
		// ====================================================================
	case ExceptionClass_EV6::CallPal:
		return PalEntryReason::CALL_PAL_INSTRUCTION;

		// ====================================================================
		// DTB Misses and Faults
		// ====================================================================
	case ExceptionClass_EV6::Dtb_miss_single:
	case ExceptionClass_EV6::Dtb_miss_double_4:
	case ExceptionClass_EV6::Dfault:
	case ExceptionClass_EV6::DStream:
		return PalEntryReason::FAULT_DTBM;

		// ====================================================================
		// ITB Misses and Faults
		// ====================================================================
	case ExceptionClass_EV6::ItbMiss:
		return PalEntryReason::FAULT_ITB;

		// ====================================================================
		// Access Violations (ITB and DTB)
		// ====================================================================
	case ExceptionClass_EV6::ItbAcv:
	case ExceptionClass_EV6::DtbAcv:
		return PalEntryReason::FAULT_ACV;

		// ====================================================================
		// Arithmetic Exceptions
		// ====================================================================
	case ExceptionClass_EV6::Arithmetic:
	case ExceptionClass_EV6::MT_FPCR:
		return PalEntryReason::FAULT_ARITH;

		// ====================================================================
		// Unaligned Access
		// ====================================================================
	case ExceptionClass_EV6::Unalign:
		return PalEntryReason::FAULT_UNALIGNED;

		// ====================================================================
		// Interrupts
		// ====================================================================
	case ExceptionClass_EV6::Interrupt:
		return PalEntryReason::INTERRUPT;

		// ====================================================================
		// Machine Check
		// ====================================================================
	case ExceptionClass_EV6::MachineCheck:
	case ExceptionClass_EV6::BugCheck:
		return PalEntryReason::MACHINE_CHECK;

		// ====================================================================
		// Opcode Decode Faults (map to ITB as these occur during fetch)
		// ====================================================================
	case ExceptionClass_EV6::OpcDec:
	case ExceptionClass_EV6::OpcDecFault:
	case ExceptionClass_EV6::IllegalInstruction:
	case ExceptionClass_EV6::SubsettedInstruction:
		return PalEntryReason::FAULT_ITB;

		// ====================================================================
		// FEN (Floating-point Enable) - treat as arithmetic
		// ====================================================================
	case ExceptionClass_EV6::Fen:
		return PalEntryReason::FAULT_ARITH;

		// ====================================================================
		// Special Cases - map to closest architectural entry point
		// ====================================================================
	case ExceptionClass_EV6::Reset:
	case ExceptionClass_EV6::Panic:
		return PalEntryReason::MACHINE_CHECK;

	case ExceptionClass_EV6::BreakPoint:
	case ExceptionClass_EV6::SoftwareTrap:
	case ExceptionClass_EV6::SystemService:
		return PalEntryReason::CALL_PAL_INSTRUCTION;

	case ExceptionClass_EV6::MemoryFault:
		return PalEntryReason::FAULT_DTBM;

	case ExceptionClass_EV6::PrivilegeViolation:
		return PalEntryReason::FAULT_ACV;

	case ExceptionClass_EV6::PerformanceMonitor:
		return PalEntryReason::INTERRUPT;

		// ====================================================================
		// Default / Unknown
		// ====================================================================
	case ExceptionClass_EV6::None:
	case ExceptionClass_EV6::General:
	default:
		// Fallback to machine check for safety
		return PalEntryReason::MACHINE_CHECK;
	}
}

/**
 * @brief Get exception severity level (for logging/diagnostics)
 *
 * @param exceptionClass Exception class
 * @return quint8 Severity: 0=None, 1=Recoverable, 2=Serious, 3=Fatal
 */
static AXP_HOT AXP_ALWAYS_INLINE quint8 getExceptionSeverity(ExceptionClass_EV6 exceptionClass) noexcept
{
	switch (exceptionClass)
	{
	case ExceptionClass_EV6::None:
		return 0;

		// Recoverable faults
	case ExceptionClass_EV6::ItbMiss:
	case ExceptionClass_EV6::Dtb_miss_single:
	case ExceptionClass_EV6::Dtb_miss_double_4:
	case ExceptionClass_EV6::Unalign:
	case ExceptionClass_EV6::Fen:
	case ExceptionClass_EV6::CallPal:
		return 1;

		// Serious faults
	case ExceptionClass_EV6::Arithmetic:
	case ExceptionClass_EV6::ItbAcv:
	case ExceptionClass_EV6::DtbAcv:
	case ExceptionClass_EV6::OpcDec:
	case ExceptionClass_EV6::IllegalInstruction:
	case ExceptionClass_EV6::PrivilegeViolation:
	case ExceptionClass_EV6::Dfault:
	case ExceptionClass_EV6::MemoryFault:
		return 2;

		// Fatal conditions
	case ExceptionClass_EV6::MachineCheck:
	case ExceptionClass_EV6::BugCheck:
	case ExceptionClass_EV6::Reset:
	case ExceptionClass_EV6::Panic:
		return 3;

	default:
		return 2;  // Assume serious if unknown
	}
}



/**
 * @brief Map TrapCode to PAL entry vector
 * @param trapCode The trap/fault type
 * @return PAL vector offset (add to PAL_BASE to get actual address)
 */
AXP_HOT AXP_ALWAYS_INLINE   PalVectorId_EV6 mapTrapToPalVector(TrapCode_Class trapCode) noexcept
{
	switch (trapCode) {
		// DTB miss variants
	case TrapCode_Class::DTB_MISS:
		return PalVectorId_EV6::DTB_MISS_DOUBLE;

	case TrapCode_Class::DTB_FAULT:
	case TrapCode_Class::DTB_ACCESS_VIOLATION:
		return PalVectorId_EV6::DTB_MISS_NATIVE;

		// ITB faults
	case TrapCode_Class::ITB_MISS:
		return PalVectorId_EV6::ITB_MISS;

	case TrapCode_Class::ITB_ACCESS_VIOLATION:
	case TrapCode_Class::ITB_FAULT:
		return PalVectorId_EV6::ITB_ACV;

		// Arithmetic/FP
	case TrapCode_Class::ARITHMETIC_TRAP:
	case TrapCode_Class::INTEGER_OVERFLOW:
	case TrapCode_Class::FP_OVERFLOW:
		return PalVectorId_EV6::ARITH;

	case TrapCode_Class::FP_DISABLED:
	case TrapCode_Class::FEN_FAULT:
		return PalVectorId_EV6::FEN;

		// Unaligned
	case TrapCode_Class::UN_ALIGNED:
	case TrapCode_Class::ITB_MISALIGN_FAULT:
		return PalVectorId_EV6::UNALIGN;

		// Illegal instruction
	case TrapCode_Class::ILLEGAL_INSTRUCTION:
	case TrapCode_Class::OPCODE_RESERVED:
		return PalVectorId_EV6::OPCDEC;

		// Machine check
	case TrapCode_Class::MACHINE_CHECK:
		return PalVectorId_EV6::MCHK;

	case TrapCode_Class::NONE:
	default:
		return PalVectorId_EV6::OPCDEC;  // Default to illegal instruction
	}
}
/*
  Standard Map to translate Trapcode_Class to PALVectorID vector
*/
AXP_HOT AXP_ALWAYS_INLINE   quint64 getFaultVector(TrapCode_Class trapCode) noexcept {
	return static_cast<quint64>(mapTrapToPalVector(trapCode));
}

AXP_HOT AXP_ALWAYS_INLINE   quint64 palVectorIdToVectorAddress(PalVectorId_EV6 palVectorId) noexcept {
	return static_cast<quint64>(palVectorId);
}

// ============================================================================
// palFunctionName - Zero-cost string lookup for PalCallPalFunction
// ============================================================================
// Returns a compile-time string literal for any PalCallPalFunction value.
// Used by ExecTrace and debug logging. No allocation, no QString.
// ============================================================================

inline constexpr const char* palFunctionName(PalCallPalFunction fn) noexcept
{
	switch (fn)
	{
	case PalCallPalFunction::HALT:           return "HALT";
	case PalCallPalFunction::CFLUSH:         return "CFLUSH";
	case PalCallPalFunction::DRAINA:         return "DRAINA";
	case PalCallPalFunction::LDQP:           return "LDQP";
	case PalCallPalFunction::STQP:           return "STQP";
	case PalCallPalFunction::SWPCTX:         return "SWPCTX";
	case PalCallPalFunction::MFPR_ASN:       return "MFPR_ASN";
	case PalCallPalFunction::MTPR_ASTEN:     return "MTPR_ASTEN";
	case PalCallPalFunction::MTPR_ASTSR:     return "MTPR_ASTSR";
	case PalCallPalFunction::CSERVE:         return "CSERVE";
	case PalCallPalFunction::SWPPAL:         return "SWPPAL";
	case PalCallPalFunction::MFPR_FEN:       return "MFPR_FEN";
	case PalCallPalFunction::MTPR_FEN:       return "MTPR_FEN";
	case PalCallPalFunction::MTPR_IPIR:      return "MTPR_IPIR";
	case PalCallPalFunction::MFPR_IPL:       return "MFPR_IPL";
	case PalCallPalFunction::MTPR_IPL:       return "MTPR_IPL";
	case PalCallPalFunction::MFPR_MCES:      return "MFPR_MCES";
	case PalCallPalFunction::MTPR_MCES:      return "MTPR_MCES";
	case PalCallPalFunction::MFPR_PCBB:      return "MFPR_PCBB";
	case PalCallPalFunction::MFPR_PRBR:      return "MFPR_PRBR";
	case PalCallPalFunction::MTPR_PRBR:      return "MTPR_PRBR";
	case PalCallPalFunction::MFPR_PTBR:      return "MFPR_PTBR";
	case PalCallPalFunction::MFPR_SCBB:      return "MFPR_SCBB";
	case PalCallPalFunction::MTPR_SCBB:      return "MTPR_SCBB";
	case PalCallPalFunction::MFPR_SIRR:      return "MFPR_SIRR";
	case PalCallPalFunction::MFPR_SISR:      return "MFPR_SISR";
	case PalCallPalFunction::MFPR_TBCHK:     return "MFPR_TBCHK";
	case PalCallPalFunction::MTPR_TBIA:      return "MTPR_TBIA";
	case PalCallPalFunction::MTPR_TBIAP:     return "MTPR_TBIAP";
	case PalCallPalFunction::MTPR_TBIS:      return "MTPR_TBIS";
	case PalCallPalFunction::MFPR_ESP:       return "MFPR_ESP";
	case PalCallPalFunction::MTPR_ESP:       return "MTPR_ESP";
	case PalCallPalFunction::MFPR_SSP:       return "MFPR_SSP";
	case PalCallPalFunction::MTPR_SSP:       return "MTPR_SSP";
	case PalCallPalFunction::MFPR_USP:       return "MFPR_USP";
	case PalCallPalFunction::MTPR_USP:       return "MTPR_USP";
	case PalCallPalFunction::MTPR_TBISD:     return "MTPR_TBISD";
	case PalCallPalFunction::MTPR_TBISI:     return "MTPR_TBISI";
	case PalCallPalFunction::MFPR_ASTEN:     return "MFPR_ASTEN";
	case PalCallPalFunction::MFPR_ASTSR:     return "MFPR_ASTSR";
	case PalCallPalFunction::MFPR_VPTB:      return "MFPR_VPTB";
	case PalCallPalFunction::MTPR_VPTB:      return "MTPR_VPTB";
	case PalCallPalFunction::MTPR_PERFMON:   return "MTPR_PERFMON";
	case PalCallPalFunction::WRVPTPTR_OSF:   return "WRVPTPTR_OSF";
	case PalCallPalFunction::MTPR_DATFX:     return "MTPR_DATFX";
	case PalCallPalFunction::SWPCTX_OSF:     return "SWPCTX_OSF";
	case PalCallPalFunction::WRVAL_OSF:      return "WRVAL_OSF";
	case PalCallPalFunction::RDVAL_OSF:      return "RDVAL_OSF";
	case PalCallPalFunction::TBI_OSF:        return "TBI_OSF";
	case PalCallPalFunction::WRENT_OSF:      return "WRENT_OSF";
	case PalCallPalFunction::SWPIPL_OSF:     return "SWPIPL_OSF";
	case PalCallPalFunction::RDPS_OSF:       return "RDPS_OSF";
	case PalCallPalFunction::WRKGP_OSF:      return "WRKGP_OSF";
	case PalCallPalFunction::WRUSP_OSF:      return "WRUSP_OSF";
	case PalCallPalFunction::WRPERFMON_OSF:  return "WRPERFMON_OSF";
	case PalCallPalFunction::RDUSP_OSF:      return "RDUSP_OSF";
	case PalCallPalFunction::WHAMI_OSF:      return "WHAMI_OSF";
	case PalCallPalFunction::RETSYS_OSF:     return "RETSYS_OSF";
	case PalCallPalFunction::WTINT:          return "WTINT";
	case PalCallPalFunction::MFPR_WHAMI:     return "MFPR_WHAMI";
	case PalCallPalFunction::BPT:            return "BPT";
	case PalCallPalFunction::BUGCHECK:       return "BUGCHECK";
	case PalCallPalFunction::CHME:           return "CHME";
	case PalCallPalFunction::CHMK:           return "CHMK";
	case PalCallPalFunction::CHMS:           return "CHMS";
	case PalCallPalFunction::CHMU:           return "CHMU";
	case PalCallPalFunction::IMB:            return "IMB";
	case PalCallPalFunction::INSQHIL:        return "INSQHIL";
	case PalCallPalFunction::INSQTIL:        return "INSQTIL";
	case PalCallPalFunction::INSQHIQ:        return "INSQHIQ";
	case PalCallPalFunction::INSQTIQ:        return "INSQTIQ";
	case PalCallPalFunction::INSQUEL:        return "INSQUEL";
	case PalCallPalFunction::INSQUEQ:        return "INSQUEQ";
	case PalCallPalFunction::INSQUEL_D:      return "INSQUEL_D";
	case PalCallPalFunction::INSQUEQ_D:      return "INSQUEQ_D";
	case PalCallPalFunction::PROBER:         return "PROBER";
	case PalCallPalFunction::PROBEW:         return "PROBEW";
	case PalCallPalFunction::RD_PS:          return "RD_PS";
	case PalCallPalFunction::REI:            return "REI";
	case PalCallPalFunction::REMQHIL:        return "REMQHIL";
	case PalCallPalFunction::REMQTIL:        return "REMQTIL";
	case PalCallPalFunction::REMQHIQ:        return "REMQHIQ";
	case PalCallPalFunction::REMQTIQ:        return "REMQTIQ";
	case PalCallPalFunction::REMQUEL:        return "REMQUEL";
	case PalCallPalFunction::REMQUEQ:        return "REMQUEQ";
	case PalCallPalFunction::REMQUEL_D:      return "REMQUEL_D";
	case PalCallPalFunction::REMQUEQ_D:      return "REMQUEQ_D";
	case PalCallPalFunction::SWASTEN:        return "SWASTEN";
	case PalCallPalFunction::WR_PS_SW:       return "WR_PS_SW";
	case PalCallPalFunction::RSCC:           return "RSCC";
	case PalCallPalFunction::READ_UNQ:       return "READ_UNQ";
	case PalCallPalFunction::WRITE_UNQ:      return "WRITE_UNQ";
	case PalCallPalFunction::AMOVRR:         return "AMOVRR";
	case PalCallPalFunction::AMOVRM:         return "AMOVRM";
	case PalCallPalFunction::INSQHILR:       return "INSQHILR";
	case PalCallPalFunction::INSQTILR:       return "INSQTILR";
	case PalCallPalFunction::INSQHIQR:       return "INSQHIQR";
	case PalCallPalFunction::INSQTIQR:       return "INSQTIQR";
	case PalCallPalFunction::REMQHILR:       return "REMQHILR";
	case PalCallPalFunction::REMQTILR:       return "REMQTILR";
	case PalCallPalFunction::REMHIQR:        return "REMHIQR";
	case PalCallPalFunction::REMQTIQR:       return "REMQTIQR";
	case PalCallPalFunction::GENTRAP:        return "GENTRAP";
	case PalCallPalFunction::KBPT:           return "KBPT";
	case PalCallPalFunction::CLRFEN:         return "CLRFEN";
	default:                                 return "PAL_UNKNOWN";
	}
}

// Overload taking raw uint8 from getFunctionCode
inline constexpr const char* palFunctionName(quint8 funcCode) noexcept
{
	return palFunctionName(static_cast<PalCallPalFunction>(funcCode));
}




#endif // Pal_core_inl_h__
