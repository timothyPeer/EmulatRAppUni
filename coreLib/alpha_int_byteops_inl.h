// ============================================================================
// alpha_int_byteops_inl.h - ============================================================================
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

#ifndef ALPHA_INT_BYTEOPS_INL_H
#define ALPHA_INT_BYTEOPS_INL_H
#include <QtGlobal>
#include <emmintrin.h>  // SSE2
#include <tmmintrin.h>  // SSSE3 for pshufb
#include "Axp_Attributes_core.h"

namespace alpha_byteops {

    AXP_HOT AXP_ALWAYS_INLINE qint64 sext16_to_64(quint16 v) noexcept
    {
        return static_cast<qint64>(static_cast<qint16>(v));
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 alignDown(quint64 v, quint64 align) noexcept
    {
        return v & ~(align - 1ull);
    }
// ============================================================================
// Alpha Byte Manipulation Operations with SSE Optimization
//
// These operations are HOT PATH and benefit significantly from SIMD:
// - ZAP/ZAPNOT: Byte masking (8 independent byte operations)
// - MSK: Byte masking based on address offset
// - EXT: Byte extraction and zero-extension
// - INS: Byte insertion with position control
//
// SSE Strategy:
// - Use pshufb (SSSE3) for byte-level permutation
// - Use pand/pandn for byte masking
// - Fallback to scalar for non-SSE platforms
// ============================================================================


// ============================================================================
// ZAP / ZAPNOT - Zero Byte Operations
// ============================================================================

// ZAP: Zero bytes according to mask (1 = zero that byte)
// Example: ZAP 0x0123456789ABCDEF, 0x3C -> 0x01234567000000EF
inline quint64 zap(quint64 value, quint64 mask) noexcept
{
#if defined(__SSSE3__) && !defined(ALPHA_NO_SSE)
    // Use SIMD for parallel byte masking
    __m128i val = _mm_cvtsi64_si128(static_cast<qint64>(value));

    // Build byte mask: expand each bit of mask to 0xFF or 0x00
    quint64 byteMask = 0;
    for (int i = 0; i < 8; i++) {
        if ((mask & (1ULL << i)) == 0) {  // If bit NOT set, keep byte
            byteMask |= (0xFFULL << (i * 8));
        }
    }

    __m128i maskVec = _mm_cvtsi64_si128(static_cast<qint64>(byteMask));
    __m128i result = _mm_and_si128(val, maskVec);

    return static_cast<quint64>(_mm_cvtsi128_si64(result));
#else
    // Scalar fallback
    quint64 result = value;
    for (int i = 0; i < 8; i++) {
        if (mask & (1ULL << i)) {
            result &= ~(0xFFULL << (i * 8));
        }
    }
    return result;
#endif
}

// ZAPNOT: Zero bytes NOT in mask (1 = keep that byte, 0 = zero)
// Example: ZAPNOT 0x0123456789ABCDEF, 0x3C -> 0x00000000890000000
inline quint64 zapnot(quint64 value, quint64 mask) noexcept
{
#if defined(__SSSE3__) && !defined(ALPHA_NO_SSE)
    __m128i val = _mm_cvtsi64_si128(static_cast<qint64>(value));

    // Build byte mask: expand each bit to 0xFF or 0x00
    quint64 byteMask = 0;
    for (int i = 0; i < 8; i++) {
        if (mask & (1ULL << i)) {  // If bit set, keep byte
            byteMask |= (0xFFULL << (i * 8));
        }
    }

    __m128i maskVec = _mm_cvtsi64_si128(static_cast<qint64>(byteMask));
    __m128i result = _mm_and_si128(val, maskVec);

    return static_cast<quint64>(_mm_cvtsi128_si64(result));
#else
    // Scalar fallback
    quint64 result = 0;
    for (int i = 0; i < 8; i++) {
        if (mask & (1ULL << i)) {
            result |= value & (0xFFULL << (i * 8));
        }
    }
    return result;
#endif
}

// ============================================================================
// MSK - Mask Bytes (Low/High variants)
// ============================================================================

// MSKBL: Mask byte at low position
inline quint64 mskbl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    const quint64 mask = ~(0xFFULL << (bytePos * 8));
    return value & mask;
}

// MSKWL: Mask word (2 bytes) at low position
inline quint64 mskwl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    const quint64 mask = ~(0xFFFFULL << (bytePos * 8));
    return value & mask;
}

// MSKLL: Mask longword (4 bytes) at low position
inline quint64 mskll(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    const quint64 mask = ~(0xFFFFFFFFULL << (bytePos * 8));
    return value & mask;
}

// MSKQL: Mask quadword (8 bytes) - essentially returns 0
inline quint64 mskql(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const quint64 mask = (1ULL << (bytePos * 8)) - 1;
    return value & mask;
}

// MSKWH: Mask word at high position
inline quint64 mskwh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return value;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return value;
    const quint64 mask = ~(0xFFFFULL << shift);
    return value & mask;
}

// MSKLH: Mask longword at high position
inline quint64 msklh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return value;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return value;
    const quint64 mask = ~(0xFFFFFFFFULL << shift);
    return value & mask;
}

// MSKQH: Mask quadword at high position
inline quint64 mskqh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return value;
    const quint64 mask = (1ULL << ((8 - bytePos) * 8)) - 1;
    return value & mask;
}

// ============================================================================
// EXT - Extract Bytes (Low/High variants)
// ============================================================================

// EXTBL: Extract byte at low position
inline quint64 extbl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value >> (bytePos * 8)) & 0xFF;
}

// EXTWL: Extract word at low position
inline quint64 extwl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value >> (bytePos * 8)) & 0xFFFF;
}

