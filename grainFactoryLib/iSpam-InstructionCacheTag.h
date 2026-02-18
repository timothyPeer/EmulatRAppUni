#pragma once
#include <QtGlobal>
#include "types_core.h"
#include "iGrain-Decode-Meta.h"

// ============================================================================
// Optional: Shard by execution box for even better locality
// ============================================================================

enum class ShardClass : quint8 {
	INTEGER_ALU = 0,    // ADDL, SUBL, AND, OR, XOR, etc.
	BRANCH = 1,         // BR, BSR, JMP, JSR
	LOAD_STORE = 2,     // LDQ, STQ, LDA, etc.
	FLOATING_POINT = 3  // ADDT, MULT, DIVT, etc.
};

// ============================================================================
// Instruction Cache Tag - Composite (PC, PA) key
// ============================================================================
// Now integer instructions naturally cluster in different buckets
// than FP instructions, improving cache locality per execution unit
struct ISpamTag {
	quint64 pc;
	quint64 pa;
	quint8 opcode;
	quint8 logFunction;
	ShardClass boxClass;  // - Add this

	[[nodiscard]] constexpr quint64 hash() const noexcept {
		quint64 x = pc ^ (pa << 7)
			^ (static_cast<quint64>(opcode) << 14)
			^ (static_cast<quint64>(logFunction) << 20)
			^ (static_cast<quint64>(boxClass) << 26);  // - Include box class
		// ... avalanche ...
		return x;
	}
};

// ============================================================================
// Decoded Instruction Grain
// ============================================================================

// enum class ExecutionBox : quint8 {
// 	IBox = 0,  // Instruction fetch/issue
// 	EBox = 1,  // Integer ALU
// 	FBox = 2,  // Floating-point
// 	MBox = 3   // Memory/Load-Store
// };

struct InstructionGrain_iSpam {
	quint32 rawInstruction;     // Raw 32-bit Alpha instruction
	ExecutionBox targetBox;     // Which execution box handles this

	// Decoded fields (cache the decode work)
	quint8 opcode;
	quint8 ra, rb, rc;          // Register operands
	quint16 logFunction;           // Function code for operate format
	qint16 displacement;        // Branch/memory displacement

	// Execution metadata
	bool isBranch : 1;
	bool isLoad : 1;
	bool isStore : 1;
	bool isFP : 1;
	bool isPALcode : 1;
	bool isPrivileged : 1;

	// Performance hints
	quint8 latency;             // Predicted latency cycles
	quint8 pipelineStage;       // Which pipeline stage
};

// ============================================================================
// ISPAM Entry - Cached instruction grain
// ============================================================================

struct ISpamEntry {
	ISpamTag tag;
	InstructionGrain_iSpam grain;

	// Cache metadata
	quint16 generation;         // For invalidation
	bool valid : 1;
	bool locked : 1;            // Pin hot instructions
	bool transitioning : 1;

	quint8 accessCount;         // Promotion/eviction hint

	[[nodiscard]] constexpr bool isValid() const noexcept {
		return valid && !transitioning;
	}
};
