// ============================================================================
// Tlb_Fault_core.h - ============================================================================
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

// ============================================================================
// TLB and PTE Fault Types for Alpha AXP Architecture
// ============================================================================
// These fault types represent various translation failures that can occur
// during virtual-to-physical address translation in the TLB/MMU subsystem.
// ============================================================================

enum class TLBFaultType : quint8
{
	// No fault - translation succeeded
	NO_FAULT = 0,

	// ========================================================================
	// TLB Miss Faults
	// ========================================================================
	ITB_MISS,               // Instruction TLB miss
	DTB_MISS_LOAD,          // Data TLB miss on load operation
	DTB_MISS_STORE,         // Data TLB miss on store operation

	// ========================================================================
	// Access Violation Faults (Permission Failures)
	// ========================================================================
	ITB_ACV,                // Instruction TLB access control violation
	DTB_ACV_READ,           // Data TLB access violation on read
	DTB_ACV_WRITE,          // Data TLB access violation on write
	DTB_ACV_EXECUTE,        // Data TLB access violation on execute attempt

	// ========================================================================
	// Page Faults (Present bit clear or invalid PTE)
	// ========================================================================
	ITB_PAGE_FAULT,         // Instruction page not present
	DTB_PAGE_FAULT_READ,    // Data page not present (read)
	DTB_PAGE_FAULT_WRITE,   // Data page not present (write)

	// ========================================================================
	// Protection Faults
	// ========================================================================
	FOE_FAULT,              // Fault On Execute - execute not allowed
	FOR_FAULT,              // Fault On Read - read not allowed
	FOW_FAULT,              // Fault On Write - write not allowed (copy-on-write)

	// ========================================================================
	// Alignment and Format Faults
	// ========================================================================
	UNALIGNED_ACCESS,       // Unaligned memory access
	INVALID_PTE_FORMAT,     // PTE format is invalid or corrupt

	// ========================================================================
	// ASN (Address Space) Faults
	// ========================================================================
	ASN_MISMATCH,           // ASN in TLB doesn't match current ASN
	INVALID_ASN,            // ASN value is out of valid range

	// ========================================================================
	// Virtual Address Range Faults
	// ========================================================================
	VA_OUT_OF_RANGE,        // Virtual address exceeds architectural limits
	VA_HOLE_ACCESS,         // Access to non-canonical VA (address hole)

	// ========================================================================
	// Page Table Walk Faults
	// ========================================================================
	L1_PTE_INVALID,         // Level 1 PTE invalid during walk
	L2_PTE_INVALID,         // Level 2 PTE invalid during walk
	L3_PTE_INVALID,         // Level 3 PTE invalid during walk
	PAGE_TABLE_FAULT,       // General page table walk failure

	// ========================================================================
	// Hardware/System Faults
	// ========================================================================
	DOUBLE_FAULT,           // Fault occurred while handling another fault
	MACHINE_CHECK,          // Hardware error during translation
	TNV_FAULT,              // Translation Not Valid

	// ========================================================================
	// Special Cases
	// ========================================================================
	RESERVED_OPERAND,       // Reserved addressing mode or operand
	PRIVILEGE_VIOLATION,    // Privilege level insufficient for access

	// Sentinel value
	FAULT_TYPE_MAX
};

// ============================================================================
// Fault Information Structure
// ============================================================================
struct TLBFaultInfo
{
	TLBFaultType type{ TLBFaultType::NO_FAULT };
	quint64 faultVA{ 0 };           // Faulting virtual address
	quint64 faultPC{ 0 };           // PC where fault occurred
	ASNType faultASN{ 0 };          // ASN at time of fault
	quint8  mode{ 0 };              // Privilege mode (user/kernel/etc)
	bool    isWrite{ false };       // Was this a write access?
	bool    isExecute{ false };     // Was this an execute access?

	// Clear fault info
	inline void clear() noexcept
	{
		type = TLBFaultType::NO_FAULT;
		faultVA = 0;
		faultPC = 0;
		faultASN = 0;
		mode = 0;
		isWrite = false;
		isExecute = false;
	}

	// Check if there's an active fault
	inline bool hasFault() const noexcept
	{
		return type != TLBFaultType::NO_FAULT;
	}
};

