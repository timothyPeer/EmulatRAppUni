#pragma once
#include "InstructionGrainRegistry.h"
#include "InstructionGrain.h"
#include "InstructionGrain_core.h"
#include "iGrain_helper_inl.h"

enum class InstrFormat {
	Operate,       // integer operate (non-MB)
	Memory,        // normal LD/ST/LDA/LDAH
	Branch,        // BR/BSR/BEQ/etc.
	Pal,           // CALL_PAL
	Float,         // FP arithmetic / FP compare
	MemoryMB,      // memory-with-function: FETCH/FETCH_M/WH64/MB/WMB/etc.
	Vec_Format,    // vector instructions (future use)
	JMP_JSR_Format, // // JMP, JSR, RET, JSR_COROUTINE
	Unknown
};

// ============================================================================
// GrainResolver - Decodes raw instructions and resolves to InstructionGrains
// ============================================================================
// This class bridges raw instruction bits to executable grain objects.
// It handles:
//   - Opcode and function code extraction
//   - Platform-specific grain lookup (VMS, Unix, NT, Pal_Internal)
//   - HW_INTERNAL grain differentiation (PAL19, PAL1B, PAL1D, PAL1E, PAL1F)
//   - Format classification for pipeline routing
// ============================================================================

class GrainResolver
{
public:
	static GrainResolver& instance()
	{
		static GrainResolver g;
		return g;
	}

	// ========================================================================
	// ResolveGrain - Main entry point
	// Takes raw 32-bit instruction, decodes it, returns matching grain
	// ========================================================================
	 InstructionGrain* ResolveGrain(quint32 rawInstruction) 
	{
		// Extract opcode (bits 26-31)
		const quint8 opcode = extractOpcode(rawInstruction);

		// Determine if this is a HW_INTERNAL instruction
		const bool isHwInternal = isHwInternalOpcode(opcode);

		// Extract function code based on format
		const quint16 func = extractFunctionCode(rawInstruction, opcode);

		// Determine lookup platform
		const GrainPlatform lookupPlatform = isHwInternal
			? GrainPlatform::Alpha
			: m_overridePlatform;

		// Look up the grain in registry
		InstructionGrain* grain = InstructionGrainRegistry::instance().lookup(
			opcode,
			func,
			lookupPlatform
		);

		return grain;  // Returns nullptr if not found
	}


	static AXP_HOT inline InstrFormat classifyFormat(quint8 opcode) noexcept
	 {
		 // =========================================================
		 // 0. PAL hardware instructions (Memory-like format, no function code)
		 // =========================================================
		 if (isHwInternalOpcode(opcode))    // 0x19, 0x1B, 0x1D, 0x1E, 0x1F
			 return InstrFormat::Memory;

		 // =========================================================
		 // 1. Floating point first: 0x14-0x17
		 // =========================================================
		 if (opcode >= 0x14 && opcode <= 0x17)
			 return InstrFormat::Float;

		 // =========================================================
		 // 2. PALcode
		 // =========================================================
		 if (opcode == 0x00)
			 return InstrFormat::Pal;

		 // =========================================================
		 // 3. Branch (0x30, 0x31)
		 // =========================================================
		 if (opcode >= 0x30 && opcode <= 0x3F)
			 return InstrFormat::Branch;

		 // =========================================================
		 // 4. Memory (LD/ST, LDx_L, STx_C, LDA/LDAH)
		 //    Opcodes 0x20-0x2F (inclusive)
		 // =========================================================
		 if (opcode >= 0x20 && opcode <= 0x2F)
			 return InstrFormat::Memory;

		 // =========================================================
		 // 5. Memory-with-FUNC (FETCH/FETCH_M/TRAPB/WH64/WMB/RC/RS/MB)
		 //    These are operate-format instructions using opcode 0x18.
		 // =========================================================
		 if (opcode == 0x18)
			 return InstrFormat::MemoryMB;

		 // JMP/JSR are also in this family, but opcode 0x1A, 0x1B
		 if (opcode == 0x1A)
			 return InstrFormat::JMP_JSR_Format;

		 if (opcode == 0x1B)
			 return InstrFormat::MemoryMB;

		 // =========================================================
		 // 6. Integer operate (all non-MB operate instructions)
		 // =========================================================
		 if ((opcode >= 0x10 && opcode <= 0x17) ||
			 opcode == 0x1C)
			 return InstrFormat::Operate;

		 // =========================================================
		 // 7. Vector instructions (optional layer)
		 // =========================================================
		 if (opcode >= 0x40 && opcode <= 0x7F)
			 return InstrFormat::Vec_Format;

		

		 // =========================================================
		 // 8. Unknown
		 // =========================================================
		 return InstrFormat::Unknown;
	 }

	// ========================================================================
	// Overload for compatibility with existing decode pipeline
	// ========================================================================
	 InstructionGrain* resolve(quint8 opcode, quint16 func) 
	{
		return InstructionGrainRegistry::instance().lookup(
			opcode,
			func,
			m_overridePlatform
		);
	}

