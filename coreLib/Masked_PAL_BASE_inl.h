#ifndef MASKED_PAL_BASE_INL_H
#define MASKED_PAL_BASE_INL_H

// ============================================================================
// Masked_PAL_BASE_inl.h
// ============================================================================
// PALcode Base Address Register (PAL_BASE) inline helpers
//
// PAL_BASE Register Layout (EV6):
// Bits [43:15] - PAL_BASE   Base physical address of PALcode (29 bits)
// Bits [14:0]  - Reserved   (Always zero)
//
// PAL_BASE contains the physical address of the PALcode image in memory.
// The address is aligned to 32KB boundaries (bits [14:0] are always 0).
//
// Address calculation:
//   Physical PAL address = PAL_BASE[43:15] << 15
//   Valid range: 0x0000_0000_0000 to 0x0000_07FF_FFFF_8000 (29 bits of address space)
//
// Reference: Alpha Architecture Reference Manual, EV6 Hardware Reference
// ============================================================================

#include <QtGlobal>
#include <QString>
#include "Axp_Attributes_core.h"

// ============================================================================
// PAL_BASE Bit Masks and Constants
// ============================================================================

namespace PAL_BASE {
// PAL_BASE field (bits 43:15) - 29 bits of physical address
static constexpr quint64 BASE_MASK       = 0x00000FFFFFFFE000ULL;
static constexpr quint64 BASE_SHIFT      = 15;

// Reserved bits (bits 14:0) - must be zero
static constexpr quint64 RESERVED_MASK   = 0x0000000000007FFFULL;

// Valid bits mask
static constexpr quint64 VALID_MASK      = BASE_MASK;

// Alignment requirement
static constexpr quint64 ALIGNMENT       = 32768;  // 32KB = 2^15 bytes
static constexpr quint64 ALIGNMENT_MASK  = ALIGNMENT - 1;

// Address space limits
static constexpr quint64 MIN_ADDRESS     = 0x0000000000000000ULL;
static constexpr quint64 MAX_ADDRESS     = 0x00000FFFFFFFE000ULL;  // 29 bits << 15

// Width of PAL_BASE field
static constexpr quint8  BASE_WIDTH      = 29;
}

// ============================================================================
// PAL_BASE Getters
// ============================================================================

/**
 * @brief Get PAL_BASE field (bits 43:15)
 * @return 29-bit PAL base address field
 */
AXP_PURE AXP_FLATTEN
    inline quint32 getPAL_BASE_Field(quint64 palBase) noexcept
{
    return static_cast<quint32>((palBase & PAL_BASE::BASE_MASK) >> PAL_BASE::BASE_SHIFT);
}

/**
 * @brief Get full physical PAL address (with alignment applied)
 * @return Physical address of PALcode base (32KB aligned)
 */
AXP_PURE AXP_FLATTEN
    inline quint64 getPAL_BASE_Address(quint64 palBase) noexcept
{
    return palBase & PAL_BASE::BASE_MASK;
}

/**
 * @brief Get PAL entry point address for specific vector
 * @param palBase PAL_BASE register value
 * @param vectorOffset Offset into PAL code (typically from SCBB)
 * @return Physical address of PAL entry point
 */
AXP_PURE AXP_FLATTEN
    inline quint64 getPAL_EntryPoint(quint64 palBase, quint64 vectorOffset) noexcept
{
    return getPAL_BASE_Address(palBase) + vectorOffset;
}

// ============================================================================
// PAL_BASE Setters
// ============================================================================

/**
 * @brief Set PAL_BASE from physical address
 * @param palBase PAL_BASE register (modified in place)
 * @param physicalAddr Physical address (will be aligned to 32KB)
 */
AXP_FLATTEN
    inline void setPAL_BASE_Address(quint64& palBase, quint64 physicalAddr) noexcept
{
    // Clear reserved bits (force alignment to 32KB)
    palBase = physicalAddr & PAL_BASE::BASE_MASK;
}

/**
 * @brief Set PAL_BASE from 29-bit field value
 * @param palBase PAL_BASE register (modified in place)
 * @param fieldValue 29-bit PAL base address field
 */
AXP_FLATTEN
    inline void setPAL_BASE_Field(quint64& palBase, quint32 fieldValue) noexcept
{
    palBase = (static_cast<quint64>(fieldValue & ((1u << PAL_BASE::BASE_WIDTH) - 1)) << PAL_BASE::BASE_SHIFT);
}

// ============================================================================
// PAL_BASE Validation
// ============================================================================

/**
 * @brief Check if address is aligned to PAL_BASE requirements (32KB)
 */
AXP_PURE AXP_FLATTEN
    inline bool isPAL_BASE_Aligned(quint64 address) noexcept
{
    return (address & PAL_BASE::ALIGNMENT_MASK) == 0;
}

/**
 * @brief Check if PAL_BASE value is valid (no reserved bits set)
 */
