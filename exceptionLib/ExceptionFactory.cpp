// ============================================================================
// ExceptionFactory.cpp - Updated for existing MachineCheckReason enum
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#include "ExceptionFactory.h"
#include "../palLib_EV6/Pal_Service.h"
#include "../faultLib/PendingEvent_Refined.h"
#include "../coreLib/types_core.h"
#include "PendingEventKind.h"
#include "../exceptionLib/ExceptionClass_EV6.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../memoryLib/memory_core.h"
#include "../coreLib/enum_MCES.h"
#include "../coreLib/VA_types.h"
#include "../coreLib/CurrentCpuTls.h"
#include "../pteLib/AlphaPTE_Core.h"
#include "faultLib/global_faultDispatcher.h"
#include "faultLib/FaultDispatcher.h"
#include "palLib_EV6/Global_PALVectorTable.h"
#include "palLib_EV6/PalVectorTable_final.h"
#include "../coreLib/LoggingMacros.h"

/**
* @brief Create PendingEvent for invalid PTE (fetched but malformed)
*
* Raised when page table walk succeeds but PTE is invalid/malformed.
* Different from DTB_MISS (which is "PTE not found").
*
* @param cpuId CPU identifier
* @param va Virtual address
* @param pte The invalid PTE that was fetched
* @return PendingEvent configured as invalid PTE fault
*/
AXP_HOT AXP_FLATTEN PendingEvent makeInvalidPTE(
	CPUIdType cpuId,
	quint64 va,
	const AlphaPTE& pte) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;  // Data fault (invalid PTE)
	ev.faultVA = va;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.isInvalidPTE = true;
	ev.pendingEvent_Info.pteValue = pte.raw;  // Store PTE for debugging

	return ev;
}

/**
* @brief Create PendingEvent for illegal instruction
*
* Raised when instruction decoder encounters illegal/reserved opcode.
*
* @param trapCode Trap classification
* @param faultPC PC of illegal instruction
* @return PendingEvent configured as OPCDEC
*/
AXP_HOT AXP_FLATTEN PendingEvent makeIllegalInstruction(
	TrapCode_Class trapCode,
	quint64 faultPC) noexcept
{
    CPUIdType cpuId = static_cast<CPUIdType>(CurrentCpuTLS::get());

	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::OpcDec;
	ev.faultPC = faultPC;
	ev.faultVA = 0;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.trapCode = trapCode;
	ev.pendingEvent_Info.isIllegalInstruction = true;

	return ev;
}


/**
 * @brief Create PendingEvent for access violation fault
 *
 * Raised when permission check fails (read/write/execute).
 * Combines DTB and ITB access violations.
 *
 * @param cpuId CPU identifier
 * @param va Virtual address with permission violation
 * @param isWrite true for store, false for load/execute
 * @return PendingEvent configured as access violation
 */
AXP_HOT AXP_FLATTEN PendingEvent makeAccessViolationFault(
	CPUIdType cpuId,
	quint64 va,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;  // Data fault (ACV)
	ev.faultVA = va;
	ev.faultPC = globalHWPCB(cpuId).pc;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM();

	ev.pendingEvent_Info.isAccessViolation = true;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;

	return ev;
}
/***
 * @brief Create PendingEvent for fatal SMP barrier rendezvous timeout
 *
 * This event indicates that a global SMP memory barrier rendezvous failed
 * to complete (one or more CPUs did not acknowledge within the timeout).
 *
 * Architectural note:
 * - It is unsafe to "force complete" a global barrier; doing so can violate
 *   Alpha memory ordering guarantees.
 * - Policy should treat this as a fatal machine check / halt condition.
 *
 * References:
 * - Alpha Architecture: memory ordering model and barriers (MB/WMB/TRAPB)
 * - Platform policy: fatal SMP coordination failure -> machine check / halt
 *
 * @param cpuId CPU that detected the timeout
 * @param initiatingCpu CPU that initiated the barrier rendezvous (optional diagnostics)
 * @param participatingCpus expected participants (optional diagnostics)
 * @param acknowledgedCpus number that acknowledged (optional diagnostics)
 * @return PendingEvent configured as fatal machine check
 */
