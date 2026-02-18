#pragma once
#include <QtGlobal>
#include "exception_helpers.h"
/*
  Register is READ-ONLY
  Instruction: HW_MFPR

  Considerations:
   The register is updated at trap deliver time.  Its contents are valid only if it is read 
   (by way of a HW_MFPR) in the first fetch block of the exception handler. 
   There are three types of traps for which this register captures related information.
   -- Arithmetic Traps:  The instruction generated an exceptional condition that should be reported to the operating system, and/or the FPCR status bit 
      associated with this condition is clear and should be set by PALcode.  
      Additionally, the REG field contains the register number of the destination specifier for the instruction that triggered the trap. 

	-- IStream ACV: The BAD_IVA bit of this register indicates whether the offending Istream VA is latched into the EXC_ADDR register or the VA register.

	-- DStream exceptions:  the REG field contains the register number of either 
	   the source specifier (for stores) or the destination specifier (for loads) of the instruction that triggered the trap. 
*/


// ============================================================================
//  EXC_SUM_helpers.h
//  Alpha AXP Emulator - Exception Summary (EXC_SUM) helpers
//
//  This header encapsulates the bit layout and helper functions for the
//  Exception Summary (EXC_SUM) internal processor register.
//
//  Architectural references:
//
//    - Alpha AXP Architecture (Exceptions and Arithmetic Traps)
//      Alpha AXP System Reference Manual, chapter "Exceptions, Interrupts,
//      and Machine Checks" (EXC_SUM field descriptions).
//
//    - Alpha 21164 Microprocessor Hardware Reference Manual
//      Section 5.1.13 "Exception Summary (EXC_SUM) Register".
//      Bits <16:10> record arithmetic trap types:
//        IOV  (integer overflow)
//        INE  (inexact)
//        UNF  (underflow)
//        FOV  (floating overflow)
//        DZE  (divide by zero)
//        INV  (invalid operation)
//        SWC  (software completion possible)
//      All higher bits are RAZ/IGN on reads and writes.
//
//  Emulation notes:
//
//    - For EV4 / EV5 / EV6 style implementations, the arithmetic summary
//      semantics are equivalent for the floating point and integer traps
//      you care about. If a future core variant extends EXC_SUM, you can
//      augment this header with additional masks, leaving the existing
//      bits stable.
//
//    - Hardware semantics are:
//
//        * Any write to EXC_SUM clears bits <16:10>, then loads the new
//          value for those bits from the write data.
//
//        * Arithmetic units set the corresponding bit when a trap condition
//          is recognized. Software can clear bits by writing EXC_SUM.
//
//      The helper excSumApplyWrite implements the write semantics.
// ============================================================================



// ============================================================================
// Bit positions and masks for EXC_SUM (EV6 style)
// ----------------------------------------------------------------------------
// 31:17  RAZ/IGN
// 16     IOV  - Integer overflow
// 15     INE  - Inexact result
// 14     UNF  - Underflow
// 13     FOV  - Floating overflow
// 12     DZE  - Divide by zero
// 11     INV  - Invalid operation
// 10     SWC  - Software completion possible
//  9:0   Reserved (RAZ/IGN in this emulator)
// ============================================================================

static constexpr int EXC_SUM_BIT_SWC = 10;
static constexpr int EXC_SUM_BIT_INV = 11;
static constexpr int EXC_SUM_BIT_DZE = 12;
static constexpr int EXC_SUM_BIT_FOV = 13;
static constexpr int EXC_SUM_BIT_UNF = 14;
static constexpr int EXC_SUM_BIT_INE = 15;
static constexpr int EXC_SUM_BIT_IOV = 16;

static constexpr quint64 EXC_SUM_MASK_SWC =
(quint64(1) << EXC_SUM_BIT_SWC);
static constexpr quint64 EXC_SUM_MASK_INV =
(quint64(1) << EXC_SUM_BIT_INV);
static constexpr quint64 EXC_SUM_MASK_DZE =
(quint64(1) << EXC_SUM_BIT_DZE);
static constexpr quint64 EXC_SUM_MASK_FOV =
(quint64(1) << EXC_SUM_BIT_FOV);
static constexpr quint64 EXC_SUM_MASK_UNF =
(quint64(1) << EXC_SUM_BIT_UNF);
static constexpr quint64 EXC_SUM_MASK_INE =
(quint64(1) << EXC_SUM_BIT_INE);
static constexpr quint64 EXC_SUM_MASK_IOV =
(quint64(1) << EXC_SUM_BIT_IOV);

// All defined arithmetic-trap bits in EXC_SUM.
static constexpr quint64 EXC_SUM_MASK_ARITH =
EXC_SUM_MASK_SWC |
EXC_SUM_MASK_INV |
EXC_SUM_MASK_DZE |
EXC_SUM_MASK_FOV |
EXC_SUM_MASK_UNF |
EXC_SUM_MASK_INE |
EXC_SUM_MASK_IOV;

// ============================================================================
// Raw bit test helpers
// ============================================================================

inline bool excSumHasSWC(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_SWC) != 0;
}

inline bool excSumHasINV(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_INV) != 0;
}

inline bool excSumHasDZE(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_DZE) != 0;
}

inline bool excSumHasFOV(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_FOV) != 0;
}

inline bool excSumHasUNF(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_UNF) != 0;
}

inline bool excSumHasINE(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_INE) != 0;
}

inline bool excSumHasIOV(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_IOV) != 0;
}

// Returns true if any arithmetic-trap bit (IOV, INE, UNF, FOV, DZE, INV, SWC)
// is set in EXC_SUM.
inline bool excSumHasAnyArithmetic(quint64 excSum) noexcept
{
	return (excSum & EXC_SUM_MASK_ARITH) != 0;
}

