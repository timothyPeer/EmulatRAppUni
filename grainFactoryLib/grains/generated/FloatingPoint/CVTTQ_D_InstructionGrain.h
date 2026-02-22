// ============================================================================
// CVTTQ_D_InstructionGrain.h - CVTTQ_D Instruction Grain
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
//  Instruction: CVTTQ_D - CVTTQ_D D
//  Opcode: 0x16, Function: 0x00EF
//  Execution Box: FBox
//  Format: GF_OperateFormat
//  Latency: 6 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef CVTTQ_D_INSTRUCTIONGRAIN_H
#define CVTTQ_D_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "FBoxLib/FBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// CVTTQ_D Instruction Grain
// ============================================================================

class CVTTQ_D_InstructionGrain : public InstructionGrain
{
public:
    CVTTQ_D_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_OperateFormat,     // flags
            6,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("CVTTQ_D")
        , m_opcode(0x16)
        , m_functionCode(0x00EF)
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
        slot.m_fBox->executeCVTTQ_D(slot);
    }

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {
        return ExecutionBox::FBox;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {
        return GrainType::FloatingPoint;
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
    static GrainAutoRegistrar<CVTTQ_D_InstructionGrain> s_cvttq_d_registrar(
        0x16, 0x00EF
    );
}

#endif // CVTTQ_D_INSTRUCTIONGRAIN_H