AXP_HOT AXP_FLATTEN PendingEvent makeSmpBarrierTimeoutEvent(
	CPUIdType cpuId,
	CPUIdType initiatingCpu,
	quint32 participatingCpus,
	quint32 acknowledgedCpus) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;

	// Use MachineCheck as the canonical fatal path.
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;

	ev.faultPC = globalHWPCB(cpuId).pc;
	ev.faultVA = 0;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM();

	ev.pendingEvent_Info.isMachineCheck = true;

	// If you have a dedicated reason enum, prefer it.
	// Otherwise reuse/extend MachineCheckReason with a new value.
	ev.pendingEvent_Info.machineCheckReason = MachineCheckReason::SMP_BARRIER_TIMEOUT;

	// Optional diagnostics (store in "info" payload fields you already have)
	ev.pendingEvent_Info.initiatingCpu = initiatingCpu;
	ev.pendingEvent_Info.participatingCpus = participatingCpus;
	ev.pendingEvent_Info.acknowledgedCpus = acknowledgedCpus;
	ev.pendingEvent_Info.isSmpRendezvousFailure = true;

	// Optional: set MCES bits if your PAL/mchk flow uses them.
	// ev.pendingEvent_Info.mcesBits = (quint64)MCESBits::SOME_FATAL_BIT;

	return ev;
}


/**
 * @brief Variant without cpuId (uses TLS)
 */
AXP_HOT AXP_FLATTEN PendingEvent makeAccessViolationFault(
	quint64 va,
	bool isWrite) noexcept
{
    CPUIdType cpuId = static_cast<CPUIdType>(CurrentCpuTLS::get());
	return makeAccessViolationFault(cpuId, va, isWrite);
}

/**
 * @brief Create PendingEvent for memory fault (bus error, machine check)
 *
 * Raised when physical memory access fails after successful translation.
 *
 * @param cpuId CPU identifier
 * @param va Virtual address (for debugging)
 * @return PendingEvent configured as memory fault
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMemoryFault(
	CPUIdType cpuId,
	quint64 va) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;  // Machine check
	ev.faultVA = va;
	ev.faultPC = globalHWPCB(cpuId).pc;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM();

	ev.pendingEvent_Info.isMachineCheck = true;
	ev.pendingEvent_Info.machineCheckReason =
		MachineCheckReason::MEMORY_BUS_ERROR;

	return ev;
}

/**
* @brief Create PendingEvent for Fault-on-Read
*
* Raised when PTE has FOR (Fault-on-Read) bit set.
* Used by OS for demand paging, copy-on-write, etc.
*
* @param cpuId CPU identifier
* @param faultVA Virtual address with FOR bit set
* @return PendingEvent configured as fault-on-read
*/
AXP_HOT AXP_FLATTEN PendingEvent makeFaultOnReadEvent(
	CPUIdType cpuId,
	quint64 faultVA) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;  // Data fault
	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM();

	ev.pendingEvent_Info.isFaultOnRead = true;
	ev.pendingEvent_Info.isWrite = false;
	ev.pendingEvent_Info.accessType = MemoryAccessType::READ;

	return ev;
}

/**
 * @brief Create PendingEvent for Fault-on-Write
 *
 * Raised when PTE has FOW (Fault-on-Write) bit set.
 * Used by OS for copy-on-write, write protection, etc.
 *
 * @param cpuId CPU identifier
 * @param faultVA Virtual address with FOW bit set
 * @return PendingEvent configured as fault-on-write
 */
AXP_HOT AXP_FLATTEN PendingEvent makeFaultOnWriteEvent(
	CPUIdType cpuId,
	quint64 faultVA) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;  // Data fault
	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.isFaultOnWrite = true;
	ev.pendingEvent_Info.isWrite = true;
	ev.pendingEvent_Info.accessType = MemoryAccessType::WRITE;

	return ev;
}