AXP_PURE AXP_FLATTEN
    inline bool isValidPAL_BASE(quint64 palBase) noexcept
{
    return (palBase & PAL_BASE::RESERVED_MASK) == 0;
}

/**
 * @brief Check if address is within valid PAL address range
 */
AXP_PURE AXP_FLATTEN
    inline bool isInPAL_AddressRange(quint64 address) noexcept
{
    return address >= PAL_BASE::MIN_ADDRESS && address <= PAL_BASE::MAX_ADDRESS;
}

/**
 * @brief Sanitize PAL_BASE value (clear reserved bits)
 */
AXP_FLATTEN
    inline quint64 sanitizePAL_BASE(quint64 palBase) noexcept
{
    return palBase & PAL_BASE::VALID_MASK;
}

/**
 * @brief Align address to PAL_BASE requirements (round down to 32KB)
 */
AXP_FLATTEN
    inline quint64 alignPAL_BASE(quint64 address) noexcept
{
    return address & PAL_BASE::BASE_MASK;
}

// ============================================================================
// PAL_BASE Address Calculations
// ============================================================================

/**
 * @brief Calculate PAL entry point from base and offset
 * @param palBase PAL_BASE register value
 * @param entryOffset Entry point offset (e.g., from exception vector table)
 * @return Physical address of PAL entry point
 */
AXP_PURE AXP_FLATTEN
    inline quint64 calculatePAL_EntryPoint(quint64 palBase, quint64 entryOffset) noexcept
{
    Q_ASSERT(isValidPAL_BASE(palBase));
    return getPAL_BASE_Address(palBase) + entryOffset;
}

/**
 * @brief Check if address is within PAL code region
 * @param palBase PAL_BASE register value
 * @param address Address to check
 * @param palSize Size of PAL code region (typically 64KB or 128KB)
 * @return true if address is within PAL region
 */
AXP_PURE AXP_FLATTEN
    inline bool isInPAL_Region(quint64 palBase, quint64 address, quint64 palSize = 0x10000) noexcept
{
    quint64 baseAddr = getPAL_BASE_Address(palBase);
    return address >= baseAddr && address < (baseAddr + palSize);
}

/**
 * @brief Get offset of address within PAL region
 * @param palBase PAL_BASE register value
 * @param address Address within PAL region
 * @return Offset from PAL base (0 if address < base)
 */
AXP_PURE AXP_FLATTEN
    inline quint64 getPAL_Offset(quint64 palBase, quint64 address) noexcept
{
    quint64 baseAddr = getPAL_BASE_Address(palBase);
    return (address >= baseAddr) ? (address - baseAddr) : 0;
}

// ============================================================================
// PAL_BASE Comparison Helpers
// ============================================================================

/**
 * @brief Compare two PAL_BASE values
 */
AXP_PURE AXP_FLATTEN
    inline bool isPAL_BASE_Equal(quint64 palBase1, quint64 palBase2) noexcept
{
    return getPAL_BASE_Address(palBase1) == getPAL_BASE_Address(palBase2);
}

/**
 * @brief Check if PAL_BASE is zero (uninitialized)
 */
AXP_PURE AXP_FLATTEN
    inline bool isPAL_BASE_Zero(quint64 palBase) noexcept
{
    return getPAL_BASE_Address(palBase) == 0;
}

// ============================================================================
// PAL_BASE Display / Debug Helpers
// ============================================================================

/**
 * @brief Format PAL_BASE for debugging
 */
inline QString formatPAL_BASE(quint64 palBase) noexcept
{
    quint64 addr = getPAL_BASE_Address(palBase);
    quint32 field = getPAL_BASE_Field(palBase);

    return QString("PAL_BASE[addr=0x%1, field=0x%2]")
        .arg(addr, 12, 16, QChar('0'))
        .arg(field, 7, 16, QChar('0'));
}

/**
 * @brief Format PAL_BASE with detailed breakdown
 */
inline QString formatPAL_BASE_Detailed(quint64 palBase) noexcept
{
    quint64 addr = getPAL_BASE_Address(palBase);
    quint32 field = getPAL_BASE_Field(palBase);
    bool valid = isValidPAL_BASE(palBase);
    bool aligned = isPAL_BASE_Aligned(addr);

    QString result = QString("PAL_BASE Register:\n");
    result += QString("  Raw value:     0x%1\n").arg(palBase, 16, 16, QChar('0'));
    result += QString("  Physical addr: 0x%1\n").arg(addr, 12, 16, QChar('0'));
    result += QString("  Field[43:15]:  0x%1 (%2 bits)\n")
                  .arg(field, 7, 16, QChar('0'))
                  .arg(PAL_BASE::BASE_WIDTH);
    result += QString("  Alignment:     %1 bytes (%2)\n")
                  .arg(PAL_BASE::ALIGNMENT)
                  .arg(aligned ? "OK" : "MISALIGNED");
    result += QString("  Valid:         %1\n").arg(valid ? "Yes" : "No");

    return result;
}

