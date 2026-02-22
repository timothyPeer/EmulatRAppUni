// ============================================================================
// CVTGD_UC_InstructionGrain.h - CVTGD_UC Instruction Grain
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
//  Instruction: CVTGD_UC - CVTGD_UC UC
//  Opcode: 0x15, Function: 0x012D
//  Execution Box: FBox
//  Format: GF_OperateFormat
//  Latency: 6 cycles, Throughput: 1/cycle
//
//  Generated: 2026-02-18 12:45:23
//
// ============================================================================

#ifndef CVTGD_UC_INSTRUCTIONGRAIN_H
#define CVTGD_UC_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "FBoxLib/FBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// CVTGD_UC Instruction Grain
// ============================================================================

class CVTGD_UC_InstructionGrain : public InstructionGrain
{
public:
    CVTGD_UC_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_OperateFormat,     // flags
            6,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("CVTGD_UC")
        , m_opcode(0x15)
        , m_functionCode(0x012D)
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
        slot.m_fBox->executeCVTGD_UC(slot);
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
    static GrainAutoRegistrar<CVTGD_UC_InstructionGrain> s_cvtgd_uc_registrar(
        0x15, 0x012D
    );
}

#endif // CVTGD_UC_INSTRUCTIONGRAIN_H
