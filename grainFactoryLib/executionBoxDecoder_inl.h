// ============================================================================
// executionBoxDecoder_inl.h - ============================================================================
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

#ifndef EXECUTIONBOXDECODER_INL_H
#define EXECUTIONBOXDECODER_INL_H

#include "coreLib/Axp_Attributes_core.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"

// ============================================================================
// Execution Box - Pipeline routing
// ============================================================================
enum class ExecutionBox : quint8 {
    IBox,       // Integer box (integer ALU, branches)
    EBox,       // Integer execution unit (subset of IBox)
    FBox,       // Floating-point box
    MBox,       // Address/memory box (loads, stores)
    CBox,       // Control box (PALcode, traps)
    HWBox,		// Hardware internal (HW_MFPR, HW_MTPR, etc.)
    VBox,       // Vector box (future)
    IBoxOnly,   // IBox-only operations (traps, etc.)
    PalBox,
    Unknown
};

AXP_HOT AXP_ALWAYS_INLINE ExecutionBox executionBoxDecoder(const quint16 opCode) noexcept
{
    switch (opCode)
    {
        // === Memory instructions ==========================================
        // Opcodes: 0x28–0x2F (LDx), 0x20–0x27 (STx)
        // Ref: Alpha AXP Architecture Vol II-A, Table 4-4.
    case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2A: case 0x2B:
    case 0x2C: case 0x2D: case 0x2E: case 0x2F:
        return ExecutionBox::MBox;

        // === Branch instructions ===========================================
        // Opcodes: 0x30–0x3F
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3A: case 0x3B:
    case 0x3C: case 0x3D: case 0x3E: case 0x3F:
        return ExecutionBox::IBox;  // Or EBox if branches are executed there

        // === Integer operate ===============================================
        // Opcode: 0x10, 0x11, 0x12, 0x13
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
        return ExecutionBox::EBox;

        // === Floating point operate ========================================
        // Opcodes: 0x14, 0x16
    case 0x14:
    case 0x16:
        return ExecutionBox::FBox;

        // === CALL_PAL / PALcode Format =====================================
        // Opcode: 0x00
    case 0x00:
        return ExecutionBox::MBox; // Always PAL -> MBox

        // === Memory barrier & control instructions ==========================
        // Opcode: 0x18
    case 0x18:
        return ExecutionBox::MBox;

    default:
        return ExecutionBox::Unknown;
    }
}



#endif // EXECUTIONBOXDECODER_INL_H
