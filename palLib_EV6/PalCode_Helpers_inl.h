#ifndef PALCODE_HELPERS_INL_H
#define PALCODE_HELPERS_INL_H
// PalDecodeHelpers.h
#include <QtGlobal>

/*
    PALcode Instruction Format (CALL_PAL)
    - inst<31:26> : Opcode (6 bits)
    - inst<25:0>  : PALcode Function (26 bits)

    Source: Alpha AXP System Reference Manual, Version 6 (Dec 12, 1994),
            Instruction Formats (I), "PALcode Instruction Format", Figure 3-6.
*/
namespace PalDecodeHelpers
{
static constexpr quint32 kPalOpcodeMask  = 0xFC000000u; // bits 31:26
static constexpr quint32 kPalFuncMask    = 0x03FFFFFFu; // bits 25:0
static constexpr quint32 kPalOpcodeShift = 26u;

// Returns the 6-bit opcode (inst<31:26>).
static inline constexpr quint8 opcode(quint32 rawBits) noexcept
{
    return static_cast<quint8>((rawBits & kPalOpcodeMask) >> kPalOpcodeShift);
}

// Returns the 26-bit PAL function selector (inst<25:0>).
static inline constexpr quint32 palFunction(quint32 rawBits) noexcept
{
    return (rawBits & kPalFuncMask);
}

// True if the instruction is PALcode format (opcode == 0).
static inline constexpr bool isCallPal(quint32 rawBits) noexcept
{
    return (opcode(rawBits) == 0x00u);
}
}

#endif // PALCODE_HELPERS_INL_H
