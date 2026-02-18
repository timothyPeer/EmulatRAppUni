#ifndef LDST_CORE_INL_H
#define LDST_CORE_INL_H

#include <QtGlobal>
#include "AlphaCPU.h"
#include "../memoryLib/global_safeMemory.h"
#include "../memoryLib/SafeMemory.h"
#include "global_ReservationManager.h"
#include "../faultLib/scheduleMemoryTrap.h"

// For readability, alias the SafeMemory status type.
// Adjust if your actual type is SafeMemory::Status.
// -------------------------------------------------------------------------- -
// loadLongword: 32-bit load with sign extension (used by LDL/LDL_L)
//
// EA: effective address in virtual address space.
// On error: scheduleMemoryTrap() and return 0 (result is architecturally
//           undefined on fault; PAL will handle the trap before next instr).
// ---------------------------------------------------------------------------
inline quint32 loadLongWord(AlphaCPU& argCpu, quint64 ea) noexcept
{
	auto& mem = global_SafeMemory();
	quint32 outValue = 0;
	MEM_STATUS memStat = mem.load32(ea, outValue);

	if (memStat != MEM_STATUS::Ok) {
		// Determine fault type from status
		MemoryFaultType faultType;
		switch (memStat) {
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;
		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::LOAD_ACCESS;
			break;
		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;
		default:
			faultType = MemoryFaultType::LOAD_ACCESS;
			break;
		}

		scheduleMemoryTrap(argCpu, ea, /*isWrite=*/false, faultType);
		return 0;  // Dummy value (trap will prevent use)
	}

	return outValue;
}
// ---------------------------------------------------------------------------
// storeLongword: 32-bit store (used by STL/STL_C)
//
// EA: effective address in virtual address space.
// On error: scheduleMemoryTrap(); store may be partial/undefined per Alpha
//           specification, but PAL will see the fault.
// ---------------------------------------------------------------------------
inline void storeLongWord(AlphaCPU& argCpu, quint64 ea, quint32 value) noexcept
{
	auto& mem = global_SafeMemory();
	MEM_STATUS memStat = mem.store32(ea, value);

	if (memStat != MEM_STATUS::Ok) {
		// Determine fault type from status
		MemoryFaultType faultType;
		switch (memStat) {
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;
		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;
		default:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		}

		scheduleMemoryTrap(argCpu, ea, /*isWrite=*/true, faultType);
		// Store fails silently; trap will be handled before next instruction
	}
}

// ---------------------------------------------------------------------------
// loadQuadword: 64-bit load (used by LDQ/LDQ_L).
// ---------------------------------------------------------------------------
inline quint64 loadQuadWord(AlphaCPU& argCpu, quint64 ea) noexcept
{
	auto& mem = global_SafeMemory();
	quint64 outValue = 0;
	MEM_STATUS memStat = mem.load64(ea, outValue);

	if (memStat != MEM_STATUS::Ok) {
		MemoryFaultType faultType;
		switch (memStat) {
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;
		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::LOAD_ACCESS;
			break;
		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;
		default:
			faultType = MemoryFaultType::LOAD_ACCESS;
			break;
		}

		scheduleMemoryTrap(argCpu, ea, /*isWrite=*/false, faultType);
		return 0;
	}

	return outValue;
}
// ---------------------------------------------------------------------------
// storeQuadword: 64-bit store (used by STQ/STQ_C).
// ---------------------------------------------------------------------------
inline void storeQuadWord(AlphaCPU& argCpu, quint64 ea, quint64 value) noexcept
{
	auto& mem = global_SafeMemory();
	MEM_STATUS memStat = mem.store64(ea, value);

	if (memStat != MEM_STATUS::Ok) {
		MemoryFaultType faultType;
		switch (memStat) {
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;
		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;
		default:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		}

		scheduleMemoryTrap(argCpu, ea, /*isWrite=*/true, faultType);
	}
}
// ---------------------------------------------------------------------------
// storeLongwordConditional:
//   Helper for STL_C.
//
// You will eventually want to:
//
/*
	1. Check the CPU's reservation state (ReservationManager) for the EA
	   granule.
	2. If reservation is valid, perform storeLongword() and clear the
	   reservation; return true.
	3. Otherwise, return false without storing.
*/
// For now, this stub only schedules traps on memory errors and always
// returns false on SafeMemory failure.
// ---------------------------------------------------------------------------
inline bool storeLongwordConditional(AlphaCPU& argCpu,
	quint64 ea,
	quint32 value) noexcept
{
	// -----------------------------------------------------------------------
	// 1. Check Reservation
	// -----------------------------------------------------------------------
	// The reservation granule is implementation-defined (Alpha uses at least
	// 32 bytes, sometimes 64). Your ReservationManager will determine this
	// via its own configuration.
	//
	// If the reservation is not valid for this CPU+EA, STL_C MUST:
	//    • not store
	//    • return FALSE
	//    • NOT raise a trap
	//
	// Reference: Alpha AXP ISA, "Load-locked/Store-conditional".
	// -----------------------------------------------------------------------

	auto& resMgr = global_ReservationManager();

	if (!resMgr.hasReservation(argCpu.cpuId(), ea))
	{
		// No trap. Just return 0 (failure).
		// Architecturally correct behavior.
		return false;
	}

	// -----------------------------------------------------------------------
	// 2. Attempt the Store
	// -----------------------------------------------------------------------
	auto& mem = global_SafeMemory();
	MEM_STATUS memStat = mem.store32(ea, value);

	if (memStat != MEM_STATUS::Ok)
	{
		// Store attempt failed -> schedule synchronous memory trap.
		// STL_C result is architecturally undefined when a trap is raised,
		// but we return false to avoid misleading the grain layer.

		MemoryFaultType faultType;
		switch (memStat)
		{
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;

		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;

		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;

		default:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		}

		scheduleMemoryTrap(argCpu, ea, /*isWrite=*/true, faultType);
		return false;
	}

	// -----------------------------------------------------------------------
	// 3. Store Succeeded -> Clear Reservation
	// -----------------------------------------------------------------------
	//
	// Alpha ISA requires that:
	//    • On a SUCCESSFUL STx_C, the reservation is cleared.
	//    • On a FAILED STx_C due to "lost reservation", the reservation is
	//      also cleared.
	//    • On a trap during STx_C, the reservation is cleared as well.
	//
	// HOWEVER - here, since SafeMemory succeeded, and *reservation was valid*,
	// we only clear on success. (Your ReservationManager should clear for any
	// failed explicit call as well.)
	// -----------------------------------------------------------------------

	resMgr.clearReservation(argCpu.cpuId());

	return true;  // Store was successful
}