/**
		 * @brief Create PendingEvent for Fault-on-Execute
		 *
		 * Raised when PTE has FOE (Fault-on-Execute) bit set.
		 * Used by OS for execute protection, code paging, etc.
		 *
		 * @param cpuId CPU identifier
		 * @param faultVA Virtual PC with FOE bit set
		 * @return PendingEvent configured as fault-on-execute
		 */
AXP_HOT AXP_FLATTEN PendingEvent makeFaultOnExecuteEvent(
	CPUIdType cpuId,
	quint64 faultVA) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::ItbAcv;  // Instruction ACV
	ev.faultVA = faultVA;
	ev.faultPC = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.isFaultOnExecute = true;
	ev.pendingEvent_Info.accessType = MemoryAccessType::EXECUTE;

	return ev;
}
/**
* @brief Create PendingEvent for translation fault based on TranslationResult
*
* Maps TranslationResult to appropriate exception class and creates PendingEvent.
*
* @param cpuId CPU identifier
* @param va Virtual address that caused translation fault
* @param tr TranslationResult indicating fault type
* @param isWrite true for store, false for load
* @return PendingEvent configured for the specific translation fault
*/
AXP_HOT AXP_FLATTEN PendingEvent makeTranslationFault(
	CPUIdType cpuId,
	quint64 va,
	TranslationResult tr,
	bool isWrite) noexcept
{
    quint64 pc = globalHWPCB(cpuId).pc ;
    ASNType asn = globalHWPCB(cpuId).asn;
	switch (tr) {
	case TranslationResult::Success:
		// Should not be called for success
		return PendingEvent{};

	case TranslationResult::TlbMiss:
	case TranslationResult::DlbMiss:
        return makeDTBMissSingleEvent(cpuId, va,asn,pc, isWrite);

	case TranslationResult::IlbMiss:
		return makeITBMissEvent(cpuId, va);

	case TranslationResult::AccessViolation:
		return makeDTBAccessViolationEvent(cpuId, va, isWrite);

	case TranslationResult::FaultOnRead:
		return makeFaultOnReadEvent(cpuId, va);

	case TranslationResult::FaultOnWrite:
		return makeFaultOnWriteEvent(cpuId, va);

	case TranslationResult::FaultOnExecute:
		return makeFaultOnExecuteEvent(cpuId, va);

	case TranslationResult::NonCanonical:
		return makeNonCanonicalAddressEvent(cpuId, va, isWrite);

	case TranslationResult::Unaligned:
		return makeUnalignedEvent(cpuId, va, isWrite);

	// case TranslationResult::
	// 	return makeDTBDoubleMissEvent(cpuId, va, isWrite);

	default:
		// Unknown translation result - treat as general fault
		return makeDTBAccessViolationEvent(cpuId, va, isWrite);
	}
}
/**
 * @brief Simplified variant when PA is not available
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMemoryAccessFault(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	return makeMemoryAccessFault(cpuId, faultVA, 0xDEADBEEFDEADBEEF, isWrite);
}

/**
 * @brief Create PendingEvent for physical memory access failure
 *
 * Called when SafeMemory/guestMemory read/write fails AFTER successful
 * translation. This is a physical memory error, not a translation fault.
 *
 * Typical causes:
 * - Bus error (hardware failure)
 * - Physical address out of range
 * - Memory system error
 * - Cache parity error
 *
 * @param cpuId CPU identifier
 * @param faultVA Virtual address that caused the fault (for debugging)
 * @param pa Physical address that failed (optional, for diagnostics)
 * @param isWrite true if write failed, false if read failed
 * @return PendingEvent configured as memory access fault
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMemoryAccessFault(
	CPUIdType cpuId,
	quint64 faultVA,
	quint64 pa,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;

	// Alpha architecture: physical memory errors are machine checks
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;

	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	// Store physical address for diagnostics
	ev.pendingEvent_Info.physicalAddress = pa;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;

	// Machine check specific info
	ev.pendingEvent_Info.isMachineCheck = true;
	ev.pendingEvent_Info.machineCheckReason =
		MachineCheckReason::MEMORY_BUS_ERROR;

	return ev;
}


/**
 * @brief Variant without cpuId (uses TLS)
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMemoryFault(quint64 va) noexcept
{
    CPUIdType cpuId = static_cast<CPUIdType>(CurrentCpuTLS::get());
	return makeMemoryFault(cpuId, va);
}

/**
 * @brief Alternative: Use DSTREAM exception for memory system errors
 *
 * Some Alpha implementations distinguish between:
 * - MCHK: Hardware/processor errors (unrecoverable)
 * - DSTREAM: Memory system errors (potentially recoverable)
 *
 * Use this variant if your architecture treats memory access failures
 * as recoverable stream errors rather than fatal machine checks.
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMemoryStreamFault(
	CPUIdType cpuId,
	quint64 faultVA,
	quint64 pa,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::DStream;  // D-stream error

	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.physicalAddress = pa;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;

	return ev;
}
/**
* @brief Create PendingEvent for non-canonical virtual address
*
* Raised when VA is outside valid virtual address space.
* Alpha uses 43-bit virtual addresses (sign-extended from bit 42).
*
* @param cpuId CPU identifier
* @param faultVA Non-canonical virtual address
* @param isWrite true for store, false for load
* @return PendingEvent configured as non-canonical address fault
*/
AXP_HOT AXP_FLATTEN PendingEvent makeNonCanonicalAddressEvent(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;  // Data fault
	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.isNonCanonical = true;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;

	return ev;
}


