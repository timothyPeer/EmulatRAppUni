#pragma once
#ifndef CORELIB_CORE_H
#define CORELIB_CORE_H

#include <QtGlobal>

// ============================================================================
// Alpha AXP SRM v6.0 (1994) – ASTEN/ASTSR/ASTER bit semantics
//   - ASTEN<3:0> : enable AST delivery per mode (K/E/S/U)
//   - ASTSR<3:0> : AST pending summary per mode (K/E/S/U)
//   - ASTER<3:0> : implementation often mirrors the same 4-mode layout
//
// Source:
//   Alpha AXP System Reference Manual, Version 6.0 (1994)
//   Section 6.7.6.2 (ASTEN) and 6.7.6.3 (ASTSR), printed page 6-37.
// ============================================================================
//
// NOTE on ASTEN write semantics (from SRM 6.7.6.2):
//   - MTPR ASTEN: R16<3:0> sets bits; R16<7:4> clears bits.
//   - This means software can set and clear in one write.
// ============================================================================

namespace ASTBits4
{
	// Mode bit positions within <3:0>
	static constexpr quint8 ASTK_BIT = 0; // Kernel
	static constexpr quint8 ASTE_BIT = 1; // Executive
	static constexpr quint8 ASTS_BIT = 2; // Supervisor
	static constexpr quint8 ASTU_BIT = 3; // User

	static constexpr quint64 ASTK_MASK = (1ULL << ASTK_BIT);
	static constexpr quint64 ASTE_MASK = (1ULL << ASTE_BIT);
	static constexpr quint64 ASTS_MASK = (1ULL << ASTS_BIT);
	static constexpr quint64 ASTU_MASK = (1ULL << ASTU_BIT);

	static constexpr quint64 AST_ALL_MASK = (ASTK_MASK | ASTE_MASK | ASTS_MASK | ASTU_MASK);

	// ASTEN write semantics masks (R16)
	static constexpr quint64 WRITE_SET_MASK = 0x000000000000000FULL; // R16<3:0> sets corresponding ASTEN bits
	static constexpr quint64 WRITE_CLEAR_MASK = 0x00000000000000F0ULL; // R16<7:4> clears corresponding ASTEN bits
	static constexpr quint8  WRITE_CLEAR_SHIFT = 4;

	// Helpers
	static AXP_HOT AXP_ALWAYS_INLINE constexpr bool test(quint64 reg, quint64 mask) noexcept
	{
		return (reg & mask) != 0;
	}

	static AXP_HOT AXP_ALWAYS_INLINE constexpr void set(quint64& reg, quint64 mask, bool enable) noexcept
	{
		reg = enable ? (reg | mask) : (reg & ~mask);
	}

	// Apply SRM-defined ASTEN write semantics:
	//   newASTEN = (oldASTEN | (R16<3:0>)) & ~(R16<7:4> >> 4)
	static AXP_HOT AXP_ALWAYS_INLINE constexpr quint64 applyAstenWrite(quint64 oldAsten, quint64 r16) noexcept
	{
		const quint64 setBits = (r16 & WRITE_SET_MASK);
		const quint64 clearBits = ((r16 & WRITE_CLEAR_MASK) >> WRITE_CLEAR_SHIFT) & WRITE_SET_MASK;
		return (oldAsten | setBits) & ~clearBits;
	}
}

// Keep your original namespace names, but corrected to <3:0> layout.
namespace ASTSR
{
	static constexpr quint64 ASTK_MASK = ASTBits4::ASTK_MASK;
	static constexpr quint64 ASTE_MASK = ASTBits4::ASTE_MASK;
	static constexpr quint64 ASTS_MASK = ASTBits4::ASTS_MASK;
	static constexpr quint64 ASTU_MASK = ASTBits4::ASTU_MASK;
	static constexpr quint64 AST_ALL_MASK = ASTBits4::AST_ALL_MASK;
}

