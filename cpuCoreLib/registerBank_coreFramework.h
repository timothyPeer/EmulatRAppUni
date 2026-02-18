// ============================================================================
// RegisterBankFramework.H
// ----------------------------------------------------------------------------
// Per-CPU integer and floating-point register banks for Alpha AXP emulation.
// Intended ownership: AlphaCPU (one instance per CPU).
//
// References:
//   - Alpha Architecture Reference Manual (AARM), integer register set R0-R31.
//   - Alpha AXP System Reference Manual (SRM), floating-point register set F0-F31.
//   - Calling standard: R26 = return address, R29 = global pointer.
// ============================================================================

#pragma once

#include <QtGlobal>
#include "../memoryLib/global_GuestMemory.h"

// Optional: enable/disable bounds checking in debug builds.
#ifndef REGISTERBANK_ENABLE_BOUNDS_CHECK
#define REGISTERBANK_ENABLE_BOUNDS_CHECK 1
#endif

// ============================================================================
// Integer Register Bank (R0-R31)
// ============================================================================
//
// This class models the 32 architectural integer registers of a single Alpha CPU.
// Ownership:
//   - One instance per AlphaCPU.
//   - NOT a global singleton, NOT shared between CPUs.
// Access:
//   - Access by index (0..31).
//   - Optional: enum-based names for clarity.
//
// Notes:
//   - R31 is hard-wired to zero in hardware; in the emulator we typically
//     store a value and enforce the zero semantics at read time for R31.
//   - You can enforce the "R31 = 0" rule either here or in AlphaCPU.
// ============================================================================

class RegisterBankInteger
{
public:
	static constexpr int RegisterCount = 32;

	// Named indices for readability, optional.
	enum RegIndex : int
	{
		R0 = 0, R1 = 1, R2 = 2, R3 = 3,
		R4 = 4, R5 = 5, R6 = 6, R7 = 7,
		R8 = 8, R9 = 9, R10 = 10, R11 = 11,
		R12 = 12, R13 = 13, R14 = 14, R15 = 15,
		R16 = 16, R17 = 17, R18 = 18, R19 = 19,
		R20 = 20, R21 = 21, R22 = 22, R23 = 23,
		R24 = 24, R25 = 25, R26 = 26, R27 = 27,
		R28 = 28, R29 = 29, R30 = 30, R31 = 31, NONE = -9
	};

	RegisterBankInteger() noexcept
	{
		reset();
	}

	// Reset all registers to 0.
	inline void reset() noexcept
	{
		for (int i = 0; i < RegisterCount; ++i)
		{
			m_regs[i] = 0;
		}
	}

	// Raw indexed accessor (0..31).
	inline quint64 read(int index) const noexcept
	{
#if REGISTERBANK_ENABLE_BOUNDS_CHECK
		if (index < 0 || index >= RegisterCount)
		{
			// In release builds this just returns 0; you can replace with
			// Q_ASSERT or std::abort as desired.
			return 0;
		}
#endif
		// If you want strict Alpha semantics, enforce R31 = 0 here:
		// if (index == R31) return 0;
		return m_regs[index];
	}

	inline void write(int index, quint64 value) noexcept
	{
#if REGISTERBANK_ENABLE_BOUNDS_CHECK
		if (index < 0 || index >= RegisterCount)
		{
			return;
		}
#endif
		// If enforcing R31 as zero register, ignore writes:
		// if (index == R31) return;
		m_regs[index] = value;
	}

	// Typed accessors for named registers, for readability in PAL helpers.
	inline quint64 getR(RegIndex r) const noexcept
	{
		return read(static_cast<int>(r));
	}

	inline void setR(RegIndex r, quint64 value) noexcept
	{
		write(static_cast<int>(r), value);
	}

	// Convenience helpers for common calling-standard registers.
	// R26: return address
	// R29: global pointer
	inline quint64 getReturnAddress() const noexcept { return read(R26); }
	inline void    setReturnAddress(quint64 v) noexcept { write(R26, v); }

	inline quint64 getGlobalPointer() const noexcept { return read(R29); }
	inline void    setGlobalPointer(quint64 v) noexcept { write(R29, v); }

private:
	quint64 m_regs[RegisterCount]{};
};