	// ========================================================================
	// Platform management
	// ========================================================================
	void setPlatform(GrainPlatform p)
	{
		m_overridePlatform = p;
	}

	GrainPlatform getPlatform() const
	{
		return m_overridePlatform;
	}

	// ========================================================================
	// Helper: Get instruction format (uses existing classifyFormat)
	// ========================================================================
	InstrFormat getInstructionFormat(quint32 rawInstruction) const
	{
		const quint8 opcode = extractOpcode(rawInstruction);
		return classifyFormat(opcode);
	}

	// ========================================================================
	// Helper: Get GrainType from raw instruction
	// ========================================================================
	GrainType getGrainType(quint32 rawInstruction) const
	{
		const quint8 opcode = extractOpcode(rawInstruction);
		return classifyGrainType(opcode);
	}


	GrainPlatform m_overridePlatform{ GrainPlatform::Alpha };

	// ========================================================================
	// Extract opcode from instruction (bits 26-31)
	// ========================================================================
	static quint8 extractOpcode(quint32 instruction)
	{
		return static_cast<quint8>((instruction >> 26) & 0x3F);
	}

	// ========================================================================
	// Check if opcode is a HWBox reserved opcode
	// PAL19 (0x19), PAL1B (0x1B), PAL1D (0x1D), PAL1E (0x1E), PAL1F (0x1F)
	// ========================================================================
	static bool isHwInternalOpcode(quint8 opcode)
	{
		return (opcode == 0x19 || opcode == 0x1B || opcode == 0x1D ||
			opcode == 0x1E || opcode == 0x1F);
	}

	// ========================================================================
	// Extract function code based on instruction format
	// ========================================================================
	static quint16 extractFunctionCode(quint32 instruction, quint8 opcode)
	{
		const InstrFormat fmt = classifyFormat(opcode);

		switch (fmt)
		{
		case InstrFormat::Operate:
		case InstrFormat::MemoryMB:
			// Operate format and memory-with-function: bits 5-11 (7 bits)
			return static_cast<quint16>((instruction >> 5) & 0x7F);

		case InstrFormat::Float:
			// Floating-point: bits 5-15 (11 bits for FP function)
			return static_cast<quint16>((instruction >> 5) & 0x7FF);

		case InstrFormat::Pal:
			// PALcode: bits 0-25 (26-bit PAL function)
			return static_cast<quint16>(instruction & 0x3FFFFFF);

		case InstrFormat::Memory:
		case InstrFormat::Branch:
			// Memory and branch formats use displacement, not function code
			return 0;

		case InstrFormat::Vec_Format:
			// Vector instructions: bits 5-11 (similar to operate)
			return static_cast<quint16>((instruction >> 5) & 0x7F);

		case InstrFormat::JMP_JSR_Format:
			// Jump format (opcode 0x1A): bits [15:14] = subtype
			// 0=JMP, 1=JSR, 2=RET, 3=JSR_COROUTINE
			return static_cast<quint16>((instruction >> 14) & 0x3);
		default:
			return 0;
		}

        qDebug() << QString("Passed opcode: 0x%1 (decimal %2)")
                        .arg(opcode, 2, 16, QChar('0'))
                        .arg(opcode);
        quint8 opcode_q8 = (instruction >> 26) & 0x3F;
		qDebug() << QString("Extracted opcode: 0x%1 (decimal %2)")
            .arg(opcode_q8, 2, 16, QChar('0'))
            .arg(opcode_q8);

	}

	// ========================================================================
	// Classify instruction into GrainType
	// Maps InstrFormat to your GrainType enumeration
	// ========================================================================
	static GrainType classifyGrainType(quint8 opcode)
	{
		const InstrFormat fmt = classifyFormat(opcode);

		switch (fmt)
		{
		case InstrFormat::Operate:
			// Integer operate instructions
			return GrainType::IntegerOperate;

		case InstrFormat::Memory:
			// Integer or float memory based on opcode
			if (opcode >= 0x20 && opcode <= 0x23)  // LDF, LDG, LDS, LDT
				return GrainType::FloatMemory;
			if (opcode >= 0x24 && opcode <= 0x27)  // STF, STG, STS, STT
				return GrainType::FloatMemory;
			return GrainType::IntegerMemory;

		case InstrFormat::Branch:
			return GrainType::IntegerBranch;

		case InstrFormat::Pal:
			return GrainType::Pal;

		case InstrFormat::Float:
			return GrainType::FloatOperate;

		case InstrFormat::MemoryMB:
			// Check if it's actually a jump (0x1A, 0x1B)
			if (opcode == 0x1A)
				return GrainType::Jump;
			return GrainType::MemoryMB;

		case InstrFormat::Vec_Format:
			return GrainType::Vector;

		default:
			return GrainType::Unknown;
		}
	}
};