/**
 * @brief Create PendingEvent for double DTB miss
 *
 * Raised when DTB miss occurs during PAL's page table walk for a
 * previous DTB miss (recursive miss). Requires special handling.
 *
 * Alpha distinguishes between:
 * - DTB_MISS_SINGLE: Normal TLB miss
 * - DTB_MISS_DOUBLE: Miss during page table walk
 *
 * @param cpuId CPU identifier
 * @param faultVA Virtual address that caused double miss
 * @param isWrite true for store, false for load
 * @return PendingEvent configured as double DTB miss
 */
AXP_HOT AXP_FLATTEN PendingEvent makeDTBDoubleMissEvent(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dtb_miss_double_4;  // Double miss
	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.cm = globalHWPCB(cpuId).getCM() ;

	ev.pendingEvent_Info.isDoubleMiss = true;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;

	return ev;
}

/**
 * @brief Create DTB miss event (3-level page table)
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of data access
 * @param isWrite True if write access, false if read
 */
AXP_HOT AXP_FLATTEN PendingEvent makeDTBMissDouble3Event(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dtb_miss_double_4;
	ev.faultVA = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.faultType = MemoryFaultType::DTB_MISS_READ;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;
	return ev;
}

AXP_HOT  AXP_FLATTEN PendingEvent makeDTBMissFault(CPUIdType cpuId, VAType va, ASNType asn, quint64 pc, bool isWrite )  noexcept
{
	return makeDTBMissSingleEvent(cpuId, va, asn, pc, isWrite);
}
/**
 * @brief Create DTB miss event (single-level page table)
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of data access
 * @param isWrite True if write access, false if read
 */
AXP_HOT  AXP_FLATTEN PendingEvent makeDTBMissSingleEvent( CPUIdType cpuId, quint64 faultVA,ASNType asn, quint64 faultPC, bool isWrite) noexcept
{
	PendingEvent ev{};
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dtb_miss_single;
	ev.faultVA = faultVA;
	ev.asn = asn;
	ev.faultPC = faultPC;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.faultType = MemoryFaultType::DTB_MISS_READ;
	ev.pendingEvent_Info.accessType = isWrite ? MemoryAccessType::WRITE
		: MemoryAccessType::READ;
	return ev;
}



