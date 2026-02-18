#ifndef _EMULATRAPPUNI_CORELIB_ICCSR_BITS_INL_H
#define _EMULATRAPPUNI_CORELIB_ICCSR_BITS_INL_H

#pragma once

#include <QtGlobal>

// ============================================================================
// ICCSR (Instruction Cache Control and Status Register) Bit Definitions
// ============================================================================
// Reference: Alpha 21264/EV67 Hardware Reference Manual, Section 5.1.4
// The ICCSR register controls various Ibox functions including floating-point
// enable, hardware enable, branch prediction, and performance counter selection.
// ============================================================================

namespace ICCSR_Bits {

	// ========================================================================
	// CONTROL BITS (Bits 0-7)
	// ========================================================================

	/// \brief Floating-Point Enable (FPE) - Bit 0
	/// When clear, all floating-point instructions generate FEN exceptions.
	/// When set, floating-point instructions execute normally.
	/// This is the only architecturally defined bit in ICCSR.
	constexpr unsigned FPE_BIT = 0;
	constexpr quint64  FPE_MASK = (1ULL << FPE_BIT);

	/// \brief Hardware Enable (HWE) - Bit 1
	/// When set, allows special privileged PALcode instructions (HW_MFPR/HW_MTPR)
	/// to execute in kernel mode outside of PALmode.
	/// Intended for diagnostic or OS alternative PALcode routines only.
	/// WARNING: Should remain 0 in production systems for security.
	constexpr unsigned HWE_BIT = 1;
	constexpr quint64  HWE_MASK = (1ULL << HWE_BIT);

	/// \brief Branch Prediction Enable (BPE) - Bit 2
	/// When set, enables branch prediction.
	/// When clear, branch prediction is disabled (useful for deterministic debugging).
	constexpr unsigned BPE_BIT = 2;
	constexpr quint64  BPE_MASK = (1ULL << BPE_BIT);

	/// \brief Branch History Enable (BHE) - Bit 3
	/// When set, enables branch history tables.
	/// When clear, branch history is disabled.
	constexpr unsigned BHE_BIT = 3;
	constexpr quint64  BHE_MASK = (1ULL << BHE_BIT);

	/// \brief Jump Stack Enable (JSE) - Bit 4
	/// When set, enables the jump prediction stack.
	/// When clear, jump prediction is disabled.
	constexpr unsigned JSE_BIT = 4;
	constexpr quint64  JSE_MASK = (1ULL << JSE_BIT);

	/// \brief Single-Issue Disable Enable (SDE) - Bit 5
	/// When set, allows single-issue mode for debugging.
	/// When clear, normal multi-issue operation.
	constexpr unsigned SDE_BIT = 5;
	constexpr quint64  SDE_MASK = (1ULL << SDE_BIT);

	/// \brief Serial Line Enable (SLE) - Bit 6
	/// When set, enables serial line diagnostics.
	/// When clear, serial line is disabled.
	constexpr unsigned SLE_BIT = 6;
	constexpr quint64  SLE_MASK = (1ULL << SLE_BIT);

	/// \brief Memory Address Prediction (MAP) - Bit 7
	/// When set, enables memory address prediction.
	/// When clear, memory address prediction is disabled.
	constexpr unsigned MAP_BIT = 7;
	constexpr quint64  MAP_MASK = (1ULL << MAP_BIT);

	// ========================================================================
	// RESERVED BITS (Bits 8-43)
	// ========================================================================
	// Implementation-specific or reserved for future use
	// Should be written as zero and ignored on read

	// ========================================================================
	// PERFORMANCE COUNTER SELECTION (Bits 44-47)
	// ========================================================================

	/// \brief Performance Counter 0 Select (PC0) - Bits 45:44
	/// Selects the event counted by performance counter 0.
	/// Values:
	///   00 = Aggregate counting mode
	///   01 = ProfileMe mode  
	///   10 = Reserved
	///   11 = Reserved
	constexpr unsigned PC0_SHIFT = 44;
	constexpr quint64  PC0_MASK = (0x3ULL << PC0_SHIFT);

	/// \brief Performance Counter 1 Select (PC1) - Bits 47:46
	/// Selects the event counted by performance counter 1.
	/// Values:
	///   00 = Aggregate counting mode
	///   01 = ProfileMe mode
	///   10 = Reserved
	///   11 = Reserved
	constexpr unsigned PC1_SHIFT = 46;
	constexpr quint64  PC1_MASK = (0x3ULL << PC1_SHIFT);

	// ========================================================================
	// STATUS/ERROR BITS (Bits 48-63)
	// ========================================================================
	// These are typically read-only status bits
	// Exact layout is implementation-specific

