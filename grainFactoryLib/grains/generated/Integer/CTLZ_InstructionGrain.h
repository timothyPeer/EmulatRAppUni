// ============================================================================
// CTLZ_InstructionGrain.h - CTLZ Instruction Grain
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
//  Instruction: CTLZ - CTLZ
//  Opcode: 0x1C, Function: 0x32
//  Execution Box: EBox
//  Format: GF_OperateFormat
//  Latency: 6 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef CTLZ_INSTRUCTIONGRAIN_H
#define CTLZ_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "EBoxLib/EBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// CTLZ Instruction Grain
// ============================================================================

class CTLZ_InstructionGrain : public InstructionGrain
{
public:
    CTLZ_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_OperateFormat,     // flags
            6,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("CTLZ")
        , m_opcode(0x1C)
        , m_functionCode(0x32)
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
        slot.m_eBox->executeCTLZ(slot);
    }

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {
        return ExecutionBox::EBox;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {
        return GrainType::IntegerOperate;
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
    static GrainAutoRegistrar<CTLZ_InstructionGrain> s_ctlz_registrar(
        0x1C, 0x32
    );
}

#endif // CTLZ_INSTRUCTIONGRAIN_H
