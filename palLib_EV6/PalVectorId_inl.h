#ifndef PALVECTORID_INL_H
#define PALVECTORID_INL_H

#include <QtGlobal>
#include "coreLib/types_core.h"

// ============================================================================
// Helper: Check if in Kernel Mode
// ============================================================================

/**
 * @brief Checks if processor is currently in kernel mode
 *
 * @param cpuId
 * @param ctx Processor context
 * @return true if IER_CM[CM] == 0 (kernel mode)
 */
// AXP_HOT AXP_ALWAYS_INLINE  bool isKernelMode(const CPUIdType cpuId) noexcept
// {
// 	const quint64 currentMode =  globalgetCM_Active(cpuId);
// 	return (currentMode == 0);
// }

// ============================================================================
// CALL_PAL Function Field Categories
// ============================================================================

namespace CallPalCategory {
	/**
	 * @brief Categorizes CALL_PAL function field
	 *
	 * @param func Function field [7:0]
	 * @return Category: 0=privileged, 1=unprivileged, 2=illegal
	 */

	AXP_HOT AXP_HOT AXP_ALWAYS_INLINE   int categorize(const quint8 func) noexcept
	{
		if (func <= 0x3F) {
			return 0;  // Privileged (0x00-0x3F)
		}
		else if (func >= 0x80 && func <= 0xBF) {
			return 1;  // Unprivileged (0x80-0xBF)
		}
		else {
			return 2;  // Illegal (0x40-0x7F or > 0xBF)
		}
	}

	/**
	 * @brief Gets the base offset for CALL_PAL category
	 *
	 * Per EV6 spec:
	 * - Privileged: offset 0x2000
	 * - Unprivileged: offset 0x3000
	 *
	 * @param func Function field [7:0]
	 * @return Base offset from PAL_BASE
	 */
	AXP_HOT AXP_HOT AXP_ALWAYS_INLINE   quint64 getBaseOffset(const quint8 func) noexcept
	{
		if (func <= 0x3F) {
			return 0x2000;  // Privileged base
		}
		else {
			return 0x3000;  // Unprivileged base
		}
	}
}

// ============================================================================
// Example Usage in CALL_PAL Grain
// ============================================================================

#if 0
// In CallPal_InstructionGrain.h execute() method:

void execute(const DecodedInstruction& di, CPUIdType cpuId) const override
{
	const quint8 func = di.pal_func;  // Function field from instruction

	// Save return address to PALshadow R23
	const quint64 returnPC = getPC_Active(cpuId) + 4;  // PC of next instruction
	const bool wasInPalMode = ctx.isInPalMode();

	// Bit [0] of return address indicates if we were already in PALmode
	const quint64 linkage = returnPC | (wasInPalMode ? 0x1ULL : 0x0ULL);
	ctx.writePalShadowReg(23, linkage);  // R23 = linkage register

	// Compute target PC (may be exception vector if illegal)
	const quint64 targetPC = computeCallPalEntry(&ctx, func);

	// Update return prediction stack
	ctx.pushReturnPrediction(returnPC);

	// Jump to PALcode entry point
	ctx.setPC(targetPC);
	ctx.enterPalMode();  // Ensure PALmode set (bit [0] of PC = 1)
}
#endif

// ============================================================================
// Exception Vector Table (for reference)
// ============================================================================

/**
 * Standard Alpha PAL Exception Vectors (EV6)
 *
 * Offset   Name        Description
 * ------   ----        -----------
 * 0x0000   RESET       System reset
 * 0x0080   MCHK        Machine check (hardware error)
 * 0x0100   ARITH       Arithmetic exception (overflow, etc.)
 * 0x0180   INTERRUPT   External interrupt
 * 0x0200   DTBMISS     Data TLB miss (first level)
 * 0x0280   ITBMISS     Instruction TLB miss (first level)
 * 0x0300   UNALIGN     Unaligned data access
 * 0x0380   OPCDEC      Illegal opcode/operand
 * 0x0400   FEN         FP disabled (FP instruction while FEN=0)
 * 0x0480   DTBFAULT    Data TLB fault (double miss)
 * 0x0500   DTBACV      Data TLB access violation
 * 0x0580   ITBACV      Instruction TLB access violation
 *
 * All vectors calculated as: PAL_BASE[63:15] | vector_offset | PALmode_bit
 */


#endif // PALVECTORID_INL_H
