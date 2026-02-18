#ifndef BITUTILS_H
#define BITUTILS_H
// ============================================================================
// BitUtils.h - Optimized Bit Manipulation Utilities
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Purpose:
//   High-performance bit manipulation functions using compiler intrinsics
//   Falls back to portable implementations on unsupported platforms
//
// Performance:
//   - highestSetBit(): 1 cycle (vs 6-8 for loop)
//   - lowestSetBit():  1 cycle (vs 6-8 for loop)
//   - popcount():      1 cycle (vs N for loop)
// ============================================================================



#include <QtGlobal>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// ============================================================================
// Bit Manipulation Utilities
// ============================================================================

namespace BitUtils {

/**
 * @brief Find the index of the highest set bit in a value
 * @tparam T Unsigned integer type
 * @param value Input value (0 returns 0)
 * @return Bit index (0-based) of highest set bit
 *
 * Examples:
 *   highestSetBit(0b0000'0001) = 0
 *   highestSetBit(0b0000'1000) = 3
 *   highestSetBit(0b1000'0000) = 7
 *   highestSetBit(0b1111'1111) = 7
 *   highestSetBit(0) = 0
 *
 * Performance: 1 CPU instruction (bsr/clz)
 */
template<typename T>
AXP_HOT AXP_ALWAYS_INLINE quint8 highestSetBit(T value) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "highestSetBit requires unsigned integer type");

    if (value == 0) return 0;

    // Promote to appropriate size for intrinsics
    if constexpr (sizeof(T) <= sizeof(quint32)) {
        quint32 mask = static_cast<quint32>(value);

#if defined(__GNUC__) || defined(__clang__)
        // GCC/Clang: __builtin_clz counts leading zeros from bit 31
        // Result: 31 - (leading zeros) = highest set bit index
        return 31 - __builtin_clz(mask);

#elif defined(_MSC_VER)
        // MSVC: _BitScanReverse finds highest set bit directly
        unsigned long index;
        _BitScanReverse(&index, mask);
        return static_cast<quint8>(index);

#else
        // Portable fallback (still reasonably fast)
        quint8 bit = 31;
        while (bit > 0 && !(mask & (1u << bit))) {
            --bit;
        }
        return bit;
#endif
    } else {
        // 64-bit version
        quint64 mask = static_cast<quint64>(value);

#if defined(__GNUC__) || defined(__clang__)
        return 63 - __builtin_clzll(mask);

#elif defined(_MSC_VER) && defined(_WIN64)
        unsigned long index;
        _BitScanReverse64(&index, mask);
        return static_cast<quint8>(index);

#else
        // Portable fallback for 64-bit
        quint8 bit = 63;
        while (bit > 0 && !(mask & (1ULL << bit))) {
            --bit;
        }
        return bit;
#endif
    }
}

/**
 * @brief Find the index of the lowest set bit in a value
 * @tparam T Unsigned integer type
 * @param value Input value (0 returns 0)
 * @return Bit index (0-based) of lowest set bit
 *
 * Examples:
 *   lowestSetBit(0b0000'0001) = 0
 *   lowestSetBit(0b0000'1000) = 3
 *   lowestSetBit(0b1000'0000) = 7
 *   lowestSetBit(0b1111'1111) = 0
 *   lowestSetBit(0) = 0
 *
 * Performance: 1 CPU instruction (bsf/ctz)
 */
template<typename T>
inline quint8 lowestSetBit(T value) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "lowestSetBit requires unsigned integer type");

    if (value == 0) return 0;

    if constexpr (sizeof(T) <= sizeof(quint32)) {
        quint32 mask = static_cast<quint32>(value);

#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctz(mask);

#elif defined(_MSC_VER)
        unsigned long index;
        _BitScanForward(&index, mask);
        return static_cast<quint8>(index);

#else
        quint8 bit = 0;
        while (!(mask & (1u << bit))) {
            ++bit;
        }
        return bit;
#endif
    } else {
        quint64 mask = static_cast<quint64>(value);

#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ctzll(mask);

#elif defined(_MSC_VER) && defined(_WIN64)
        unsigned long index;
        _BitScanForward64(&index, mask);
        return static_cast<quint8>(index);

#else
        quint8 bit = 0;
        while (!(mask & (1ULL << bit))) {
            ++bit;
        }
        return bit;
#endif
    }
}

/**
 * @brief Count the number of set bits (population count)
 * @tparam T Unsigned integer type
 * @param value Input value
 * @return Number of bits set to 1
 *
 * Examples:
 *   popcount(0b0000'0000) = 0
 *   popcount(0b0000'0001) = 1
 *   popcount(0b1111'1111) = 8
 *   popcount(0b1010'1010) = 4
 *
 * Performance: 1 CPU instruction (popcnt)
 */
template<typename T>
inline quint8 popcount(T value) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "popcount requires unsigned integer type");

    if constexpr (sizeof(T) <= sizeof(quint32)) {
        quint32 mask = static_cast<quint32>(value);

#if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcount(mask);

#elif defined(_MSC_VER)
        return __popcnt(mask);

#else
        // Portable fallback
        quint8 count = 0;
        while (mask) {
            count += (mask & 1);
            mask >>= 1;
        }
        return count;
#endif
    } else {
        quint64 mask = static_cast<quint64>(value);

#if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcountll(mask);

#elif defined(_MSC_VER) && defined(_WIN64)
        return __popcnt64(mask);

#else
        quint8 count = 0;
        while (mask) {
            count += (mask & 1);
            mask >>= 1;
        }
        return count;
#endif
    }
}

/**
 * @brief Check if a value is a power of 2
 * @tparam T Unsigned integer type
 * @param value Input value
 * @return true if value is a power of 2 (exactly one bit set)
 *
 * Examples:
 *   isPowerOfTwo(0) = false
 *   isPowerOfTwo(1) = true  (2^0)
 *   isPowerOfTwo(2) = true  (2^1)
 *   isPowerOfTwo(3) = false
 *   isPowerOfTwo(8) = true  (2^3)
 */
template<typename T>
inline bool isPowerOfTwo(T value) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "isPowerOfTwo requires unsigned integer type");

    return (value != 0) && ((value & (value - 1)) == 0);
}

/**
 * @brief Round up to next power of 2
 * @tparam T Unsigned integer type
 * @param value Input value
 * @return Smallest power of 2 >= value
 *
 * Examples:
 *   nextPowerOfTwo(0) = 1
 *   nextPowerOfTwo(1) = 1
 *   nextPowerOfTwo(3) = 4
 *   nextPowerOfTwo(5) = 8
 */
template<typename T>
inline T nextPowerOfTwo(T value) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "nextPowerOfTwo requires unsigned integer type");

    if (value == 0) return 1;
    if (isPowerOfTwo(value)) return value;

    return T(1) << (highestSetBit(value) + 1);
}

/**
 * @brief Extract a bit field from a value
 * @tparam T Unsigned integer type
 * @param value Source value
 * @param startBit Starting bit position (0-based, LSB)
 * @param bitCount Number of bits to extract
 * @return Extracted bit field (right-aligned)
 *
 * Example:
 *   extractBits(0xABCD, 4, 8) = 0xBC
 */
template<typename T>
inline T extractBits(T value, quint8 startBit, quint8 bitCount) noexcept
{
    static_assert(std::is_unsigned<T>::value,
                  "extractBits requires unsigned integer type");

    T mask = (T(1) << bitCount) - 1;
    return (value >> startBit) & mask;
}

} // namespace BitUtils

#endif // BITUTILS_H