// EXTLL: Extract longword at low position
inline quint64 extll(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value >> (bytePos * 8)) & 0xFFFFFFFF;
}

// EXTQL: Extract quadword at low position
inline quint64 extql(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return value >> (bytePos * 8);
}

// EXTWH: Extract word at high position
inline quint64 extwh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return (value << shift) & 0xFFFF;
}

// EXTLH: Extract longword at high position
inline quint64 extlh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return (value << shift) & 0xFFFFFFFF;
}

// EXTQH: Extract quadword at high position
inline quint64 extqh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return value << shift;
}

// ============================================================================
// INS - Insert Bytes (Low/High variants)
// ============================================================================

// INSBL: Insert byte at low position
inline quint64 insbl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value & 0xFF) << (bytePos * 8);
}

// INSWL: Insert word at low position
inline quint64 inswl(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value & 0xFFFF) << (bytePos * 8);
}

// INSLL: Insert longword at low position
inline quint64 insll(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return (value & 0xFFFFFFFF) << (bytePos * 8);
}

// INSQL: Insert quadword at low position
inline quint64 insql(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    return value << (bytePos * 8);
}

// INSWH: Insert word at high position
inline quint64 inswh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return (value & 0xFFFF) >> shift;
}

// INSLH: Insert longword at high position
inline quint64 inslh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return (value & 0xFFFFFFFF) >> shift;
}

// INSQH: Insert quadword at high position
inline quint64 insqh(quint64 value, quint64 offset) noexcept
{
    const int bytePos = static_cast<int>(offset & 0x7);
    if (bytePos == 0) return 0;
    const int shift = (8 - bytePos) * 8;
    if (shift >= 64) return 0;
    return value >> shift;
}


// ============================================================================
// STQ_U merge helper (unaligned quadword store)
// ----------------------------------------------------------------------------
// Concept (ASA Vol I, unaligned store semantics):
// - Store 8 bytes starting at address PA, even if not 8-byte aligned.
// - Implemented as RMW of two aligned quadwords:
//     PA0 = align_down(PA, 8)
//     PA1 = PA0 + 8
// - Byte offset k = PA[2:0] (0..7) determines split across q0/q1.
//
// This helper merges SRC into (q0,q1) using masks/shifts only.
// No byte arrays, no SSE. Very fast and easy to audit.
// ----------------------------------------------------------------------------
// IMPORTANT:
// - If k == 0, the store is aligned and only q0 changes (q1 untouched).
// - If the access crosses a page boundary, PA1 may not be contiguous with PA0
//   in physical space and must be translated/validated separately.
// ============================================================================

// ============================================================================
// STQ_U merge helper (unaligned quadword store)
// ----------------------------------------------------------------------------
// Concept (ASA Vol I, unaligned store semantics):
// - Store 8 bytes starting at address PA, even if not 8-byte aligned.
// - Implemented as RMW of two aligned quadwords:
//     PA0 = align_down(PA, 8)
//     PA1 = PA0 + 8
// - Byte offset k = PA[2:0] (0..7) determines split across q0/q1.
//
// This helper merges SRC into (q0,q1) using masks/shifts only.
// No byte arrays, no SSE. Very fast and easy to audit.
// ----------------------------------------------------------------------------
// IMPORTANT:
// - If k == 0, the store is aligned and only q0 changes (q1 untouched).
// - If the access crosses a page boundary, PA1 may not be contiguous with PA0
//   in physical space and must be translated/validated separately.
// ============================================================================

static AXP_HOT AXP_ALWAYS_INLINE void stq_u_merge_lane(
	quint64 q0_in,
	quint64 q1_in,
	quint64 src,
	quint8  byteOffset,   // 0..7 (PA & 7)
	quint64& q0_out,
	quint64& q1_out,
	bool& touchesQ1) noexcept
{
	const quint8 k = static_cast<quint8>(byteOffset & 0x7U);

	if (k == 0) {
		q0_out = src;
		q1_out = q1_in;
		touchesQ1 = false;
		return;
	}

	const quint32 sh = static_cast<quint32>(k) * 8U;
	const quint64 lowMask = (1ULL << sh) - 1ULL;

	q0_out = (q0_in & lowMask) | (src << sh);

	const quint32 shiftRight = (8U - static_cast<quint32>(k)) * 8U;
	const quint64 insertLow = (src >> shiftRight) & lowMask;

	q1_out = (q1_in & ~lowMask) | insertLow;

	touchesQ1 = true;
}



AXP_HOT AXP_ALWAYS_INLINE void unpackLE64(quint64 value, quint8 out[8]) noexcept
{
	out[0] = static_cast<quint8>(value >> 0);
	out[1] = static_cast<quint8>(value >> 8);
	out[2] = static_cast<quint8>(value >> 16);
	out[3] = static_cast<quint8>(value >> 24);
	out[4] = static_cast<quint8>(value >> 32);
	out[5] = static_cast<quint8>(value >> 40);
	out[6] = static_cast<quint8>(value >> 48);
	out[7] = static_cast<quint8>(value >> 56);
}

AXP_HOT AXP_ALWAYS_INLINE quint64 packLE64(const quint8 in[8]) noexcept
{
	return
		(static_cast<quint64>(in[0]) << 0) |
		(static_cast<quint64>(in[1]) << 8) |
		(static_cast<quint64>(in[2]) << 16) |
		(static_cast<quint64>(in[3]) << 24) |
		(static_cast<quint64>(in[4]) << 32) |
		(static_cast<quint64>(in[5]) << 40) |
		(static_cast<quint64>(in[6]) << 48) |
		(static_cast<quint64>(in[7]) << 56);
}


} // namespace alpha_byteops 

#endif // ALPHA_INT_BYTEOPS_INL_H
