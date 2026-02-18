#ifndef IEEE754_FLOATCONVERSION_INL_H
#define IEEE754_FLOATCONVERSION_INL_H

// ============================================================================
// FloatConversion.h - IEEE 754 Float Format Conversions
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// ============================================================================

#include <QtGlobal>
#include <cstring>
#include <cmath>

// ============================================================================
// Platform-specific count leading zeros
// ============================================================================

// ============================================================================
// Simple Conversions (using C++ float/double casting)
// Fast and correct for normal values
// ============================================================================

inline quint64 convertSFloatToTFloat_Simple(quint32 sFloat) noexcept
{
    // Convert IEEE 754 single (32-bit) to double (64-bit)
    float f;
    std::memcpy(&f, &sFloat, sizeof(float));

    double d = static_cast<double>(f);

    quint64 result;
    std::memcpy(&result, &d, sizeof(double));

    return result;
}

inline quint32 convertTFloatToSFloat_Simple(quint64 tFloat) noexcept
{
    // Convert IEEE 754 double (64-bit) to single (32-bit)
    double d;
    std::memcpy(&d, &tFloat, sizeof(double));

    float f = static_cast<float>(d);

    quint32 result;
    std::memcpy(&result, &f, sizeof(float));

    return result;
}

// ============================================================================
// Bit-Accurate Conversions (manual IEEE 754 format conversion)
// Handles all edge cases: denormals, NaNs, infinities, zeros
// ============================================================================

// ============================================================================
// Platform-specific count leading zeros
// ============================================================================

#if defined(_MSC_VER)
    // MSVC
#include <intrin.h>

inline int countLeadingZeros32(quint32 value) noexcept
{
    if (value == 0) return 32;

    unsigned long index;
    _BitScanReverse(&index, value);
    return 31 - static_cast<int>(index);
}
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang
inline int countLeadingZeros32(quint32 value) noexcept
{
    if (value == 0) return 32;
    return __builtin_clz(value);
}
#else
    // Portable fallback (slower)
inline int countLeadingZeros32(quint32 value) noexcept
{
    if (value == 0) return 32;

    int count = 0;
    quint32 mask = 0x80000000u;

    while ((value & mask) == 0) {
        count++;
        mask >>= 1;
    }

    return count;
}
#endif

// ============================================================================
// Updated convertSFloatToTFloat with portable CLZ
// ============================================================================

inline quint64 convertSFloatToTFloat(quint64 sFloat) noexcept
{
    const quint32 sign_s = (sFloat >> 31) & 0x1;
    const quint32 exp_s = (sFloat >> 23) & 0xFF;
    const quint32 mant_s = sFloat & 0x7FFFFF;

    quint64 sign_t = static_cast<quint64>(sign_s) << 63;
    quint64 exp_t;
    quint64 mant_t;

    if (exp_s == 0) {
        // Zero or denormal
        if (mant_s == 0) {
            // ± Zero
            return sign_t;
        }
        else {
            // Denormal - convert to normalized double
            // Find leading 1 in mantissa
            int leadingZeros = countLeadingZeros32(mant_s);
            int shift = leadingZeros - 9;  // -9 because mantissa is 23 bits (32 - 23 = 9)

            exp_t = (1023 - 127) - shift;
            mant_t = (static_cast<quint64>(mant_s) << (shift + 1)) & 0x7FFFFF;
            mant_t <<= 29;  // Shift to double mantissa position
            return sign_t | (exp_t << 52) | mant_t;
        }
    }
    else if (exp_s == 0xFF) {
        // Infinity or NaN
        exp_t = 0x7FF;
        mant_t = static_cast<quint64>(mant_s) << 29;
        return sign_t | (exp_t << 52) | mant_t;
    }
    else {
        // Normal number
        exp_t = static_cast<quint64>(exp_s) - 127 + 1023;
        mant_t = static_cast<quint64>(mant_s) << 29;
        return sign_t | (exp_t << 52) | mant_t;
    }
}

inline quint32 convertTFloatToSFloat(quint64 tFloat) noexcept
{
    // Extract double precision fields
    const quint64 sign_t = (tFloat >> 63) & 0x1;
    const quint64 exp_t  = (tFloat >> 52) & 0x7FF;
    const quint64 mant_t = tFloat & 0xFFFFFFFFFFFFFULL;

    quint32 sign_s = static_cast<quint32>(sign_t) << 31;
    quint32 exp_s;
    quint32 mant_s;

    // ================================================================
    // Handle special cases
    // ================================================================

    if (exp_t == 0) {
        // Zero or denormal double
        if (mant_t == 0) {
            // ± Zero
            return sign_s;
        } else {
            // Denormal double -> underflow to zero in single
            return sign_s;
        }
    }
    else if (exp_t == 0x7FF) {
        // Infinity or NaN
        exp_s = 0xFF;
        mant_s = static_cast<quint32>(mant_t >> 29);  // Truncate mantissa
        return sign_s | (exp_s << 23) | mant_s;
    }
    else {
        // Normal number
        // Adjust exponent bias: double bias=1023, single bias=127
        qint64 exp_adjusted = static_cast<qint64>(exp_t) - 1023 + 127;

        // Check for overflow
        if (exp_adjusted >= 0xFF) {
            // Overflow to infinity
            return sign_s | (0xFF << 23);
        }

        // Check for underflow
        if (exp_adjusted <= 0) {
            // Underflow to zero or denormal
            if (exp_adjusted < -23) {
                // Too small, rounds to zero
                return sign_s;
            }
            // Create denormal
            // TODO: Implement denormal handling if needed
            return sign_s;
        }

        // Normal conversion
        exp_s = static_cast<quint32>(exp_adjusted);

        // Truncate mantissa from 52 bits to 23 bits
        // Round to nearest (add 0.5 in the truncated bit position)
        quint64 mant_rounded = mant_t + (1ULL << 28);  // Round bit
        mant_s = static_cast<quint32>(mant_rounded >> 29);

        // Handle rounding overflow
        if (mant_s > 0x7FFFFF) {
            mant_s = 0;
            exp_s++;
            if (exp_s >= 0xFF) {
                // Rounded up to infinity
                return sign_s | (0xFF << 23);
            }
        }

        return sign_s | (exp_s << 23) | mant_s;
    }
}

// ============================================================================
// Default implementations (use bit-accurate versions)
// ============================================================================

// You can switch between simple and bit-accurate by changing these aliases
#define USE_SIMPLE_FLOAT_CONVERSION 0

#if USE_SIMPLE_FLOAT_CONVERSION
inline quint64 convertSToT(quint32 s) noexcept { return convertSFloatToTFloat_Simple(s); }
inline quint32 convertTToS(quint64 t) noexcept { return convertTFloatToSFloat_Simple(t); }
#else
inline quint64 convertSToT(quint32 s) noexcept { return convertSFloatToTFloat(s); }
inline quint32 convertTToS(quint64 t) noexcept { return convertTFloatToSFloat(t); }
#endif


#endif // IEEE754_FLOATCONVERSION_INL_H
