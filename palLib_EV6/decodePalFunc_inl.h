#pragma once
// ============================================================================
// pal_decode_helpers.h
// ============================================================================
// Header-only PAL instruction decode helpers
//
// This file provides canonical helpers for decoding CALL_PAL instructions.
//
// ASA Reference:
// - Alpha Architecture Reference Manual
//   Section: "CALL_PAL Instruction"
//   PAL function number is encoded in bits <25:0> of the instruction.
//
// All code ASCII, header-only, no CPP required.
// ============================================================================

#include <QtGlobal>
#include "types_core.h"
#include "../grainFactoryLib/PipeLineSlot.h" // provides raw instruction word

// ============================================================================
// decodePalFunc
// ----------------------------------------------------------------------------
// Extract PAL function number from a CALL_PAL instruction.
//
// The CALL_PAL instruction encodes the PAL function number in the low
// 26 bits of the instruction word.
//
// This helper MUST be used by the CALL_PAL instruction grain.
// PAL function dispatch MUST NOT be done in the grain itself.
//
// @param slot PipelineSlot containing the fetched instruction
// @return PAL function number (0 .. 0x03FFFFFF)
//
// ASA Reference:
// - Alpha Architecture Reference Manual
//   "CALL_PAL Instruction"
// ============================================================================
AXP_FLATTEN
    quint32 decodePalFunc(const PipelineSlot& slot) noexcept
{
    // Raw 32-bit instruction word as fetched
    const quint32 instr = slot.instructionWord;

    // PAL function number occupies bits <25:0>
    // Mask: 0b00000011_11111111_11111111_11111111
    constexpr quint32 PAL_FUNC_MASK = 0x03FFFFFFu;

    return instr & PAL_FUNC_MASK;
}
