// ============================================================================
// memory_enums_structs.h - ============================================================================
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
// MEMORY FAULT TYPE - Detailed exception classification
// ============================================================================
// Map these to PAL vectors and OS reasons. Keep ITB vs DTB, read vs write.
// (AAH Vol. I: Exceptions & Interrupts;  Memory Mgmt; TB miss/fault vectors)
//
enum class MemoryFaultType : quint32
{
	NONE = 0,                      // No fault

	// ------------------------------------------------------------------------
	// Translation Buffer / Page Table events
	// ------------------------------------------------------------------------
	ITB_MISS,                      // Instruction TB miss                         //  ITB miss vector
	//DTB_MISS_READ,                 // Data TB miss on read                        //  DTB miss vector (read)
	//DTB_MISS_WRITE,                // Data TB miss on write (incl. FOW/dirty)     //  DTB miss vector (write)

	ITB_FAULT,                     // ITB fault (e.g., translation invalid)       //  ITB fault vector

	// ========================================================================
	// Data Translation Buffer (DTB) Faults - Split by Access Type
	// ========================================================================
	DTB_MISS_READ,                 // DTB miss on read operation
	DTB_MISS_WRITE,                // DTB miss on write operation
	DTB_FAULT_READ,                // DTB fault on read operation (invalid PTE)
	DTB_FAULT_WRITE,               // DTB fault on write operation (invalid PTE)
	DTB_ACCESS_VIOLATION_READ,     // DTB ACV on read (no read permission)
	DTB_ACCESS_VIOLATION_WRITE,    // DTB ACV on write (no write permission)
	ITB_ACCESS_VIOLATION,          // ITB access violation - no execute permission


	// ========================================================================
	// Page Management
	// ========================================================================
	PAGE_NOT_PRESENT,              // Page not present in physical memory
	FAULT_ON_WRITE,                // Fault-on-Write (FOW bit - copy-on-write) PTE FOW/modify/dirty rules
	FAULT_ON_READ,                 // Fault-on-Read (FOR bit - demand paging)
	FAULT_ON_EXECUTE,              // Fault-on-Execute (FOE bit - code page-in)
	DOUBLE_FAULT,                  // Fault during fault handling - Conventional escalation

	// ------------------------------------------------------------------------
	// Address validity / bounds
	// ------------------------------------------------------------------------
	INVALID_ADDRESS,               // Address not canonical/implemented           //  VA format / region checks
	OUT_OF_BOUNDS,                 // Exceeds configured RAM / device aperture

	// ------------------------------------------------------------------------
	// Alignment
	// ------------------------------------------------------------------------
	ALIGNMENT_FAULT,               // Unaligned access (int/FP/stack)             //  Unaligned access trap
	INVALID_SIZE,                  // Bad transfer size for operation

	// ------------------------------------------------------------------------
	// Protection / Privilege
	// ------------------------------------------------------------------------
	ACCESS_VIOLATION,              // Generic access violation                     //  ACCVIO result to OS
	PROTECTION_VIOLATION,          // PTE permission denied (mode vs R/W)         //  K/E/S/U masks
	PRIVILEGE_VIOLATION,           // Requires higher mode (e.g., PAL/K mode)     //  Privilege rules
	// Emulator policy
	  //INSTRUCTION_FETCH_VIOLATION,   // Fetch denied (policy); ITB-side synthesis

	  // ------------------------------------------------------------------------
	  // Arithmetic / FP / Enable
	  // ------------------------------------------------------------------------
	ARITHMETIC_TRAP,               // See ArithmeticTrapKind                       // FP exceptions & FPCR
	FEN_FAULT,                     // FP instruction while FP disabled (FEN)       // FEN trap

	// ------------------------------------------------------------------------
	// Opcode / Decode
	// ------------------------------------------------------------------------
	OPCODE_FAULT,                  // Invalid/reserved opcode or illegal PAL - OPCDEC/RESOP/ILLPAL

	// ------------------------------------------------------------------------
	// Software events (surface only if you normalize CALL_PAL->faults)
	// ------------------------------------------------------------------------
	BREAKPOINT,                    // BPT                                          // BPT vector
	BUGCHECK,                      // BUGCHK                                       // BUGCHK vector
	SOFTWARE_TRAP,                 // CHMx / SYS calls surfaced as trap (optional) // CALL_PAL CHMx
	GENTRAP,                       // GENTRAP instruction (software trap with code)
	// User-generated trap with arbitrary code (GENTRAP instruction)

// ========================================================================
// Instruction Fetch Violations
// ========================================================================

INSTRUCTION_FETCH_VIOLATION,   // Instruction fetch from protected page
EXECUTION_VIOLATION,           // Execute from non-executable page
//EXECUTION_VIOLATION,           // If NX modeled; otherwise fold into ACCESS 

// ------------------------------------------------------------------------
// I/O and Device
// ------------------------------------------------------------------------
MMIO_ERROR,                    // MMIO device reports error
DEVICE_NOT_FOUND,              // No device mapped at PA
DEVICE_ERROR,                  // Device-specific error
DEVICE_TIMEOUT,                // No response within budget
SIZE_VIOLATION,                // Wrong width to an I/O register
READ_ERROR,
WRITE_ERROR,

// ------------------------------------------------------------------------
// System / Hardware
// ------------------------------------------------------------------------
MACHINE_CHECK,                 // Machine check                                //  Machine check vector
POWER_FAIL,                    // Power fail / low condition (if modeled)
RESET_OCCURRED,                // Reset detected during op (if modeled)
HARDWARE_ERROR,                // Unclassified HW error
GENERAL_PROTECTION_FAULT,      // Catch-all (keep rare)
};



