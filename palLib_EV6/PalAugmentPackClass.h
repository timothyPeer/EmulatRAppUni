#ifndef PALAUGMENTPACKCLASS_H
#define PALAUGMENTPACKCLASS_H

#include <QtGlobal>
#include "../faultLib/PendingEvent_Refined.h"
#include "../grainFactoryLib/PipeLineSlot.h"
#include "PalVectorId_refined.h"
#include "Global_PALVectorTable.h"
#include "PalArgumentPack_str.h"

class PALAugmentPackClass {

public: 

    static  AXP_FLATTEN PalArgumentPack buildAugmentPack(PipelineSlot slot, PendingEvent palEvent) noexcept {
        Q_UNUSED(slot)
        PalVectorEntry* entry = globalPALVectorTable().lookup(palEvent.palVectorId);

    }

	/**
 * @brief Build PAL arguments - unified dispatcher
 *
 * Routes to type-specific builders based on exception class.
 */
	AXP_HOT	AXP_FLATTEN PalArgumentPack PalbuildPalArgumentPack(PipelineSlot slot, PendingEvent& ev) noexcept
	{
		switch (ev.exceptionClass) {
			// ========================================================================
			// Memory Faults
			// ========================================================================
		case ExceptionClass::DTB_MISS_SINGLE:
		case ExceptionClass::DTB_MISS_DOUBLE:
		case ExceptionClass::ITB_MISS:
		case ExceptionClass::DFAULT:
		case ExceptionClass::ITB_ACV:
			return buildMemoryFaultArgs(ev);

			// ========================================================================
			// Hardware Exceptions
			// ========================================================================
		case ExceptionClass::AST:
			return buildASTArgs(ev);

		case ExceptionClass::ARITH:
			return buildArithmeticArgs(ev);

		case ExceptionClass::INTERRUPT:
			return buildInterruptArgs(ev);

		case ExceptionClass::MCHK:
			return buildMachineCheckArgs(ev);

		case ExceptionClass::OPCDEC:
			return buildIllegalInstructionArgs(ev);

			//case ExceptionClass::FP_DISABLED:
		case ExceptionClass::FEN:
			return buildFPDisabledArgs(ev);

		case ExceptionClass::UNALIGN:
			return buildUnalignedArgs(ev);

			// ========================================================================
			// Generic CALL_PAL
			// ========================================================================
		case ExceptionClass::CALL_PAL:
			return buildCallPalArgs(ev);

			// ========================================================================
			// Default/Unknown
			// ========================================================================
		default:
			WARN_LOG(QString("buildPalArgumentPack: Unknown exception class %1")
				.arg(static_cast<int>(ev.exceptionClass)));

			// Generic fallback - pass what we have
			PalArgumentPack args;
			args.a0 = ev.extraInfo;
			args.a1 = ev.faultPC;
			args.a2 = ev.faultVA;
			return args;
		}
	}
private:


#pragma region PAL Argument Builders
		// ======================================================================
		// Build PAL Argument Pack for synchronous traps
		// ======================================================================

		/**
		 * @brief Build PAL arguments for memory fault
		 */
        inline PalArgumentPack buildMemoryFaultArgs( PendingEvent& ev)  noexcept
		{
			auto& palTable = globalPALVectorTable();
			PalVectorEntry* entry = palTable.lookup(ev.palVectorId);
			PalArgumentPack args;
			args.a0 = ev.faultVA;                           // R16: Fault VA
			args.a1 = ev.asn;                               // R17: ASN
			args.a2 = static_cast<quint64>(ev.pendingEvent_Info.faultType);  // R18: Fault type
			args.a3 = ev.pendingEvent_Info.isWrite ? 1 : 0; // R19: Write flag
			args.a4 = ev.faultPC;                     // R20: Faulting PC
			args.PalOffset = entry;
			return args;
		}

