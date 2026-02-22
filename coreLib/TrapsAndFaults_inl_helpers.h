// ============================================================================
// TrapsAndFaults_inl_helpers.h - TLB fault detail structure.
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

#pragma once
#include <QtGlobal>

enum class FaultCause : quint8 {
	FAULT_UNKNOWN,
	FAULT_NONE,
	TLB_MISS_DTLB,
	TLB_MISS_ITLB,
	TLB_ACCESS_VIOLATION,
	TLB_MOD_FAULT,
	PAGE_PROTECTION,
	PAGE_DIRTY,
	UNALIGNED_LOAD,
	UNALIGNED_STORE,
	UNALIGNED_INSTRUCTION,
	FP_INVALID_OP,
	FP_DENORMAL,
	PAGE_NOT_PRESENT,
	INTEGER_OVERFLOW,
	INTEGER_DIVIDE_BY_ZERO,
	FP_OVERFLOW,
	FP_UNDERFLOW,
	FP_INEXACT,
	FP_DIVIDE_BY_ZERO,
	FEN_DISABLED,
	SYSTEM_RESET,
	ILLEGAL_OPCODE,
	ILLEGAL_OPERAND,
	PRIVILEGED_INSTRUCTION,
	MACHINE_CHECK,
	BUGCHECK,
	BREAKPOINT,
	GENTRAP,
	SOFTWARE_TRAP
};


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

