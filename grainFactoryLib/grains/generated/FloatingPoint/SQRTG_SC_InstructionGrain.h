// ============================================================================
// SQRTG_SC_InstructionGrain.h - SQRTG_SC Instruction Grain
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
//  Instruction: SQRTG_SC - SQRTG_SC SC
//  Opcode: 0x14, Function: 0x042A
//  Execution Box: FBox
//  Format: GF_OperateFormat
//  Latency: 70 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef SQRTG_SC_INSTRUCTIONGRAIN_H
#define SQRTG_SC_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "FBoxLib/FBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// SQRTG_SC Instruction Grain
// ============================================================================

class SQRTG_SC_InstructionGrain : public InstructionGrain
{
public:
    SQRTG_SC_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_OperateFormat,     // flags
            70,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("SQRTG_SC")
        , m_opcode(0x14)
        , m_functionCode(0x042A)
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
        slot.m_fBox->executeSQRTG_SC(slot);
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
    static GrainAutoRegistrar<SQRTG_SC_InstructionGrain> s_sqrtg_sc_registrar(
        0x14, 0x042A
    );
}

#endif // SQRTG_SC_INSTRUCTIONGRAIN_H