/**
 * @brief Create ITB access violation event
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of instruction fetch
 */
AXP_HOT AXP_FLATTEN PendingEvent makeITBAccessViolationEvent(
	CPUIdType cpuId,
	quint64 faultVA) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::ItbAcv;
	ev.faultVA = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.pendingEvent_Info.isInstruction = true;
	ev.pendingEvent_Info.isExecute = true;
	ev.pendingEvent_Info.faultType = MemoryFaultType::ITB_ACCESS_VIOLATION;
	ev.pendingEvent_Info.accessType = MemoryAccessType::EXECUTE;
	return ev;
}

/**
 * @brief Create ITB miss event
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of instruction fetch
 */
AXP_HOT AXP_FLATTEN PendingEvent makeITBMissEvent(
	CPUIdType cpuId,
	quint64 faultVA) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::ItbMiss;
	ev.faultVA = faultVA;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.pendingEvent_Info.isInstruction = true;
	ev.pendingEvent_Info.isExecute = true;
	ev.pendingEvent_Info.faultType = MemoryFaultType::ITB_MISS;
	ev.pendingEvent_Info.accessType = MemoryAccessType::EXECUTE;
	return ev;
}


/**
 * @brief Create machine check event
 * @param cpuId CPU that raised exception
 * @param reason Machine check reason code
 * @param errorAddr Address associated with machine check
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMachineCheckEvent(
	CPUIdType cpuId,
	MachineCheckReason reason,
	quint64 errorAddr) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::MachineCheck;
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;
	ev.mcReason = reason;
	ev.faultVA = errorAddr;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create reset/wakeup event
 * @param cpuId CPU that raised exception
 */
AXP_HOT AXP_FLATTEN PendingEvent makeResetEvent(CPUIdType cpuId) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Reset;
	ev.exceptionClass = ExceptionClass_EV6::Reset;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create software trap event (GENTRAP - CALL_PAL 0xAA)
 * @param cpuId CPU that raised exception
 * @param faultPC PC of GENTRAP instruction
 * @param trapCode Software trap code (stored in R16 typically)
 */
AXP_HOT AXP_FLATTEN PendingEvent makeSoftwareTrapEvent(
	CPUIdType cpuId,
	quint64 faultPC,
	quint64 trapCode) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Interrupt;
	ev.extraInfo = trapCode;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create CALL_PAL event
 * @param cpuId CPU that raised exception
 * @param faultPC PC of CALL_PAL instruction
 * @param palFunction PAL function code from instruction [7:0]
 */
AXP_HOT AXP_FLATTEN PendingEvent makeCallPalEvent(
	CPUIdType cpuId,
	quint64 faultPC,
	quint8 palFunction) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::CallPal;
	ev.extraInfo = palFunction;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create breakpoint event (BPT - CALL_PAL 0x80)
 * @param cpuId CPU that raised exception
 * @param faultPC PC of BPT instruction
 */
AXP_HOT AXP_FLATTEN PendingEvent makeBreakpointEvent(
	CPUIdType cpuId,
	quint64 faultPC) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;
	ev.extraInfo = 0x80;  // BPT function code
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create bugcheck event (BUGCHECK - CALL_PAL 0x81)
 * @param cpuId CPU that raised exception
 * @param faultPC PC of BUGCHECK instruction
 */
AXP_HOT AXP_FLATTEN PendingEvent makeBugcheckEvent(
	CPUIdType cpuId,
	quint64 faultPC) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;
	ev.extraInfo = 0x81;  // BUGCHECK function code
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc;
	return ev;
}


/**
 * @brief Create MT_FPCR trap event
 * @param cpuId CPU that raised exception
 * @param faultPC PC of MT_FPCR instruction
 */
AXP_HOT AXP_FLATTEN PendingEvent makeMTFPCREvent(
	CPUIdType cpuId,
	quint64 faultPC) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::MT_FPCR;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc;
	return ev;
}

