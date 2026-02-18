#pragma once
#include <QtGlobal>


enum class SoftwareTrapReason : quint16 {
	NONE = 0,
	SYSTEM_CALL,     // CHMK/CHME/CHMS or your syscall vector ///< OS system call (CALL_PAL CALLSYS)
	BREAKPOINT,
	TRACE_TRAP
};



/**
 * @brief IEEE 754 floating point exception reasons.
 *
 * Aligns to FPCR trap enable and status bits.
 * Used with ExceptionCategory::FLOATING_POINT.
 */
enum class FloatingPointReason : quint8 {
	NONE,
	INVALID_OPERATION,                ///< NaN generation, sqrt(-1), inf/inf
	DIVIDE_BY_ZERO,                   ///< Division by zero
	OVER_FLOW,                         ///< Result exceeds format maximum
	UNDER_FLOW,                        ///< Result below normalized minimum
	INEXACT,                          ///< Rounding changed result
	DENORMAL_OPERAND,                 ///< Subnormal operand (if trap enabled)
	UNALIGNED_ACCESS,                 ///< Misaligned FP register access
	RESERVED_OPERAND,                 ///< Signaling NaN or reserved encoding
	FP_DISABLED                       ///< FP instruction with FEN=0
};


/**
 * @brief Integer arithmetic exception reasons.
 *
 * Used with ExceptionCategory::ARITHMETIC.
 */
enum class ArithmeticReason : quint8 {
	NONE,
	INTEGER_OVERFLOW,                 ///< Signed overflow (with /V qualifier)
	INTEGER_DIVIDE_BY_ZERO,           ///< Division by zero
	ILLEGAL_OPCODE,                   ///< Reserved arithmetic opcode
	ILLEGAL_OPERAND,                  ///< Invalid register combination
	INVALID_SHIFT_COUNT,               ///< Shift count out of range
	INEXACT
};

/**
 * @brief Privilege violation sub-reasons.
 *
 * Used with ExceptionCategory::PRIVILEGE_VIOLATION.
 */
enum class PrivilegeViolationReason : quint8 {
	NONE,
	PRIVILEGED_INSTRUCTION,           ///< Kernel instruction in user mode
	PRIVILEGED_REGISTER_ACCESS,       ///< MTPR/MFPR to privileged IPR
	INVALID_IPR,                      ///< Invalid IPR number
	BREAKPOINT,                       ///< Break instruction
	SOFTWARE_INTERRUPT                ///< Explicit software interrupt
};

/**
 * @brief Interrupt sources and reasons.
 *
 * Used with ExceptionCategory::INTERRUPT.
 */
enum class InterruptReason : quint8 {
	NONE,
	HARDWARE,                         ///< External device IRQ
	TIMER,                            ///< Periodic timer expiration
	PERFORMANCE_COUNTER,              ///< Performance counter overflow
	POWER_FAIL,                       ///< Power failure imminent
	CORRECTED_ERROR,                  ///< Correctable error notification
	INTERPROCESSOR,                   ///< Inter-processor interrupt (IPI)
	SYSTEM_MANAGEMENT,                ///< System management interrupt
	HALT_REQUEST,                     ///< External halt signal
	SOFTWARE_IPL_CHANGE,              ///< Software IPL transition
	PASSIVE_RELEASE,                  ///< Device passive release
	CONSOLE_INTERRUPT                 ///< Console/firmware interrupt
};

/**
 * @brief Emulator-specific exception reasons.
 *
 * Used with ExceptionCategory::EMULATOR.
 */
enum class EmulatorReason : quint8 {
	NONE,
	UNSUPPORTED_OPCODE,               ///< Instruction not implemented
	UNSUPPORTED_FEATURE,              ///< Feature not emulated
	PAL_FAULT,                        ///< PALcode fault
	CONTEXT_SWITCH,                   ///< Context transition
	STACK_OVERFLOW,                   ///< Emulator stack overflow
	DOUBLE_FAULT,                     ///< Recursive exception
	RESERVED_OPERAND_FAULT,           ///< Reserved instruction bits used
	ILLEGAL_OPERAND_FAULT,            ///< Reserved AMOVRR/AMOVRM ILLEGAL OPERAND FAULT
	UNHANDLED_ILLEGAL_INSTRUCTION,    ///< Illegal encoding not caught
	MISSING_SAVED_CONTEXT,            ///< Context restoration failed
	INTERNAL_ERROR,                   ///< Emulator internal inconsistency
	UNKNOWN
};

enum class MemoryFaultReason : quint8 {
	PAGE_FAULT_REASON,
	ACCESS_VIOLATION_REASON,
	DOUBLE_MACHINE_CHECK_REASON,
	CORRECTABLE_ERROR_REASON,
	SOFTWARE_TRAP_REASON_REASON,
	DOUBLE_EXCEPTION_ERROR_REASON,
	KERNEL_STACK_NOT_VALID_REASON

};
/**
 * @brief Power management exception reasons.
 *
 * System power and thermal management events.
 */
enum class PowerManagementReason : quint8 {
	NONE,
	SLEEP_MODE_ENTRY,                 ///< Entering sleep/low-power mode
	WAKE_EVENT,                       ///< Wake from sleep
	POWER_FAILURE_IMMINENT,           ///< Power supply failing
	THERMAL_THROTTLE,                 ///< Temperature-based throttling
	CLOCK_STOP_REQUEST,               ///< Clock gating request
	VOLTAGE_CHANGE                    ///< Dynamic voltage scaling
};

/**
 * @brief Performance monitoring exception reasons.
 *
 * Used for performance counter events.
 */
enum class PerformanceReason : quint8 {
	NONE,
	COUNTER_OVERFLOW,                 ///< Performance counter overflow
	SAMPLING_TRIGGER,                 ///< Sampling event triggered
	TRACE_BUFFER_FULL,                ///< Trace buffer full
	BRANCH_PREDICTION_MISS_THRESHOLD, ///< Branch mispredict threshold exceeded
	CACHE_MISS_THRESHOLD              ///< Cache miss threshold exceeded
};