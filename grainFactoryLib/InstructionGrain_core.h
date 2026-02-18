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
	NONE = 0,        // Platform-independent (default)
	Alpha = 1,       // Generic Alpha AXP
	VMS = 2,         // OpenVMS Alpha
	Unix = 3,        // Digital UNIX / Tru64
	NT = 4,          // Windows NT Alpha
	Pal_Internal = 5 // Hardware internal (PAL19, PAL1B, PAL1D, PAL1E, PAL1F)
};