/**
 * @brief Format PAL entry point address
 */
inline QString formatPAL_EntryPoint(quint64 palBase, quint64 offset, const QString& name) noexcept
{
    quint64 entryPoint = calculatePAL_EntryPoint(palBase, offset);
    return QString("PAL[%1] @ 0x%2 (base=0x%3 + offset=0x%4)")
        .arg(name)
        .arg(entryPoint, 12, 16, QChar('0'))
        .arg(getPAL_BASE_Address(palBase), 12, 16, QChar('0'))
        .arg(offset, 4, 16, QChar('0'));
}

// ============================================================================
// PAL_BASE Constants for Common PAL Entry Points
// ============================================================================

namespace PAL_ENTRY {
// Common PAL entry point offsets (from Digital Unix PAL)
static constexpr quint64 RESET           = 0x0000;  // Reset entry
static constexpr quint64 MACHINE_CHECK   = 0x0080;  // Machine check
static constexpr quint64 ARITH_EXCEPTION = 0x0100;  // Arithmetic exception
static constexpr quint64 INTERRUPT       = 0x0180;  // Interrupt
static constexpr quint64 DFAULT          = 0x0200;  // D-stream fault
static constexpr quint64 ITB_MISS        = 0x0280;  // ITB miss
static constexpr quint64 DTB_MISS_SINGLE = 0x0300;  // DTB miss (single)
static constexpr quint64 DTB_MISS_DOUBLE = 0x0380;  // DTB miss (double)
static constexpr quint64 UNALIGNED       = 0x0400;  // Unaligned access
static constexpr quint64 OPCDEC          = 0x0480;  // Opcode decode
static constexpr quint64 FEN             = 0x0500;  // FP disabled
static constexpr quint64 CALL_PAL        = 0x2000;  // CALL_PAL table base
}

/**
 * @brief Get PAL entry point for specific exception
 */
inline quint64 getPAL_ExceptionEntry(quint64 palBase, quint64 exceptionOffset) noexcept
{
    return calculatePAL_EntryPoint(palBase, exceptionOffset);
}

// ============================================================================
// PAL_BASE Atomic Operations (for SMP safety)
// ============================================================================

/**
 * @brief Atomically read PAL_BASE
 */
AXP_PURE AXP_FLATTEN
    inline quint64 atomicReadPAL_BASE(const std::atomic<quint64>& palBase) noexcept
{
    return palBase.load(std::memory_order_acquire);
}

/**
 * @brief Atomically write PAL_BASE
 */
AXP_FLATTEN
    inline void atomicWritePAL_BASE(std::atomic<quint64>& palBase, quint64 address) noexcept
{
    palBase.store(sanitizePAL_BASE(address), std::memory_order_release);
}

/**
 * @brief Atomically compare-and-swap PAL_BASE
 */
AXP_FLATTEN
    inline bool atomicComparePAL_BASE(std::atomic<quint64>& palBase,
                          quint64 expected,
                          quint64 desired) noexcept
{
    desired = sanitizePAL_BASE(desired);
    return palBase.compare_exchange_strong(expected, desired, std::memory_order_release);
}

#endif // MASKED_PAL_BASE_INL_H


/*
 * USAGE:
 * // Set PAL_BASE from physical address
quint64 palBase = 0;
setPAL_BASE_Address(palBase, 0x0000000000800000ULL);  // PAL at 8MB

// Validate alignment
Q_ASSERT(isPAL_BASE_Aligned(0x0000000000800000ULL));  // true (32KB aligned)
Q_ASSERT(!isPAL_BASE_Aligned(0x0000000000801000ULL)); // false (4KB aligned)

// Calculate PAL entry point
quint64 exceptionEntry = calculatePAL_EntryPoint(palBase, PAL_ENTRY::INTERRUPT);
// Result: 0x0000000000800180

// Check if PC is in PAL
quint64 pc = 0x0000000000800200;
if (isInPAL_Region(palBase, pc)) {
    // Executing PAL code
    quint64 offset = getPAL_Offset(palBase, pc);  // 0x200
}

// Get standard exception entry points
quint64 dtbMissEntry = getPAL_ExceptionEntry(palBase, PAL_ENTRY::DTB_MISS_SINGLE);
quint64 opdecEntry = getPAL_ExceptionEntry(palBase, PAL_ENTRY::OPCDEC);

// Debug output
DEBUG_LOG(formatPAL_BASE(palBase));
// Output: "PAL_BASE[addr=0x000000800000, field=0x0010000]"

DEBUG_LOG(formatPAL_EntryPoint(palBase, PAL_ENTRY::INTERRUPT, "Interrupt"));
// Output: "PAL[Interrupt] @ 0x000000800180 (base=0x000000800000 + offset=0x0180)"
 *
 *
 */