// ============================================================================
// Helper Functions
// ============================================================================

// Convert fault type to string for debugging
inline const char* tlbFaultTypeToString(TLBFaultType type) noexcept
{
	switch (type) {
	case TLBFaultType::NO_FAULT:            return "NO_FAULT";
	case TLBFaultType::ITB_MISS:            return "ITB_MISS";
	case TLBFaultType::DTB_MISS_LOAD:       return "DTB_MISS_LOAD";
	case TLBFaultType::DTB_MISS_STORE:      return "DTB_MISS_STORE";
	case TLBFaultType::ITB_ACV:             return "ITB_ACV";
	case TLBFaultType::DTB_ACV_READ:        return "DTB_ACV_READ";
	case TLBFaultType::DTB_ACV_WRITE:       return "DTB_ACV_WRITE";
	case TLBFaultType::DTB_ACV_EXECUTE:     return "DTB_ACV_EXECUTE";
	case TLBFaultType::ITB_PAGE_FAULT:      return "ITB_PAGE_FAULT";
	case TLBFaultType::DTB_PAGE_FAULT_READ: return "DTB_PAGE_FAULT_READ";
	case TLBFaultType::DTB_PAGE_FAULT_WRITE:return "DTB_PAGE_FAULT_WRITE";
	case TLBFaultType::FOE_FAULT:           return "FOE_FAULT";
	case TLBFaultType::FOR_FAULT:           return "FOR_FAULT";
	case TLBFaultType::FOW_FAULT:           return "FOW_FAULT";
	case TLBFaultType::UNALIGNED_ACCESS:    return "UNALIGNED_ACCESS";
	case TLBFaultType::INVALID_PTE_FORMAT:  return "INVALID_PTE_FORMAT";
	case TLBFaultType::ASN_MISMATCH:        return "ASN_MISMATCH";
	case TLBFaultType::INVALID_ASN:         return "INVALID_ASN";
	case TLBFaultType::VA_OUT_OF_RANGE:     return "VA_OUT_OF_RANGE";
	case TLBFaultType::VA_HOLE_ACCESS:      return "VA_HOLE_ACCESS";
	case TLBFaultType::L1_PTE_INVALID:      return "L1_PTE_INVALID";
	case TLBFaultType::L2_PTE_INVALID:      return "L2_PTE_INVALID";
	case TLBFaultType::L3_PTE_INVALID:      return "L3_PTE_INVALID";
	case TLBFaultType::PAGE_TABLE_FAULT:    return "PAGE_TABLE_FAULT";
	case TLBFaultType::DOUBLE_FAULT:        return "DOUBLE_FAULT";
	case TLBFaultType::MACHINE_CHECK:       return "MACHINE_CHECK";
	case TLBFaultType::TNV_FAULT:           return "TNV_FAULT";
	case TLBFaultType::RESERVED_OPERAND:    return "RESERVED_OPERAND";
	case TLBFaultType::PRIVILEGE_VIOLATION: return "PRIVILEGE_VIOLATION";
	default:                                 return "UNKNOWN_FAULT";
	}
}

// Check if fault requires PALcode handler
inline bool requiresPALHandler(TLBFaultType type) noexcept
{
	switch (type) {
	case TLBFaultType::ITB_MISS:
	case TLBFaultType::DTB_MISS_LOAD:
	case TLBFaultType::DTB_MISS_STORE:
	case TLBFaultType::ITB_ACV:
	case TLBFaultType::DTB_ACV_READ:
	case TLBFaultType::DTB_ACV_WRITE:
	case TLBFaultType::FOW_FAULT:
		return true;
	default:
		return false;
	}
}

// Check if fault is recoverable
inline bool isRecoverableFault(TLBFaultType type) noexcept
{
	switch (type) {
	case TLBFaultType::ITB_MISS:
	case TLBFaultType::DTB_MISS_LOAD:
	case TLBFaultType::DTB_MISS_STORE:
	case TLBFaultType::FOW_FAULT:
	case TLBFaultType::ITB_PAGE_FAULT:
	case TLBFaultType::DTB_PAGE_FAULT_READ:
	case TLBFaultType::DTB_PAGE_FAULT_WRITE:
		return true;
	default:
		return false;
	}
}