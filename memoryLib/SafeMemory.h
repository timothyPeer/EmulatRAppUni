// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
// ============================================================================
//
//  SafeMemory.h - Single source of truth for RAM (Option A architecture)
//
//  Purpose:
//    SafeMemory is the ONLY storage for all writable RAM bytes.
//    Implements offset-based interface (PA-agnostic).
//
//  Design:
//    - Offset-based API (no PA knowledge)
//    - Uses SparseMemoryBacking for on-demand page allocation
//    - Handles two PA regions mapped to one contiguous buffer:
//        PA [0x0, 0x10000)           -> SafeMemory offset [0x0, 0x10000)
//        PA [0x80000000, 0x880000000) -> SafeMemory offset [0x10000, ...)
//    - Total size: 64 KB (low) + 32 GB (main) = 0x8_0001_0000 bytes
//
//  Contract:
//    - Input: offset (0-based, relative to SafeMemory buffer start)
//    - Output: MEM_STATUS (Ok, OutOfRange, Misaligned, etc.)
//    - NO PA knowledge (GuestMemory handles PA -> offset translation)
//    - NO duplication (single source of truth for all RAM)
//
//  Memory Layout (Internal):
//    Offset 0x0000_0000 - 0x0000_FFFF: Low 64 KB
//      - HWRPB at offset 0x2000 (16 KB)
//      - Scratch/reserved
//    Offset 0x0001_0000 - 0x8_0001_0000: Main RAM (32 GB)
//
//  Usage:
//    SafeMemory sm;
//    sm.initialize(0x8_0001_0000);  // 64 KB + 32 GB
//    
//    // Read from offset (not PA!)
//    quint64 value;
//    sm.load(0x2000, 8, value);  // Read HWRPB
//    
//    // Write to offset
//    sm.store(0x10000, 8, 0x42);  // Write to main RAM
//
// ============================================================================

#ifndef SAFEMEMORY_H
#define SAFEMEMORY_H

#include <QtGlobal>
#include <memory>
#include "../coreLib/Axp_Attributes_core.h"
#include "memory_core.h"
#include "MemorySpan.h"
#include "SparseMemoryBacking.h"
// Forward declaration


// ============================================================================
// SAFEMEMORY - OFFSET-BASED RAM INTERFACE
// ============================================================================

class SafeMemory final
{
public:
    SafeMemory() = default;
    ~SafeMemory() noexcept;

    SafeMemory(const SafeMemory&) = delete;
    SafeMemory& operator=(const SafeMemory&) = delete;

    SafeMemory(SafeMemory&&) = default;
    SafeMemory& operator=(SafeMemory&&) = default;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Initialize SafeMemory with given size
     * 
     * Expected size for Option A:
     *   64 KB (low memory) + 32 GB (main RAM) = 0x8_0001_0000 bytes
     * 
     * @param sizeBytes Total size in bytes
     * @return true if initialization succeeded
     */
    AXP_HOT AXP_FLATTEN bool initialize(quint64 sizeBytes) noexcept;

    /**
     * @brief Clear all memory (release all pages)
     */
    AXP_HOT AXP_FLATTEN void clear() noexcept;

    /**
     * @brief Check if memory is initialized
     */
    AXP_HOT AXP_FLATTEN bool isInitialized() const noexcept;

    /**
     * @brief Get total size in bytes
     */
    AXP_HOT AXP_FLATTEN quint64 sizeBytes() const noexcept;

    /**
     * @brief Get actually allocated bytes (sparse backing)
     */
    quint64 allocatedBytes() const noexcept;

    /**
     * @brief Get total capacity bytes
     */
    quint64 capacityBytes() const noexcept;

    // ========================================================================
    // VALIDATION (ALPHA ARCHITECTURAL RULES)
    // ========================================================================

    /**
     * @brief Check if offset is valid
     * 
     * @param offset Offset into SafeMemory (0-based)
     * @param size Access size in bytes
     * @return true if [offset, offset+size) is within bounds
     */
    AXP_HOT AXP_FLATTEN bool isValidOffset(quint64 offset, quint64 size) const noexcept;

