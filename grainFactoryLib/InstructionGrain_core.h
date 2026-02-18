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