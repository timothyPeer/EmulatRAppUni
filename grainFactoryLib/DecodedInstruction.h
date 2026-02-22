// ============================================================================
// DecodedInstruction.h - ============================================================================
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

#ifndef DECODEDINSTRUCTION_H
#define DECODEDINSTRUCTION_H

#include <QtGlobal>
#include <QString>

#include "InstructionSemantics_mask.h"
#include "coreLib/Axp_Attributes_core.h"
#include "grainFactoryLib/iGrain_helper_inl.h"

struct InstructionGrain;

// ============================================================================
// DecodedInstruction - Cache-Optimized Instruction Metadata (40 bytes)
//
// semantics:
//   high 32 bits = raw instruction
//   low  32 bits = InstrSemantics flags + packed fields (e.g., memsize enum)
// ============================================================================

struct DecodedInstruction
{
    // ---- HOT (24 bytes)
    quint64 pc;                // VA of instruction
    InstructionGrain* grain;   // flyweight executor
    quint64 semantics;         // [63:32]=raw, [31:0]=semantic flags/fields

    // ---- WARM (12 bytes)
    qint32  branch_disp;       // branch displacement (decoded)
    quint32 pfn;               // PA >> 13 (for fetch coherence / cache validation)
    quint8  ra;
    quint8  rb;
    quint8  rc;
    quint8  literal_val;
    quint8 memSize;             // we need to track memsize separately than from deriving from semantics.

    DecodedInstruction() noexcept
        : pc(0)
        , grain(nullptr)
        , semantics(0)
        , branch_disp(0)
        , pfn(0)
        , ra(31)
        , rb(31)
        , rc(31)
        , literal_val(0)
        , memSize(0)
    {
    }


    AXP_HOT AXP_ALWAYS_INLINE QString getMneumonic() const noexcept
    {
        return getMnemonicFromRaw(rawBits());
    }
    // ---- Raw bits live ONLY in semantics high 32 bits
    [[nodiscard]] quint32 rawBits() const noexcept {
        return static_cast<quint32>(semantics >> 32);
    }

    void setRawBits(quint32 raw) noexcept {

        semantics = (semantics & 0x00000000FFFFFFFFull) | (quint64(raw) << 32);
    }

    // ---- PFN-based fetch coherence
    [[nodiscard]] quint64 physicalAddress() const noexcept {
        const quint64 offset = pc & 0x1FFFu;
        return (quint64(pfn) << 13) | offset;
    }

    void setPhysicalAddress(quint64 pa) noexcept {
        pfn = static_cast<quint32>(pa >> 13);
    }

    AXP_HOT AXP_ALWAYS_INLINE bool needsStall() const noexcept
    {
        // Treat "must stall" or "barrier" or PAL entry as stall-worthy if you encode them that way.
        return (semantics & (S_NeedsStall | S_Barrier | S_PalFormat)) != 0;
    }
};

static_assert(sizeof(DecodedInstruction) == 40, "DecodedInstruction must remain 40 bytes");

#endif // DECODEDINSTRUCTION_H