/**
 * @brief Create arithmetic trap event
 * @param cpuId CPU that raised exception
 * @param faultPC PC of arithmetic instruction
 * @param excSumBits FP exception summary bits from FPCR
 */
AXP_HOT AXP_FLATTEN PendingEvent makeArithmeticEvent(
	CPUIdType cpuId,
	quint64 faultPC,
	quint64 excSumBits) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Arithmetic;
	ev.extraInfo = excSumBits;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}
/**
 * @brief Create illegal opcode event
 * @param cpuId CPU that raised exception
 * @param faultPC PC of illegal instruction
 * @param instruction Illegal instruction word
 */
AXP_HOT AXP_FLATTEN PendingEvent makeIllegalOpcodeEvent(
	CPUIdType cpuId,
	quint64 faultPC,
	quint32 instruction) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::OpcDec;
	ev.extraInfo = instruction;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create floating-point disabled event
 * @param cpuId CPU that raised exception
 * @param faultPC PC of FP instruction
 */
AXP_HOT AXP_FLATTEN PendingEvent makeFENEvent(
	CPUIdType cpuId,
	quint64 faultPC) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Fen;
	if (faultPC == 0)
		ev.faultPC = faultPC;
	else
		ev.faultPC = globalHWPCB(cpuId).pc ;
	return ev;
}

/**
 * @brief Create unaligned access event
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of unaligned access
 * @param isWrite True if write access, false if read
 */
AXP_HOT AXP_FLATTEN PendingEvent makeUnalignedEvent(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Unalign;
	ev.faultVA = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.pendingEvent_Info.isUnaligned = true;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;
	return ev;
}

/**
 * @brief Create DTB access violation event
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of data access
 * @param isWrite True if write access, false if read
 */
AXP_HOT AXP_FLATTEN PendingEvent makeDTBAccessViolationEvent(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;
	ev.faultVA = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isWrite = isWrite;
    ev.pendingEvent_Info.faultType = MemoryFaultType::DTB_ACCESS_VIOLATION_READ;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;
	return ev;
}

/**
 * @brief Create DTB fault event (FOE/FOR/FOW/sign check)
 * @param cpuId CPU that raised exception
 * @param faultVA Virtual address of data access
 * @param isWrite True if write access, false if read
 * @param faultType Specific fault type (FOE/FOR/FOW/etc)
 */
AXP_HOT AXP_FLATTEN PendingEvent makeDTBFaultEvent(
	CPUIdType cpuId,
	quint64 faultVA,
	bool isWrite,
	MemoryFaultType faultType) noexcept
{
	PendingEvent ev;
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::Dfault;
	ev.faultVA = faultVA;
	ev.asn = globalHWPCB(cpuId).asn;
	ev.faultPC = globalHWPCB(cpuId).pc ;
	ev.pendingEvent_Info.isInstruction = false;
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.faultType = faultType;
	ev.pendingEvent_Info.accessType = isWrite ?
		MemoryAccessType::WRITE : MemoryAccessType::READ;
	return ev;
}

