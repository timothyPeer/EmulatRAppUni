// ============================================================================
// MemorySpan.h - Contiguous memory span for safe cross-page access
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

// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
// ============================================================================
//
//  MemorySpan.h - Contiguous memory span for safe cross-page access
//
//  Purpose:
//    Provides safe, bounded access to guest memory buffers.
//    Automatically truncates at page boundaries to prevent invalid access.
//
//  Design:
//    - Lightweight structure (3 fields, 17 bytes)
//    - No ownership (caller manages lifetime)
//    - Truncates to page boundary (64 KB)
//    - Used by CSERVE PUTS/GETS and other buffer operations
//
//  Usage:
//    MemorySpan span = guestMemory->getSpanToPA(pa, requestedLen, ReadOnly);
//    if (span.isValid()) {
//        // Use span.data for up to span.len bytes
//        processBuffer(span.data, span.len);
//    }
//
// ============================================================================

#ifndef MEMORY_SPAN_H
#define MEMORY_SPAN_H

#include <QtGlobal>

// ============================================================================
// ACCESS INTENT
// ============================================================================

/**
 * @brief Access intent for span retrieval
 * 
 * Determines whether span is read-only, write-only, or read-write.
 * Used for validation and permission checking.
 */
enum class AccessIntent : quint8 {
    ReadOnly,      // Only reading from buffer (const data)
    WriteOnly,     // Only writing to buffer (output buffer)
    ReadWrite      // Both reading and writing (in-place modification)
};

// ============================================================================
// MEMORY SPAN
// ============================================================================

/**
 * @brief Contiguous memory span
 * 
 * Represents a contiguous block of memory up to page boundary.
 * Used for safe buffer access across subsystems.
 * 
 * Contract:
 *   - data is nullptr if span is invalid
 *   - len is 0 if span is invalid
 *   - len may be less than requested (truncated at page boundary)
 *   - writable is false for read-only spans
 * 
 * Lifetime:
 *   - Span is valid only while underlying memory is valid
 *   - Caller must not use span after memory is freed/unmapped
 *   - No automatic cleanup (caller manages)
 */
struct MemorySpan {
    quint8* data;      ///< Pointer to memory (nullptr if invalid)
    quint64 len;       ///< Valid length (0 if error, may be < requested)
    bool writable;     ///< Read-only or read-write
    
    /**
     * @brief Check if span is valid
     * @return true if span has valid data pointer and non-zero length
     */
    bool isValid() const noexcept {
        return data != nullptr && len > 0;
    }
    
    /**
     * @brief Get const pointer (for read-only access)
     * @return Const pointer to data
     */
    const quint8* constData() const noexcept {
        return data;
    }
    
    /**
     * @brief Get remaining bytes in span
     * @return Number of bytes available
     */
    quint64 remaining() const noexcept {
        return len;
    }
    
    /**
     * @brief Check if span is writable
     * @return true if span allows writes
     */
    bool isWritable() const noexcept {
        return writable;
    }
};

#endif // MEMORY_SPAN_H
