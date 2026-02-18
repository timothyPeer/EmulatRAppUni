#pragma once
#include <emmintrin.h>  // SSE2
#include <pmmintrin.h>  // SSE3
#include <QtGlobal>
#include <cfenv>

// ============================================================================
// Alpha FPCR (Floating Point Control Register) bits
// ============================================================================
namespace AlphaFPCR {
	constexpr quint64 INV = (1ULL << 49);  // Invalid operation
	constexpr quint64 DZE = (1ULL << 50);  // Divide by zero
	constexpr quint64 OVF = (1ULL << 51);  // Overflow
	constexpr quint64 UNF = (1ULL << 52);  // Underflow
	constexpr quint64 INE = (1ULL << 53);  // Inexact
	constexpr quint64 IOV = (1ULL << 54);  // Integer overflow

	// IEEE FP compare flags: bits 21-24 in FPCR
	static constexpr quint32 FPCC_LT_BIT = (1u << 21); // Less Than
	static constexpr quint32 FPCC_EQ_BIT = (1u << 22); // Equal
	static constexpr quint32 FPCC_GT_BIT = (1u << 23); // Greater Than
	static constexpr quint32 FPCC_UN_BIT = (1u << 24); // Unordered (e.g., NaN)

	// All bits that correspond to exception conditions (FP + integer overflow)
	constexpr quint64 EXC_MASK = INV | DZE | OVF | UNF | INE | IOV;

	// Dynamic rounding mode fields
	constexpr quint64 DYN_RM_SHIFT = 58;
	constexpr quint64 DYN_RM_MASK = 0x0C00000000000000ULL;  // Bits 58-59

	// Rounding modes
	constexpr quint64 RM_CHOPPED = 0;    // Round toward zero
	constexpr quint64 RM_MINUS_INF = 1;  // Round toward -infinity
	constexpr quint64 RM_NORMAL = 2;     // Round to nearest (ties to even)
	constexpr quint64 RM_PLUS_INF = 3;   // Round toward +infinity (or dynamic)
	constexpr quint64 ROUNDING_MASK = RM_CHOPPED | RM_MINUS_INF | RM_NORMAL | RM_PLUS_INF; // TODO verify this

	// IEEE trap enable bits
	constexpr quint64 TRAP_ENABLE_INV = 0x0000020000000000ULL;  // Bit 49: Invalid operation
	constexpr quint64 TRAP_ENABLE_DZE = 0x0000040000000000ULL;  // Bit 50: Divide by zero
	constexpr quint64 TRAP_ENABLE_OVF = 0x0000080000000000ULL;  // Bit 51: Overflow
	constexpr quint64 TRAP_ENABLE_UNF = 0x0000100000000000ULL;  // Bit 52: Underflow
	constexpr quint64 TRAP_ENABLE_INE = 0x0000200000000000ULL;  // Bit 53: Inexact
	constexpr quint64 TRAP_ENABLE_MASK = TRAP_ENABLE_INV | TRAP_ENABLE_DZE | TRAP_ENABLE_OVF | TRAP_ENABLE_UNF | TRAP_ENABLE_INE;
	constexpr quint64 TRAP_ENABLE_SHIFT = TRAP_ENABLE_INV | TRAP_ENABLE_DZE | TRAP_ENABLE_OVF | TRAP_ENABLE_UNF | TRAP_ENABLE_INE; // TODO Verify This
}
// ============================================================================
// Operation Status Flags
// ============================================================================
struct ArithmeticStatus {
	bool invalid;
	bool divByZero;
	bool overflow;
	bool underflow;
	bool inexact;
	bool intOverflow;

	ArithmeticStatus()
		: invalid(false), divByZero(false), overflow(false),
		underflow(false), inexact(false), intOverflow(false) {
	}

	// Apply status to FPCR register
	void applyToFPCR(quint64& fpcr) const noexcept {
		if (invalid)     fpcr |= AlphaFPCR::INV;
		if (divByZero)   fpcr |= AlphaFPCR::DZE;
		if (overflow)    fpcr |= AlphaFPCR::OVF;
		if (underflow)   fpcr |= AlphaFPCR::UNF;
		if (inexact)     fpcr |= AlphaFPCR::INE;
		if (intOverflow) fpcr |= AlphaFPCR::IOV;
	}

	bool hasException() const noexcept {
		return invalid || divByZero || overflow || underflow;
	}


};

// ============================================================================
// SSE Helper Library with FPCR Integration
// ============================================================================
namespace AlphaSSE {

	struct Config {
		static bool useSSE2;
		static bool useSSE3;
		static bool forceFallback;
	};

	// ============================================================================
	// Helper: Check floating point exceptions
	// ============================================================================
	inline void checkFloatingPointExceptions(ArithmeticStatus& status) noexcept
	{
		int exceptions = std::fetestexcept(FE_ALL_EXCEPT);

		if (exceptions & FE_INVALID)   status.invalid = true;
		if (exceptions & FE_DIVBYZERO) status.divByZero = true;
		if (exceptions & FE_OVERFLOW)  status.overflow = true;
		if (exceptions & FE_UNDERFLOW) status.underflow = true;
		if (exceptions & FE_INEXACT)   status.inexact = true;

		std::feclearexcept(FE_ALL_EXCEPT);
	}
};
