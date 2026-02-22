// ============================================================================
// PalArgumentPack_str.h - ============================================================================
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

#ifndef PalArgumentPack_str_h__
#define PalArgumentPack_str_h__
#include <QtGlobal>
#include "../faultLib/PendingEvent_Refined.h"


// ============================================================================
// Build MMCSR (Memory Management Control/Status Register)
// ============================================================================
// MMCSR bit layout (EV6):
//   [0]    = FOW (Fault on Write)
//   [1]    = FOR (Fault on Read)
//   [2]    = FOE (Fault on Execute)
//   [3]    = ACV (Access Violation)
//   [4]    = WR  (Write access - 0=read, 1=write)
//   [5-7]  = Reserved
// ============================================================================
inline quint64 buildMMCSR(const PendingEvent& ev) noexcept
{
	quint64 mmcsr = 0;

	// Fault type bits (from your PendingPropertyInfo)
	if (ev.pendingEvent_Info.isFaultOnWrite)      mmcsr |= (1 << 0);
	if (ev.pendingEvent_Info.isFaultOnRead)       mmcsr |= (1 << 1);
	if (ev.pendingEvent_Info.isFaultOnExecute)    mmcsr |= (1 << 2);
	if (ev.pendingEvent_Info.isAccessViolation)   mmcsr |= (1 << 3);

	// Access type bit
	if (ev.pendingEvent_Info.isWrite)             mmcsr |= (1 << 4);

	// Include mmAccessType and mmFaultReason if needed
	// (Alpha architecture specific - adapt as needed)
	mmcsr |= (ev.mmAccessType & 0x7) << 8;   // Bits [10:8]
	mmcsr |= (ev.mmFaultReason & 0xF) << 12; // Bits [15:12]

	return mmcsr;
}

// ============================================================================
// PalArgumentPack
// ============================================================================
// Standard argument passing for Alpha PAL entrypoints.
//
// REGISTER MAPPING (OS PAL Convention):
//   a0 = R16 = arg0
//   a1 = R17 = arg1
//   a2 = R18 = arg2
//   a3 = R19 = arg3
//   a4 = R20 = arg4
//   a5 = R21 = arg5
//
// COMMON PAL ENTRY ARGUMENT ASSIGNMENTS:
//
// Exception/Fault Entrypoints (DTB_MISS, ITB_MISS, DFAULT, etc.):
//   a0 = Faulting virtual address (VA)
//   a1 = Memory management fault code (MMCSR equivalent)
//   a2 = Faulting PC (address of instruction that caused fault)
//   a3 = Exception-specific info (varies by vector)
//   a4 = Reserved / vector-specific
//   a5 = Reserved / vector-specific
//
// DTB_MISS_SINGLE / DTB_MISS_DOUBLE:
//   a0 = Faulting VA (virtual address that missed in TLB)
//   a1 = MMCSR (bits indicate read/write, user/kernel mode)
//   a2 = Faulting PC
//   a3 = Reserved
//
// ITB_MISS:
//   a0 = Faulting VA (= faulting PC for instruction fetch)
//   a1 = MMCSR
//   a2 = Faulting PC (same as a0 for ITB)
//   a3 = Reserved
//
// DFAULT (Data Fault - ACV, FOW, FOR):
//   a0 = Faulting VA
//   a1 = MMCSR (includes fault type: ACV, FOW, FOR)
//   a2 = Faulting PC
//   a3 = Fault-specific info
//
// UNALIGN:
//   a0 = Unaligned VA
//   a1 = Opcode of faulting instruction
//   a2 = Faulting PC
//   a3 = Destination register (Ra for loads)
//
// OPCDEC (Illegal Instruction):
//   a0 = Reserved
//   a1 = Illegal instruction word
//   a2 = Faulting PC
//   a3 = Reserved
//
// ARITH (Arithmetic Trap):
//   a0 = Trap summary (exc_sum)
//   a1 = Trap register mask
//   a2 = Faulting PC
//   a3 = Reserved
//
// FEN (Floating-Point Disabled):
//   a0 = Reserved
//   a1 = Reserved
//   a2 = Faulting PC
//   a3 = Reserved
//
// INTERRUPT:
//   a0 = Interrupt summary register
//   a1 = Interrupt vector
//   a2 = Interrupted PC
//   a3 = Reserved
//
// MCHK (Machine Check):
//   a0 = Machine check error summary
//   a1 = Logout frame pointer
//   a2 = Interrupted PC
//   a3 = Reserved
//
// CALL_PAL (Unprivileged):
//   a0-a5 = User-defined (application ABI)
//   Arguments passed through from user code in R16-R21
//
// CALL_PAL (Privileged):
//   a0-a5 = OS-defined (varies by PAL function)
//   Common privileged calls:
//     SWPPAL: a0 = new PAL base address
//     WRENT:  a0 = entrypoint address, a1 = entry type
//     WTINT:  a0 = interrupt enable mask
//
// ============================================================================
struct PalArgumentPack final
{
	// ========================================================================
	// Standard Argument Registers (R16-R21)
	// ========================================================================
	quint64 a0{ 0 };  // R16 - Primary argument (typically faulting VA or status)
	quint64 a1{ 0 };  // R17 - Secondary argument (typically MMCSR or fault code)
	quint64 a2{ 0 };  // R18 - Tertiary argument (typically faulting PC)
	quint64 a3{ 0 };  // R19 - Quaternary argument (vector-specific)
	quint64 a4{ 0 };  // R20 - Quinary argument (vector-specific)
	quint64 a5{ 0 };  // R21 - Senary argument (vector-specific)

	// ========================================================================
	// PAL Mode State
	// ========================================================================
	quint32 ipl{ 0 }; // Interrupt Priority Level when in PAL mode
	// Controls which interrupts can be delivered
	// IPL 0 = all interrupts enabled
	// IPL 7 = all maskable interrupts blocked

// ========================================================================
// PAL Vector Metadata
// ========================================================================
	PalVectorEntry* PalOffset{ nullptr };
	// Pointer to resolved PAL vector entry
	// Contains:
	//   - entryPC: PAL handler entry address
	//   - vectorId: Which PAL vector triggered
	//   - flags: Vector-specific attributes
};

// ============================================================================
// HELPER: Populate PalArgumentPack for Exception
// ============================================================================
inline void populateExceptionArgs(
	PalArgumentPack& pack,
	const PendingEvent& ev) noexcept
{
	// Common pattern for most exceptions:
	pack.a0 = ev.faultVA;        // Faulting virtual address
	pack.a1 = buildMMCSR(ev);    // Memory management fault code
	pack.a2 = ev.faultPC;        // PC of faulting instruction
	pack.a3 = 0;                 // Vector-specific (usually reserved)
	pack.a4 = 0;
	pack.a5 = 0;
}

#endif // PalArgumentPack_str_h__