		/**
		 * @brief Build PAL arguments for AST
		 */
        AXP_FLATTEN PalArgumentPack buildASTArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.astsr;           // R16: Which AST bits are pending
			args.a1 = ev.faultPC;		  // R17: Where we were interrupted
			args.a2 = 0;                  // R18: Reserved

			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for arithmetic exception
		 */
        AXP_FLATTEN PalArgumentPack buildArithmeticArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.extraInfo;       // R16: EXC_SUM (exception summary)
			args.a1 = ev.faultPC;         // R17: Faulting PC
			args.a2 = 0;                  // R18: Reserved
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for interrupt
		 */
        AXP_FLATTEN PalArgumentPack buildInterruptArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.hwVector;        // R16: Device vector
			args.a1 = ev.hwIPL;           // R17: Interrupt IPL
			args.a2 = ev.faultPC;   // R18: Where interrupted
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for machine check
		 */
        AXP_FLATTEN PalArgumentPack buildMachineCheckArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.extraInfo;                    // R16: MCHK syndrome
			args.a1 = static_cast<quint64>(ev.mcReason);  // R17: MCHK reason
			args.a2 = ev.faultVA;                      // R18: Fault address (if applicable)
			args.a3 = ev.faultPC;                // R19: Where MCHK occurred
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for CALL_PAL
		 */
        AXP_FLATTEN PalArgumentPack buildCallPalArgs( PendingEvent& ev)  noexcept
		{
			// CALL_PAL doesn't need special args - uses R16-R20 from user code
			PalArgumentPack args;
			args.a0 = ev.extraInfo;  // Function code (if needed)
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);

			return args;
		}



		/**
		 * @brief Build PAL arguments for breakpoint (BPT)
		 *
		 * BPT is used by debuggers to set breakpoints.
		 * Arguments are minimal - just the PC where breakpoint occurred.
		 */
        AXP_FLATTEN PalArgumentPack buildBreakpointArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.faultPC;         // R16: PC where BPT executed
			args.a1 = 0;                  // R17: Reserved (could be breakpoint ID)
			args.a2 = 0;                  // R18: Reserved
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for software trap (GENTRAP)
		 *
		 * GENTRAP is used for software-generated exceptions:
		 * - Integer overflow (trap code -1)
		 * - Division by zero (trap code -2)
		 * - Assert failures (trap code in extraInfo)
		 * - Range violations
		 * - Invalid operations
		 */
        AXP_FLATTEN PalArgumentPack buildSoftwareTrapArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.extraInfo;       // R16: Trap code (from GENTRAP instruction)
			args.a1 = ev.faultPC;         // R17: PC where trap occurred
			args.a2 = 0;                  // R18: Additional trap info (optional)
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for system call (CALLSYS)
		 *
		 * CALLSYS is the system call entry point.
		 * Arguments are already in R0, R16-R21 from user code.
		 * PAL just needs to know where the call came from.
		 */
        AXP_FLATTEN PalArgumentPack buildSystemCallArgs(PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			// R16-R21 already contain user's syscall arguments
			// Just pass the call site PC for continuity
			args.a0 = ev.faultPC;         // R16: PC of CALLSYS instruction
			// Note: In real implementation, syscall number is in R0
			// and syscall args are in R16-R21 (already set by user)
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for illegal instruction (OPCDEC)
		 */
        AXP_FLATTEN PalArgumentPack buildIllegalInstructionArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.faultPC;         // R16: PC of illegal instruction
			args.a1 = ev.extraInfo;       // R17: Instruction opcode (if available)
			args.a2 = 0;                  // R18: Reserved
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for floating-point disabled (FEN)
		 */
        AXP_FLATTEN PalArgumentPack buildFPDisabledArgs(PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.faultPC;         // R16: PC of FP instruction
			args.a1 = ev.extraInfo;       // R17: FP instruction opcode
			args.a2 = 0;                  // R18: Reserved
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}

		/**
		 * @brief Build PAL arguments for unaligned access
		 */
        AXP_FLATTEN PalArgumentPack buildUnalignedArgs( PendingEvent& ev)  noexcept
		{
			PalArgumentPack args;
			args.a0 = ev.faultVA;         // R16: Unaligned address
			args.a1 = ev.faultPC;         // R17: PC of unaligned access
			args.a2 = ev.pendingEvent_Info.isWrite ? 1 : 0;  // R18: Read/Write flag
			args.a3 = ev.extraInfo;       // R19: Access size (1/2/4/8 bytes)
			args.PalOffset = globalPALVectorTable().lookup(ev.palVectorId);
			return args;
		}
#pragma endregion PAL Argument Builders

};

#endif // PALAUGMENTPACKCLASS_H