    /**
     * @brief Check range validity
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS checkRange(quint64 offset, quint64 size) const noexcept;

    /**
     * @brief Check alignment (Alpha rules)
     * 
     * Alpha alignment requirements (SRM v6.0, Section 6.3.3):
     *   - Byte (1):      No alignment required
     *   - Word (2):      2-byte aligned (offset & 0x1 == 0)
     *   - Longword (4):  4-byte aligned (offset & 0x3 == 0)
     *   - Quadword (8):  8-byte aligned (offset & 0x7 == 0)
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS checkAlign(quint64 offset, quint8 size) const noexcept;

    // ========================================================================
    // GENERIC LOAD/STORE (WITH ARCHITECTURAL CHECKS)
    // ========================================================================

    /**
     * @brief Load value from offset
     * 
     * @param offset Offset into SafeMemory (0-based)
     * @param size Access width (1/2/4/8)
     * @param out Output value (little-endian)
     * @return MEM_STATUS (Ok, OutOfRange, Misaligned, etc.)
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS load(quint64 offset, quint8 size, quint64& out) const noexcept;

    /**
     * @brief Store value to offset
     * 
     * @param offset Offset into SafeMemory (0-based)
     * @param size Access width (1/2/4/8)
     * @param value Value to store (little-endian)
     * @return MEM_STATUS
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS store(quint64 offset, quint8 size, quint64 value) noexcept;

    // ========================================================================
    // TYPED ACCESS (LEGACY WRAPPERS)
    // ========================================================================

    AXP_HOT AXP_FLATTEN MEM_STATUS load8(quint64 offset, quint8& out) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS load16(quint64 offset, quint16& out) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS load32(quint64 offset, quint32& out) const noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS load64(quint64 offset, quint64& out) const noexcept;

    AXP_HOT AXP_FLATTEN MEM_STATUS store8(quint64 offset, quint8 value) noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS store16(quint64 offset, quint16 value) noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS store32(quint64 offset, quint32 value) noexcept;
    AXP_HOT AXP_FLATTEN MEM_STATUS store64(quint64 offset, quint64 value) noexcept;

    // ========================================================================
    // BLOCK OPERATIONS
    // ========================================================================

    /**
     * @brief Read block of bytes
     * 
     * @param offset Offset into SafeMemory
     * @param dst Destination buffer
     * @param size Number of bytes to read
     * @return MEM_STATUS
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS readBlock(quint64 offset, void* dst, qsizetype size) const noexcept;

    /**
     * @brief Write block of bytes
     * 
     * @param offset Offset into SafeMemory
     * @param src Source buffer
     * @param size Number of bytes to write
     * @return MEM_STATUS
     */
    AXP_HOT AXP_FLATTEN MEM_STATUS writeBlock(quint64 offset, const void* src, qsizetype size) noexcept;

    // ========================================================================
    // SPAN ACCESS (PREFERRED FOR BUFFERS)
    // ========================================================================

    /**
     * @brief Get contiguous span of memory
     * 
     * Returns span up to page boundary (64 KB).
     * Automatically truncates if requested length crosses page boundary.
     * 
     * This is the PREFERRED method for buffer access (e.g., CSERVE PUTS/GETS)
     * because it handles page boundaries correctly.
     * 
     * @param offset Offset into SafeMemory (0-based)
     * @param requestedLen Requested length in bytes
     * @param intent Read-only, write-only, or read-write
     * @return MemorySpan (may be shorter than requested)
     * 
     * Example:
     *   MemorySpan span = safeMem.getSpan(0x2000, 0x4000, ReadOnly);
     *   if (span.isValid()) {
     *       // Read HWRPB (up to span.len bytes)
     *       memcpy(buffer, span.data, span.len);
     *   }
     */
    MemorySpan getSpan(quint64 offset, quint64 requestedLen, AccessIntent intent) const noexcept;

private:
    std::unique_ptr<SparseMemoryBacking> m_backing;  ///< Sparse page allocator
    quint64 m_sizeBytes{ 0 };                        ///< Total size (64 KB + 32 GB)

    // NO m_basePA - SafeMemory is PA-agnostic!
    // GuestMemory handles PA -> offset translation
};

#endif // SAFEMEMORY_H
