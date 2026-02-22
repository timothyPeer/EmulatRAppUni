// ============================================================================
// RSCC_InstructionGrain.h - RSCC Instruction Grain
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
//
//  Instruction: RSCC - RSCC
//  Opcode: 0x00, Function: 0x009D
//  Execution Box: PalBox
//  Format: GF_PALFormat
//  Latency: 1 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef RSCC_INSTRUCTIONGRAIN_H
#define RSCC_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "palBoxLib/PalBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// RSCC Instruction Grain
// ============================================================================

class RSCC_InstructionGrain : public InstructionGrain
{
public:
    RSCC_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_PALFormat,     // flags
            1,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("RSCC")
        , m_opcode(0x00)
        , m_functionCode(0x009D)
        , m_platform(GrainPlatform::Alpha)
    {
    }

    // ========================================================================
    // Virtual Method Implementations
    // ========================================================================
    
    AXP_HOT AXP_ALWAYS_INLINE
    void execute(PipelineSlot& slot) const noexcept override
    {
        // Delegate to execution box via slot member
        slot.m_palBox->executeRSCC(slot);
    }

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {
        return ExecutionBox::PalBox;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {
        return GrainType::PALcode;
    }

    // ========================================================================
    // Pure Virtual Accessor Implementations
    // ========================================================================
    
    AXP_HOT AXP_ALWAYS_INLINE
    QString mnemonic() const noexcept override
    {
        return m_mnemonic;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    quint8 opcode() const noexcept override
    {
        return m_opcode;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    quint16 functionCode() const noexcept override
    {
        return m_functionCode;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainPlatform platform() const noexcept override
    {
        return m_platform;
    }

private:
    QString m_mnemonic;
    quint8 m_opcode;
    quint16 m_functionCode;
    GrainPlatform m_platform;
};

// ============================================================================
// Auto-registration
// ============================================================================

namespace {
    static GrainAutoRegistrar<RSCC_InstructionGrain> s_rscc_registrar(
        0x00, 0x009D
    );
}

#endif // RSCC_INSTRUCTIONGRAIN_H
