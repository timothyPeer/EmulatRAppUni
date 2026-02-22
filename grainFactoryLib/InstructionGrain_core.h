// ============================================================================
// InstructionGrain_core.h - ============================================================================
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
// Grain Type - Instruction classification
// ============================================================================
enum class GrainType : quint8 {
	IntegerOperate,   // Integer ALU operations
	IntegerMemory,    // Integer loads/stores
	IntegerBranch,    // Branch instructions
	FloatOperate,     // Floating-point operations (deprecated)
	FloatingPoint,    // Floating-point operations
	FloatMemory,      // Floating-point loads/stores
	Pal,              // PALcode instructions
	PALcode,
	Jump,             // Jump instructions (JMP, JSR, RET, JSR_COROUTINE)
	Branch,
	ControlFlow,
	MemoryMB,         // Memory barrier operations
	Vector,           // Vector instructions (future)
	MemoryBarrier,	  // Memory Barrier
	Memory,
	Miscellaneous,
	Unknown
};

// ============================================================================
// Grain Platform - PAL/OS variant identification
// ============================================================================
enum class GrainPlatform : quint8 {
	
	Alpha = 0x01,       // Generic Alpha AXP

};