AXP_HOT AXP_FLATTEN void updateMemoryTrap_IPR(CPUIdType cpuId, quint64 faultVA, bool isWrite, MemoryFaultType faultType) noexcept
{
	auto& iprs = globalIPRHotExt(cpuId);

	// Set MM_STAT classification bits based on fault type.
	// Values here are placeholders and should be aligned with your
	// MM_STAT layout and ASA definitions.
	switch (faultType)
	{
    case MemoryFaultType::FAULT_ON_READ:
		iprs.mm_stat = 0x0001;  // Data read access violation
		break;
    case MemoryFaultType::FAULT_ON_WRITE:
		iprs.mm_stat = 0x0002;  // Data write access violation
		break;
    case MemoryFaultType::ITB_MISS:
		iprs.mm_stat = 0x0004;  // Translation buffer miss
		break;
	case MemoryFaultType::ALIGNMENT_FAULT:
		iprs.mm_stat = 0x0008;  // Unaligned data reference
		break;
	default:
		iprs.mm_stat = 0x0000;
		break;
	}

	// TRACE: Report memory fault scheduling
	TRACE_LOG(QStringLiteral("scheduleMemoryTrap: cpu=%1 va=0x%2 write=%3 type=%4 pc=0x%5")
		.arg(cpuId)
		.arg(faultVA, 16, 16, QChar('0'))
		.arg(isWrite)
		.arg(static_cast<int>(faultType))
		.arg(globalHWPCB(cpuId).pc , 16, 16, QChar('0'))
	);

	// Map MEMORY_FAULT exception -> PAL vector via PAL vector table.
    auto& palTable = global_PalVectorTable();
    const PalVectorId_EV6 palVec = palTable.mapException(ExceptionClass_EV6::Dfault);

	// Build PendingEvent (synchronous memory trap)
	PendingEvent ev{};
	ev.kind = PendingEventKind::Exception;
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;
	ev.palVectorId = palVec;
	ev.faultVA = faultVA;         // Faulting virtual address
	ev.extraInfo = iprs.mm_stat;    // MM_STAT classification

	// Memory trap metadata
	ev.pendingEvent_Info.isWrite = isWrite;
	ev.pendingEvent_Info.isInstruction = false;  // Data access, not instruction fetch
	ev.pendingEvent_Info.isUnaligned = (faultType == MemoryFaultType::ALIGNMENT_FAULT);

	// Queue trap in dispatcher
    auto& disp = global_faultDispatcher();
    disp.raiseFault(ev);

	// Architecturally: EXC_ADDR = PC of faulting instruction,
	// VA = virtual address that caused the fault.
	globalHWPCB(cpuId).exc_addr = globalHWPCB(cpuId).pc ;
	globalHWPCB(cpuId).va_fault = faultVA;  // TODO check
}

// ============================================================================
// ExceptionFactory.cpp - Updated for existing MachineCheckReason enum
// ============================================================================


PendingEvent makeMachineCheckEvent(
	CPUIdType cpuId,
	const QString& reason) noexcept
{
	PendingEvent ev;

	// Core identification
	ev.cpuId = cpuId;
	ev.eventClass = EventClass::MachineCheck;
	ev.priority = EventPriority::Critical;
	ev.kind = PendingEventKind::MachineCheck;

	// Exception classification
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;

	// Machine check specific - use PROCESSOR_ERROR as default
	ev.mcReason = MachineCheckReason::PROCESSOR_ERROR;
	ev.mchkCode = 0;
	ev.mchkAddr = 0;

	// Addresses (not applicable for general errors)
	ev.faultVA = 0;
	ev.faultPC = 0;

	// Description
	ev.description = QString("CPU %1 machine check: %2")
		.arg(cpuId)
		.arg(reason);

	return ev;
}



PendingEvent makeMachineCheckEventDetailed(
	CPUIdType cpuId,
	MachineCheckReason reason,
	quint64 faultVA,
	quint64 faultPA) noexcept
{
	PendingEvent ev;

	// Core identification
	ev.cpuId = cpuId;
	ev.eventClass = EventClass::MachineCheck;
	ev.priority = EventPriority::Critical;
	ev.kind = PendingEventKind::MachineCheck;

	// Exception classification
	ev.exceptionClass = ExceptionClass_EV6::MachineCheck;

	// Machine check specific
	ev.mcReason = reason;
	ev.mchkAddr = faultPA;
	ev.faultVA = faultVA;

	// Additional properties
	ev.pendingEvent_Info.isMachineCheck = true;
	ev.pendingEvent_Info.machineCheckReason = reason;
	ev.pendingEvent_Info.physicalAddress = faultPA;

	// Generate description based on reason
	QString reasonStr = getMachineCheckReasonString(reason);

	ev.description = QString("CPU %1 machine check: %2 (VA=0x%3, PA=0x%4)")
		.arg(cpuId)
		.arg(reasonStr)
		.arg(faultVA, 16, 16, QChar('0'))
		.arg(faultPA, 16, 16, QChar('0'));

	return ev;
}

