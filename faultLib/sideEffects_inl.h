#pragma once
#include "raiseTrap_inl.h"
#include "CPUStateIPRInterface.h"
#include "types_core.h"

// ============================================================================
// Integer Overflow Side-Effect Handling
// ----------------------------------------------------------------------------
// These helpers centralize how integer overflows update architectural state.
//
// Alpha AXP Architecture:
//   - ADDL  sets V flag in the PSW (in IPR).
//   - ADDLV sets V and TRAPS.
//   - Alpha's integer overflow does NOT use FPCR.
// ============================================================================

// ---------------------------------------------------------------------------
// Set or clear the integer overflow flag (V-bit) in the CPU's PS/Flags
// ---------------------------------------------------------------------------
inline void setIntegerOverflowFlag(CPUStateIPRInterface& cpuState, bool value) noexcept
{
	// Write into the IPR storage for this CPU
	// Architectural V-bit is in the PS register (Processor Status)
	
	CPUIdType cpuId = cpuState.cpuId();
	/*auto& iprs = globalIPRBank()[cpuId];*/
	constexpr quint64 V_BIT = (1ull << 1);   // SRM: PS.V = bit <1>

	quint64 ps = getPS_Active(cpuId);
	if (value)
		ps |= V_BIT;
	else
		ps &= ~V_BIT;

	setPS_Active(cpuId,ps);
}