// ============================================================================
// Floating-Point Register Bank (F0-F31)
// ============================================================================
//
// This class models the 32 architectural floating-point registers of a single
// Alpha CPU (F0-F31).
//
// References:
//   - Alpha Architecture Reference Manual, floating-point register set.
//   - F0 is used as a scratch in many calling standards; F31 is often used as
//     a constant (e.g., 1.0) by convention, not hardware.
//
// Ownership:
//   - One instance per AlphaCPU.
// ============================================================================

class RegisterBankFP
{
public:
	static constexpr int RegisterCount = 32;

	enum RegIndex : int
	{
		F0 = 0, F1 = 1, F2 = 2, F3 = 3,
		F4 = 4, F5 = 5, F6 = 6, F7 = 7,
		F8 = 8, F9 = 9, F10 = 10, F11 = 11,
		F12 = 12, F13 = 13, F14 = 14, F15 = 15,
		F16 = 16, F17 = 17, F18 = 18, F19 = 19,
		F20 = 20, F21 = 21, F22 = 22, F23 = 23,
		F24 = 24, F25 = 25, F26 = 26, F27 = 27,
		F28 = 28, F29 = 29, F30 = 30, F31 = 31
	};

	RegisterBankFP() noexcept
	{
		reset();
	}

	// Reset all FP registers to 0.0 (bitwise 0).
	inline void reset() noexcept
	{
		for (int i = 0; i < RegisterCount; ++i)
		{
			m_regs[i] = 0;
		}
	}

	inline quint64 read(int index) const noexcept
	{
#if REGISTERBANK_ENABLE_BOUNDS_CHECK
		if (index < 0 || index >= RegisterCount)
		{
			return 0;
		}
#endif
		return m_regs[index];
	}

	inline void write(int index, quint64 value) noexcept
	{
#if REGISTERBANK_ENABLE_BOUNDS_CHECK
		if (index < 0 || index >= RegisterCount)
		{
			return;
		}
#endif
		m_regs[index] = value;
	}

	inline quint64 getF(RegIndex f) const noexcept
	{
		return read(static_cast<int>(f));
	}

	inline void setF(RegIndex f, quint64 value) noexcept
	{
		write(static_cast<int>(f), value);
	}

private:
	quint64 m_regs[RegisterCount]{};
};

// ============================================================================
// Example Integration Sketch (not part of the framework, for reference only)
// ============================================================================
//
// class AlphaCPU {
// public:
//     // Construction: each CPU owns its own register banks.
//     AlphaCPU(quint8 cpuId)
//         : m_cpuId(cpuId)
//     {
//         // Optional: further initialization
//     }
//
//     // Integer regs accessors used by CPUStateIPRInterface and executors:
//     inline quint64 getIntReg(int index) const noexcept
//     {
//         return m_intRegs.get(index);
//     }
//
//     inline void setIntReg(int index, quint64 value) noexcept
//     {
//         m_intRegs.set(index, value);
//     }
//
//     // FP regs:
//     inline quint64 getFpReg(int index) const noexcept
//     {
//         return m_fpRegs.get(index);
//     }
//
//     inline void setFpReg(int index, quint64 value) noexcept
//     {
//         m_fpRegs.set(index, value);
//     }
//
//     quint8 cpuId() const noexcept { return m_cpuId; }
//
// private:
//     quint8          m_cpuId{0};
//     RegisterBankInteger m_intRegs;
//     RegisterBankFP      m_fpRegs;
//     // IPRStorage        m_iprs;       // from IPRStorage_core.h
//     // HWPCB             m_hwpcb;      // from HWPCB_core.h
//     // Other CPU state...
// };
//
// Then CPUStateIPRInterface can hold a pointer/reference to AlphaCPU and simply
// forward getIntReg/setIntReg and getFpReg/setFpReg to these methods.
//
// This keeps:
//   - Architectural register ownership inside AlphaCPU.
//   - IPR and HWPCB ownership in their global Meyer's singletons.
//   - PAL and PAL helpers (TBCHK, TBIS, etc.) talking to AlphaCPU through
//     CPUStateIPRInterface, not to globals directly.
//
// ============================================================================