	/// \brief Icache Parity Error (ICPERR) - Bit 48 (example)
	/// Set when instruction cache parity error detected.
	/// Implementation-specific - may vary by CPU revision.
	constexpr unsigned ICPERR_BIT = 48;
	constexpr quint64  ICPERR_MASK = (1ULL << ICPERR_BIT);

	// Additional status bits (implementation-specific, consult manual)
	// Bits 49-63: Reserved or implementation-specific status

	// ========================================================================
	// HELPER FUNCTIONS
	// ========================================================================

	/// \brief Extract FPE bit from ICCSR value
	inline bool getFPE(quint64 iccsr) noexcept {
		return (iccsr & FPE_MASK) != 0;
	}

	/// \brief Set FPE bit in ICCSR value
	inline quint64 setFPE(quint64 iccsr, bool enable) noexcept {
		if (enable)
			return iccsr | FPE_MASK;
		else
			return iccsr & ~FPE_MASK;
	}

	/// \brief Extract HWE bit from ICCSR value
	inline bool getHWE(quint64 iccsr) noexcept {
		return (iccsr & HWE_MASK) != 0;
	}

	/// \brief Set HWE bit in ICCSR value
	inline quint64 setHWE(quint64 iccsr, bool enable) noexcept {
		if (enable)
			return iccsr | HWE_MASK;
		else
			return iccsr & ~HWE_MASK;
	}

	/// \brief Extract BPE bit from ICCSR value
	inline bool getBPE(quint64 iccsr) noexcept {
		return (iccsr & BPE_MASK) != 0;
	}

	/// \brief Set BPE bit in ICCSR value
	inline quint64 setBPE(quint64 iccsr, bool enable) noexcept {
		if (enable)
			return iccsr | BPE_MASK;
		else
			return iccsr & ~BPE_MASK;
	}

	/// \brief Extract BHE bit from ICCSR value
	inline bool getBHE(quint64 iccsr) noexcept {
		return (iccsr & BHE_MASK) != 0;
	}

	/// \brief Set BHE bit in ICCSR value
	inline quint64 setBHE(quint64 iccsr, bool enable) noexcept {
		if (enable)
			return iccsr | BHE_MASK;
		else
			return iccsr & ~BHE_MASK;
	}

	/// \brief Extract PC0 field from ICCSR value
	inline quint8 getPC0(quint64 iccsr) noexcept {
		return static_cast<quint8>((iccsr >> PC0_SHIFT) & 0x3);
	}

	/// \brief Set PC0 field in ICCSR value
	inline quint64 setPC0(quint64 iccsr, quint8 value) noexcept {
		return (iccsr & ~PC0_MASK) | ((quint64(value) & 0x3) << PC0_SHIFT);
	}

	/// \brief Extract PC1 field from ICCSR value
	inline quint8 getPC1(quint64 iccsr) noexcept {
		return static_cast<quint8>((iccsr >> PC1_SHIFT) & 0x3);
	}

	/// \brief Set PC1 field in ICCSR value
	inline quint64 setPC1(quint64 iccsr, quint8 value) noexcept {
		return (iccsr & ~PC1_MASK) | ((quint64(value) & 0x3) << PC1_SHIFT);
	}

	/// \brief Extract Icache parity error status
	inline bool getICPERR(quint64 iccsr) noexcept {
		return (iccsr & ICPERR_MASK) != 0;
	}

	// ========================================================================
	// POWER-ON RESET VALUE
	// ========================================================================

	/// \brief Default ICCSR value after hardware reset
	/// FPE is set by hardware on reset (per Alpha 21264 manual)
	/// All other bits are cleared
	constexpr quint64 RESET_VALUE = FPE_MASK;

	// ========================================================================
	// BIT FIELD DESCRIPTIONS (for documentation/logging)
	// ========================================================================

	inline const char* getBitName(unsigned bitIndex) {
		switch (bitIndex) {
		case FPE_BIT:    return "FPE (Floating-Point Enable)";
		case HWE_BIT:    return "HWE (Hardware Enable)";
		case BPE_BIT:    return "BPE (Branch Prediction Enable)";
		case BHE_BIT:    return "BHE (Branch History Enable)";
		case JSE_BIT:    return "JSE (Jump Stack Enable)";
		case SDE_BIT:    return "SDE (Single-Issue Disable Enable)";
		case SLE_BIT:    return "SLE (Serial Line Enable)";
		case MAP_BIT:    return "MAP (Memory Address Prediction)";
		case ICPERR_BIT: return "ICPERR (Icache Parity Error)";
		default:         return "Reserved";
		}
	}

} // namespace ICCSR_Bits
#endif
