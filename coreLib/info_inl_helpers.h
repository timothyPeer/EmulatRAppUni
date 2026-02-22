// ============================================================================
// info_inl_helpers.h - TLB fault detail structure.
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

#ifndef _EMULATRAPPUNI_CORELIB_INFO_INL_HELPERS_H
#define _EMULATRAPPUNI_CORELIB_INFO_INL_HELPERS_H

#pragma once
#include <QtGlobal>
#include "enum_Reasons.h"
#include "enum_MCES.h"

#include <QString>

/**
 * @brief TLB fault detail structure.
 *
 * Captures comprehensive TLB miss and translation fault information.
 */
struct TLBFaultInfo {
	quint64 virtualAddress;           ///< Faulting virtual address
	quint64 vpn;                      ///< Virtual page number
	quint16 asn;                      ///< Address space number

	quint8 accessType;                ///< 0=READ, 1=WRITE, 2=EXEC, 3=RMW
	quint8 pageSize;                  ///< Page size (0=8KB, 1=64KB, etc.)

	bool isWrite;                     ///< Write access
	bool isInstruction;               ///< Instruction fetch
	bool isDTBMiss;                   ///< Data TLB (vs Instruction TLB)
	bool isPTEInvalid;                ///< PTE found but V bit = 0

	TLBFaultInfo() : virtualAddress(0), vpn(0), asn(0), accessType(0),
		pageSize(0), isWrite(false), isInstruction(false),
		isDTBMiss(false), isPTEInvalid(false) {
	}
};

/**
 * @brief Memory access fault detail.
 *
 * Captures information about memory access violations.
 */
struct MemoryAccessInfo {
	quint64 virtualAddress;           ///< Faulting virtual address
	quint64 physicalAddress;          ///< Physical address (if translated)
	quint64 pcAtFault;                ///< PC of faulting instruction
	quint8 accessType;                ///< 0=READ, 1=WRITE, 2=EXEC
	bool isAligned;                   ///< Was access properly aligned?

	MemoryAccessInfo() : virtualAddress(0), physicalAddress(0), pcAtFault(0),
		accessType(0), isAligned(true) {
	}
};



/**
 * @brief Atomic operation fault detail.
 *
 * Information about LL/SC and other atomic operation failures.
 */
struct AtomicOperationInfo {
	quint64 reservationAddress;       ///< Address of LL reservation
	quint64 instructionsBetween;      ///< Instructions between LL and SC
	bool reservationLost;             ///< Was reservation lost?
	bool cacheLineEvicted;            ///< Cache line evicted?

	AtomicOperationInfo() : reservationAddress(0), instructionsBetween(0),
		reservationLost(false), cacheLineEvicted(false) {
	}
};



struct ExceptionAdditional {

	quint64 machineCheckCode;   ///< Machine Check Code from PAL BUGCHK
};
/**
 * @brief Additional exception context information.
 *
 * Supplementary data that may be needed for exception handling.
 */
struct AdditionalInfo {
	quint16 asn;                      ///< Address Space Number
	quint64 pte;                      ///< Raw Page Table Entry
	quint64 originalInstruction;      ///< Original instruction encoding
	quint64 threadId;                 ///< Thread/context identifier
	quint64 virtualAddress;           ///< General-purpose virtual address
	quint64 timeStamp;                ///< Cycle count at exception


	TLBFaultInfo tlbFault;            ///< TLB-specific information
	MemoryAccessInfo memAccess;       ///< Memory access details
	AtomicOperationInfo atomicOp;     ///< Atomic operation details

	AdditionalInfo() : asn(0), pte(0), originalInstruction(0),
		threadId(0), virtualAddress(0), timeStamp(0) {
	}
};

/**
 * @brief Unified exception detail union.
 *
 * Contains sub-reason information for each exception category.
 */
union ExceptionDetail {
	AdditionalInfo  additionalInfo;
	MachineCheckReason machineCheck;
	MemoryFaultReason memoryFault;
	FloatingPointReason fpReason;
	ArithmeticReason arithmetic;
	PrivilegeViolationReason privilege;
	InterruptReason interrupt;
	EmulatorReason emulator;
	PowerManagementReason powerManagement;
	PerformanceReason performance;
	SoftwareTrapReason softwareTrapReason;
	ExceptionAdditional exceptionAddtional;
	quint64 rawCode;                  ///< Fallback for uncategorized

	ExceptionDetail() : rawCode(0) {}
};



// /**
//  * @brief Master exception information structure.
//  *
//  * Complete exception context passed to exception handlers.
//  *
//  * Example usage:
//  *     ExceptionInfo ex;
//  *     ex.type = ExceptionType::FAULT;
//  *     ex.category = ExceptionCategory::FLOATING_POINT;
//  *     ex.detail.fpReason = FloatingPointReason::OVERFLOW;
//  *     ex.setPcAtFault(currentPC);
//  *     ex.priority = 5;
//  *     cpu->dispatchException(ex);
//  */


#endif
