// ============================================================================
// HW_ST_InstructionGrain.h - HW_ST - Hardware Store
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025, 2026 eNVy Systems, Inc. All rights reserved.
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
//  Instruction: HW_ST - Hardware Store
//  Opcode: 0x1B, Function: 0x0000
//  Execution Box: PalBox
//  Format: GF_PALFormat
//  Latency: 1 cycles, Throughput: 1/cycle
//
//  Generated: 2026-03-05 19:26:54
//
// ============================================================================

#ifndef HW_ST_INSTRUCTIONGRAIN_H
#define HW_ST_INSTRUCTIONGRAIN_H

#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/cpuTrace.h"
#include "palBoxLib/PalBoxBase.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"

// ============================================================================
// HW_ST Instruction Grain
// ============================================================================

class HW_ST_InstructionGrain : public InstructionGrain
{
public:
    HW_ST_InstructionGrain()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            GF_PALFormat,     // flags
            1,   // latency (cycles)
            1 // throughput (instructions/cycle)
          )
        , m_mnemonic("HW_ST")
        , m_opcode(0x1B)
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

        slot.m_palBox->executeHW_ST(slot);
#ifdef AXP_EXEC_TRACE
        {
            QString operands = slot.getOperandsString();
            QString result   = slot.getResultString();
            CpuTrace::instruction(
                slot.cycle,
                slot.di.pc,
                slot.di.rawBits(),
                "HW_ST",
                operands,
                result
            );
        }
#endif // AXP_EXEC_TRACE
    }

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {
        return ExecutionBox::PalBox;
    }

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {
        return GrainType::Miscellaneous;
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
    QString     m_mnemonic;
    quint8      m_opcode;
    quint16     m_functionCode;
    GrainPlatform m_platform;
};

// ============================================================================
// Auto-registration
// ============================================================================

// ReSharper disable once CppUnnamedNamespaceInHeaderFile
namespace {
    static GrainAutoRegistrar<HW_ST_InstructionGrain> s_hw_st_registrar(
        0x1B, 0x0000
    );
}

#endif // HW_ST_INSTRUCTIONGRAIN_H