namespace ASTER
{
	static constexpr quint64 ASTK_MASK = ASTBits4::ASTK_MASK;
	static constexpr quint64 ASTE_MASK = ASTBits4::ASTE_MASK;
	static constexpr quint64 ASTS_MASK = ASTBits4::ASTS_MASK;
	static constexpr quint64 ASTU_MASK = ASTBits4::ASTU_MASK;
	static constexpr quint64 AST_ALL_MASK = ASTBits4::AST_ALL_MASK;
}

namespace ASTEN
{
	static constexpr quint64 ASTK_MASK = ASTBits4::ASTK_MASK;
	static constexpr quint64 ASTE_MASK = ASTBits4::ASTE_MASK;
	static constexpr quint64 ASTS_MASK = ASTBits4::ASTS_MASK;
	static constexpr quint64 ASTU_MASK = ASTBits4::ASTU_MASK;
	static constexpr quint64 AST_ALL_MASK = ASTBits4::AST_ALL_MASK;

	static constexpr quint64 WRITE_SET_MASK = ASTBits4::WRITE_SET_MASK;
	static constexpr quint64 WRITE_CLEAR_MASK = ASTBits4::WRITE_CLEAR_MASK;
	static constexpr quint8  WRITE_CLEAR_SHIFT = ASTBits4::WRITE_CLEAR_SHIFT;

	static AXP_HOT AXP_ALWAYS_INLINE constexpr quint64 applyWrite(quint64 oldAsten, quint64 r16) noexcept
	{
		return ASTBits4::applyAstenWrite(oldAsten, r16);
	}
}

// ============================================================================
// ISUM Bit Masks
// ----------------------------------------------------------------------------
// Your masks are correct *for the bit positions stated below*:
//   - EI bits 38:33 (6 bits)
//   - SL bit 32
//   - CR bit 31
//   - PC bits 30:29
//   - SI bits 28:14 (15 bits)
//
// NOTE: “ISUM” naming/fields can be implementation-specific (EV6/21264 style).
// If you have an EV6 IPR spec page for ISUM, align to that SSOT.
// ============================================================================

namespace ISUM
{
	// External Interrupt Summary (bits 38:33) – 6 bits
	static constexpr quint64 EI_SHIFT = 33;
	static constexpr quint64 EI_MASK = ((quint64(1) << 6) - 1ULL) << EI_SHIFT; // 0x0000007E00000000

	// System Level Interrupt (bit 32)
	static constexpr quint64 SL_SHIFT = 32;
	static constexpr quint64 SL_MASK = (1ULL << SL_SHIFT);                     // 0x0000000100000000

	// Corrected Read Error (bit 31)
	static constexpr quint64 CR_SHIFT = 31;
	static constexpr quint64 CR_MASK = (1ULL << CR_SHIFT);                     // 0x0000000080000000

	// Performance Counter Summary (bits 30:29)
	static constexpr quint64 PC_SHIFT = 29;
	static constexpr quint64 PC_MASK = (3ULL << PC_SHIFT);                     // 0x0000000060000000
	static constexpr quint64 PC0_MASK = (1ULL << 29);                           // Bit 29
	static constexpr quint64 PC1_MASK = (1ULL << 30);                           // Bit 30

	// Software Interrupt Summary (bits 28:14) – 15 bits
	static constexpr quint64 SI_SHIFT = 14;
	static constexpr quint64 SI_MASK = ((quint64(1) << 15) - 1ULL) << SI_SHIFT; // 0x000000001FFFC000

	// Handy composed mask (does NOT include AST*; those are separate regs)
	static constexpr quint64 VALID_MASK = (EI_MASK | SL_MASK | CR_MASK | PC_MASK | SI_MASK);

	// Helpers
	static AXP_HOT AXP_ALWAYS_INLINE constexpr quint64 extractField(quint64 reg, quint64 mask, quint8 shift) noexcept
	{
		return (reg & mask) >> shift;
	}

	static AXP_HOT AXP_ALWAYS_INLINE constexpr bool test(quint64 reg, quint64 mask) noexcept
	{
		return (reg & mask) != 0;
	}
}





#endif // CORELIB_CORE_H