// ============================================================================
// Bit set / clear helpers
// ============================================================================

inline quint64 excSumSetSWC(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_SWC)
		: (excSum & ~EXC_SUM_MASK_SWC);
}

inline quint64 excSumSetINV(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_INV)
		: (excSum & ~EXC_SUM_MASK_INV);
}

inline quint64 excSumSetDZE(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_DZE)
		: (excSum & ~EXC_SUM_MASK_DZE);
}

inline quint64 excSumSetFOV(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_FOV)
		: (excSum & ~EXC_SUM_MASK_FOV);
}

inline quint64 excSumSetUNF(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_UNF)
		: (excSum & ~EXC_SUM_MASK_UNF);
}

inline quint64 excSumSetINE(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_INE)
		: (excSum & ~EXC_SUM_MASK_INE);
}

inline quint64 excSumSetIOV(quint64 excSum, bool set) noexcept
{
	return set
		? (excSum | EXC_SUM_MASK_IOV)
		: (excSum & ~EXC_SUM_MASK_IOV);
}

// Clear all defined arithmetic-trap bits in EXC_SUM.
inline quint64 excSumClearArithmetic(quint64 excSum) noexcept
{
	return excSum & ~EXC_SUM_MASK_ARITH;
}

// ============================================================================
// Write semantics helper
// ----------------------------------------------------------------------------
// Hardware rule (EV4/EV5/EV6 style):
//
//   Any write to EXC_SUM clears bits <16:10>. The new arithmetic bits are
//   then taken from the write data. Higher bits are RAZ/IGN.
//
// In other words, a write of W results in:
//
//   newEXC_SUM = (oldEXC_SUM & ~EXC_SUM_MASK_ARITH)
//                | (W & EXC_SUM_MASK_ARITH);
//
// For the emulator, this helper centralizes the semantics so your IPR
// descriptor for EXC_SUM simply does:
//
//   iprs.exc_sum = excSumApplyWrite(iprs.exc_sum, newValue);
// ============================================================================

inline quint64 excSumApplyWrite(quint64 current, quint64 writeValue) noexcept
{
	const quint64 preserved = current & ~EXC_SUM_MASK_ARITH;
	const quint64 newArith = writeValue & EXC_SUM_MASK_ARITH;
	return preserved | newArith;
}

// ============================================================================
// Convenience helpers for arithmetic units
// ----------------------------------------------------------------------------
// These helpers are intended to be called from the integer and floating-point
// execution pipelines when an arithmetic trap condition is detected.
// They simply OR in the corresponding EXC_SUM bit.
//
// Note: actual trap enable/disable and FPCR semantics are handled elsewhere;
// this header only records the summary bits.
// ============================================================================

inline void excSumRecordIOV(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_IOV;
}

inline void excSumRecordINE(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_INE;
}

inline void excSumRecordUNF(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_UNF;
}

inline void excSumRecordFOV(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_FOV;
}

inline void excSumRecordDZE(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_DZE;
}

inline void excSumRecordINV(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_INV;
}

inline void excSumRecordSWC(quint64& excSum) noexcept
{
	excSum |= EXC_SUM_MASK_SWC;
}
/*
    setExcSumForException

    Purpose:
        Update the EXC_SUM internal processor register when an exception
        is raised, with particular emphasis on arithmetic traps.

    Architectural background (ASA SRM, "Exceptions, Interrupts, and
    Machine Checks", EXC_SUM field description):

        - EXC_SUM is primarily defined for arithmetic traps and certain
          data-stream / instruction-stream access violations.
        - Bits 16:10 summarize arithmetic conditions:

              SWC  (bit 10)  Software completion possible
              INV  (bit 11)  Invalid operation
              DZE  (bit 12)  Divide by zero
              FOV  (bit 13)  Floating overflow
              UNF  (bit 14)  Underflow
              INE  (bit 15)  Inexact result
              IOV  (bit 16)  Integer overflow

        - Other detailed information (faulting VA, destination register,
          alignment, I-stream vs D-stream, etc.) is carried by EXC_ADDR,
          VA, DAT, and related registers, not by the EXC_SUM low bits.

    Emulator contract:

        - For non-arithmetic exceptions (ITB/DTB faults, access violation,
          breakpoints, etc.), this helper does *not* invent new fields
          in EXC_SUM. Those exceptions are described by EXC_ADDR, VA,
          MM_STAT, DAT, and your higher-level ExceptionClass.

        - For Arithmetic exceptions:
            * extraInfo is interpreted as a bitmask composed of the
              EXC_SUM_MASK_* constants defined above in this header:
                  EXC_SUM_MASK_SWC
                  EXC_SUM_MASK_INV
                  EXC_SUM_MASK_DZE
                  EXC_SUM_MASK_FOV
                  EXC_SUM_MASK_UNF
                  EXC_SUM_MASK_INE
                  EXC_SUM_MASK_IOV
            * The helper clears any previous arithmetic bits and ORs in
              the new set.

        - For a few non-arithmetic ExceptionClass values that can map
          cleanly onto the architectural arithmetic bits (for example,
          IllegalInstruction -> INV, SubsettedInstruction -> SWC), we
          opportunistically set those bits as a convenience. This is
          a modeling choice and can be refined later.

    Parameters:
        exc_sum    Existing EXC_SUM value (typically iprs.exc_sum).
        exClass    High-level ExceptionClass for this trap.
        extraInfo  For Arithmetic: bitmask of EXC_SUM_MASK_* flags
                   describing the specific arithmetic condition.
                   For other classes: currently ignored.

    Returns:
        Updated EXC_SUM value with arithmetic bits set according to
        exClass and extraInfo.
*/

