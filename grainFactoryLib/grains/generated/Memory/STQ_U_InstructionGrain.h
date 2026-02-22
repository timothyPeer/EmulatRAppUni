// ============================================================================
// STQ_U_InstructionGrain.h - STQ_U Instruction Grain
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
//  Instruction: STQ_U - STQ_U
//  Opcode: 0x0F, Function: 0x0000
//  Execution Box: MBox
//  Format: GF_MemoryFormat
//  Latency: 1 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef STQ_U_INSTRUCTIONGRAIN_H
#define STQ_U_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "MBoxLib_EV6/MBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// STQ_U Instruction Grain
// ============================================================================

class STQ_U_InstructionGrain : public InstructionGrain
{
public:
    STQ_U_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_MemoryFormat,     // flags
            1,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("STQ_U")
        , m_opcode(0x0F)
        , m_functionCode(0x0000)
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
        slot.m_mBox->executeSTQ_U(slot);
    }

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {
        return ExecutionBox::MBox;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {
        return GrainType::Memory;
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
    static GrainAutoRegistrar<STQ_U_InstructionGrain> s_stq_u_registrar(
        0x0F, 0x0000
    );
}

#endif // STQ_U_INSTRUCTIONGRAIN_H