// ============================================================================
// Helper: Get Machine Check Reason String
// ============================================================================

QString getMachineCheckReasonString(MachineCheckReason reason) noexcept
{
	switch (reason) {
		// Processor Errors
	case MachineCheckReason::PROCESSOR_ERROR:
		return "Processor error";
	case MachineCheckReason::PROCESSOR_CORRECTABLE_ERROR:
		return "Correctable processor error";
	case MachineCheckReason::EXECUTION_UNIT_ERROR:
		return "Execution unit error";
	case MachineCheckReason::REGISTER_FILE_ERROR:
		return "Register file error";
	case MachineCheckReason::PIPELINE_ERROR:
		return "Pipeline error";
	case MachineCheckReason::CONTROL_LOGIC_ERROR:
		return "Control logic error";

		// Cache Errors
	case MachineCheckReason::ICACHE_PARITY_ERROR:
		return "I-cache parity error";
	case MachineCheckReason::DCACHE_PARITY_ERROR:
		return "D-cache parity error";
	case MachineCheckReason::BCACHE_ERROR:
		return "B-cache error";
	case MachineCheckReason::SCACHE_ERROR:
		return "S-cache error";
	case MachineCheckReason::CACHE_TAG_ERROR:
		return "Cache tag error";
	case MachineCheckReason::CACHE_COHERENCY_ERROR:
		return "Cache coherency error";

		// Memory Errors
	case MachineCheckReason::SYSTEM_MEMORY_ERROR:
		return "System memory error";
	case MachineCheckReason::MEMORY_CONTROLLER_ERROR:
		return "Memory controller error";
	case MachineCheckReason::CORRECTABLE_ERROR:
		return "Correctable memory error (ECC)";
	case MachineCheckReason::UNCORRECTABLE_ERROR:
		return "Uncorrectable memory error";
	case MachineCheckReason::BUFFER_WRITE_ERROR:
		return "Buffer write error";
	case MachineCheckReason::MEMORY_BUS_ERROR:
		return "Memory bus error";

		// Bus Errors
	case MachineCheckReason::SYSTEM_BUS_ERROR:
		return "System bus error";
	case MachineCheckReason::IO_BUS_ERROR:
		return "I/O bus error";
	case MachineCheckReason::EXTERNAL_INTERFACE_ERROR:
		return "External interface error";

		// MMU/TLB Errors
	case MachineCheckReason::MMU_ERROR:
		return "MMU error";
	case MachineCheckReason::TRANSLATION_BUFFER_ERROR:
		return "TLB error";
	case MachineCheckReason::PAGE_FAULT:
		return "Page fault";
	case MachineCheckReason::TLB_INSERTION_FAILURE:
		return "TLB insertion failure";

		// SMP Errors
	case MachineCheckReason::INTERPROCESSOR_ERROR:
		return "Inter-processor error";
	case MachineCheckReason::SMP_BARRIER_TIMEOUT:
		return "SMP barrier timeout";

		// Environmental
	case MachineCheckReason::THERMAL_ERROR:
		return "Thermal error";
	case MachineCheckReason::POWER_SUPPLY_ERROR:
		return "Power supply error";
	case MachineCheckReason::CLOCK_ERROR:
		return "Clock error";

		// PALcode
	case MachineCheckReason::PALCODE_ERROR:
		return "PALcode error";

		// Critical
	case MachineCheckReason::DOUBLE_MACHINE_CHECK:
		return "DOUBLE MACHINE CHECK";

		// System Errors
	case MachineCheckReason::SYSTEM_ERROR:
		return "System error";
	case MachineCheckReason::SYSTEM_CORRECTABLE_ERROR:
		return "System correctable error";

	case MachineCheckReason::UNKNOWN_MACHINE_CHECK:
	case MachineCheckReason::NONE:
	default:
		return QString("Unknown machine check (%1)").arg(static_cast<int>(reason));
	}
}








