// ============================================================================
// iGrain-DualLookup_inl.h - Optimized Decode Pipeline
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

// ============================================================================
// iGrain-DualLookup_inl.h - Optimized Decode Pipeline
// ============================================================================
// Integrates with 32-byte DecodedInstruction structure
// Uses optimized bitwise semantics and inline accessors
// ============================================================================

#ifndef IGRAIN_DUALLOOKUP_INL_H
#define IGRAIN_DUALLOOKUP_INL_H

#include "DecodedInstruction.h"
#include "InstructionGrain.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "grainFactoryLib/InstructionSemantics_mask.h"
#include "grainFactoryLib/GrainResolver.h"
#include <QtGlobal>
#include "coreLib/FetchResult.h"

// ============================================================================
// Register Decode - Direct Extraction
// ============================================================================


static AXP_HOT AXP_ALWAYS_INLINE void decodeRegisters(DecodedInstruction& di) noexcept
{
	// Alpha EV4/EV5/EV6 standard fields:
	//   RA: bits 25..21
	//   RB: bits 20..16
	//   RC: bits 4..0
	di.ra = (di.rawBits() >> 21) & 0x1F;
	di.rb = (di.rawBits() >> 16) & 0x1F;
	di.rc = (di.rawBits() & 0x1F);
}

// ============================================================================
// Literal Decode - Operate Format
// ============================================================================


static AXP_HOT AXP_ALWAYS_INLINE void decodeLiteral(DecodedInstruction& di) noexcept
{
	if (!di.grain || !isOperateFormat(di)) {
		di.literal_val = 0;
		
		return;
	}

	// Check L-bit (bit 12)
	const bool L = (di.rawBits() >> 12) & 1;
	if (L) {
		// Extract literal (bits 20..13)
		di.literal_val = static_cast<quint8>((di.rawBits() >> 13) & 0xFF);
		addSem(di.semantics, S_UsesLiteral);
	}
	else {
		di.literal_val = 0;
		
	}
}

// ============================================================================
// Branch Displacement Decode
// ============================================================================


static AXP_HOT AXP_ALWAYS_INLINE void decodeBranchDisp(DecodedInstruction& di) noexcept
{
    if (!di.grain || !isBranchFormat(di)) {
        di.branch_disp = 0;
        return;
    }

    // Extract 21-bit displacement (bits 20:0), sign-extend
    qint32 disp = static_cast<qint32>(di.rawBits() & 0x1FFFFF);
    di.branch_disp = (disp << 11) >> 11;  // Sign-extend from bit 20

    // Semantics already set in main decode - no need to set here
}


/**
 * @brief Set semantic flags (lower 32 bits) while preserving raw instruction (upper 32 bits)
 */
AXP_HOT AXP_ALWAYS_INLINE void setSemanticFlags(DecodedInstruction& di, InstrSemantics flags) noexcept
{
    quint64 semantics_test = di.semantics;
    quint32 flag_test = flags;
    di.semantics = (di.semantics & 0xFFFFFFFF00000000ULL) | static_cast<quint32>(flags);
}

/**
 * @brief Get semantic flags (lower 32 bits only)
 */
AXP_HOT AXP_ALWAYS_INLINE InstrSemantics getSemanticFlags(const DecodedInstruction& di) noexcept
{
    return static_cast<InstrSemantics>(di.semantics & 0xFFFFFFFFu);
}


// ============================================================================
// Main Decode Entry - Optimized Single-Pass Pipeline
// ============================================================================

static AXP_HOT AXP_ALWAYS_INLINE void decodeInstruction(DecodedInstruction& di, FetchResult& fetchResult) noexcept
{
    // Initialize to zero  (correct)
    di.pc = fetchResult.di.pc;
    di.ra = 31;
    di.rb = 31;
    di.rc = 31;
    di.memSize = 0;
    di.branch_disp = 0;
    di.literal_val = 0;
    di.semantics &= 0xFFFFFFFF00000000ULL;  //  ADD THIS LINE!

    InstrSemantics formatSem = S_None;  // Should be 0
    if (!di.grain) {
        fetchResult.valid = false;
        fetchResult.fetchStatus = MEM_STATUS::TargetMisDirect;
        ERROR_LOG(QString("Decode failed: grain is NULL"));
        return;
    }

    decodeRegisters(di);


    quint32 rawBits = di.rawBits();
    quint8 opcode = extractOpcode(rawBits);

  

    // ========================================================================
    // Determine format from OPCODE, not from di.semantics!
    // ========================================================================

    // Operate format: opcodes 0x10-0x13
    if (opcode >= 0x10 && opcode <= 0x13) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_OperFmt | S_IntFormat);

        const quint16 func = GrainResolver::extractFunctionCode(rawBits, opcode);
        if (opcode == 0x10 && (func & 0x40)) {
            formatSem = static_cast<InstrSemantics>(formatSem | S_OverflowTrap);
        }
    }

    // Memory format: opcodes 0x08-0x0F, 0x20-0x2F
    else if ((opcode >= 0x08 && opcode <= 0x0F) ||
        (opcode >= 0x20 && opcode <= 0x2F)) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_MemFmt);

        // Determine if load or store
        // Loads: 0x0A (LDBU), 0x0C (LDWU), 0x28-0x2B (LDL, LDQ, etc.)
        // Stores: 0x0E (STB), 0x0F (STW), 0x2C-0x2F (STL, STQ, etc.)
        if (opcode == 0x0A || opcode == 0x0C ||
            (opcode >= 0x20 && opcode <= 0x2B)) {
            formatSem = static_cast<InstrSemantics>(formatSem | S_Load);
        }
        else if (opcode == 0x0E || opcode == 0x0F ||
            (opcode >= 0x2C && opcode <= 0x2F)) {
            formatSem = static_cast<InstrSemantics>(formatSem | S_Store);
        }


    }

    // Branch format: opcodes 0x30-0x3F
    else if (opcode >= 0x30 && opcode <= 0x3F) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_BranchFmt | S_Branch | S_ChangesPC);

        // BSR is 0x34 - unconditional
        if (opcode == 0x30 || opcode == 0x34) {  // BR or BSR
            formatSem = static_cast<InstrSemantics>(formatSem | S_Uncond);
        }
        else {
            formatSem = static_cast<InstrSemantics>(formatSem | S_Cond);
        }

    }

    // PAL format: opcode 0x00
    else if (opcode == 0x00) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_PalFormat | S_Privileged);
    }

    // Jump format: opcode 0x1A
    else if (opcode == 0x1A) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_JumpFmt | S_ChangesPC);
    }

    // Float format: opcodes 0x14-0x17
    else if (opcode >= 0x14 && opcode <= 0x17) {
        formatSem = static_cast<InstrSemantics>(formatSem | S_FloatFormat);
    }
   
    setSemanticFlags(di, formatSem);
    

    // Now these checks work because semantics is populated
    if (isOperateFormat(di)) {
        decodeLiteral(di);
    }

    if (isBranchFormat(di)) {
        decodeBranchDisp(di);
    }

    if (isMemoryFormat(di)) {
        decodeMemSize(di);
    }

    if (isCallPal(di)) {
        const quint8 palFunction = rawBits & 0x7F;
        fetchResult.isCallPal = true;
        fetchResult.palFunction = palFunction;
    }

    if (!di.grain->eligibleForDualIssue() || di.needsStall()) {
        addSem(di.semantics, S_NeedsStall);
    }

    fetchResult.valid = true;
}


#endif // IGRAIN_DUALLOOKUP_INL_H