// ---------------------------------------------------------------------------
// storeQuadwordConditional:
//   Helper for STQ_C (64-bit store-conditional).
//
// Semantics (Alpha AXP ISA):
//   - Store to memory ONLY if the CPU holds a valid reservation for this
//     address granule (established by a prior LDQ_L).
//   - On success:
//       • The store is performed.
//       • The reservation is cleared.
//       • The instruction result register receives 1 (non-zero).
//   - On failure due to lost reservation:
//       • No store is performed.
//       • The reservation is cleared.
//       • The instruction result receives 0.
//       • No trap is taken.
//   - On memory fault (alignment, access violation, TLB miss):
//       • A synchronous memory trap is scheduled.
//       • The reservation is cleared.
//       • Result is architecturally undefined; we return false here.
//
// Reference:
//   - Alpha AXP Architecture Reference Manual, 1994, "Load-locked and
//     Store-conditional Instructions" (LDL_L/LDQ_L and STL_C/STQ_C).
// ---------------------------------------------------------------------------
inline bool storeQuadwordConditional(AlphaCPU& argCpu,
	quint64 ea,
	quint64 value) noexcept
{
	auto& resMgr = global_ReservationManager();

	// 1) Check reservation: if not valid, STQ_C fails silently (no trap).
	if (!resMgr.hasReservation(argCpu.cpuId(), ea)) {
		// Per Alpha ISA: no store, result = 0, reservation cleared.
		resMgr.clearReservation(argCpu.cpuId());
		return false;
	}

	// 2) Attempt the store.
	auto& mem = global_SafeMemory();
	MEM_STATUS memStat = mem.store64(ea, value);

	if (memStat != MEM_STATUS::Ok) {
		// Store fault -> schedule a memory trap.
		MemoryFaultType faultType;
		switch (memStat) {
		case MEM_STATUS::Un_Aligned:
			faultType = MemoryFaultType::ALIGNMENT_FAULT;
			break;
		case MEM_STATUS::AccessViolation:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		case MEM_STATUS::TlbMiss:
			faultType = MemoryFaultType::TLB_MISS;
			break;
		default:
			faultType = MemoryFaultType::STORE_ACCESS;
			break;
		}

		updateMemoryTrap_IPR(argCpu, ea, /*isWrite=*/true, faultType);

		// On any fault, reservation is no longer valid.
		resMgr.clearReservation(argCpu.cpuId());
		return false;
	}

	// 3) Successful store: clear the reservation and return success.
	resMgr.clearReservation(argCpu.cpuId());
	return true;
}



#endif // LDST_CORE_INL_H
