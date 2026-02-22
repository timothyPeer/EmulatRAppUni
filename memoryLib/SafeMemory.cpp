// ============================================================================
// SafeMemory.cpp - Implementation using SparseMemoryBacking
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
//  SafeMemory.cpp - Implementation using SparseMemoryBacking
//
// ============================================================================

#include "SafeMemory.h"
#include "SparseMemoryBacking.h"
#include "coreLib/LoggingMacros.h"
#include <QtEndian>
#include <cstring>

#define COMPONENT_NAME "SafeMemory"

// Define destructor where SparseMemoryBacking is complete
SafeMemory::~SafeMemory() noexcept = default;

// ============================================================================
// LIFECYCLE
// ============================================================================

AXP_HOT AXP_FLATTEN bool SafeMemory::initialize(quint64 sizeBytes) noexcept
{
    if (sizeBytes == 0) {
        ERROR_LOG("SafeMemory: Zero-byte initialization requested");
        m_backing.reset();
        m_sizeBytes = 0;
        return false;
    }

    if (sizeBytes > MAX_RAM_SIZE + 0x10000) {  // 32 GB + 64 KB
        ERROR_LOG(QString("SafeMemory: Size %1 exceeds maximum")
            .arg(sizeBytes));
        return false;
    }

    // Expected size for Option A: 64 KB + 32 GB
    constexpr quint64 expectedSize = 0x0001'0000ULL + 0x8'0000'0000ULL;  // 64 KB + 32 GB
    
    if (sizeBytes == expectedSize) {
        INFO_LOG(QString("SafeMemory: Initializing with Option A layout"));
        INFO_LOG(QString("  Low memory:  64 KB (offsets 0x0 - 0x10000)"));
        INFO_LOG(QString("  Main RAM:    32 GB (offsets 0x10000 - 0x8_0001_0000)"));
    } else {
        WARN_LOG(QString("SafeMemory: Size %1 != expected %2 (continuing)")
            .arg(sizeBytes, 16, 16, QChar('0'))
            .arg(expectedSize, 16, 16, QChar('0')));
    }

    // Create sparse backing
    m_backing = std::make_unique<SparseMemoryBacking>();

    if (!m_backing->allocate(sizeBytes)) {
        ERROR_LOG("SafeMemory: Failed to allocate sparse backing");
        m_backing.reset();
        m_sizeBytes = 0;
        return false;
    }

    m_sizeBytes = sizeBytes;

    INFO_LOG(QString("SafeMemory: Initialized %1 GB (sparse backing)")
        .arg(sizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2));

    return true;
}

AXP_HOT AXP_FLATTEN void SafeMemory::clear() noexcept
{
    if (m_backing) {
        DEBUG_LOG("SafeMemory: Clearing memory (releasing all pages)");

        // Re-allocate to clear all pages
        quint64 size = m_sizeBytes;
        m_backing->release();
        m_backing->allocate(size);
    }
}

AXP_HOT AXP_FLATTEN bool SafeMemory::isInitialized() const noexcept
{
    return (m_sizeBytes != 0) && m_backing && m_backing->isAllocated();
}

AXP_HOT AXP_FLATTEN quint64 SafeMemory::sizeBytes() const noexcept
{
    return m_sizeBytes;
}

quint64 SafeMemory::allocatedBytes() const noexcept
{
    return m_backing ? m_backing->allocatedBytes() : 0;
}

quint64 SafeMemory::capacityBytes() const noexcept
{
    return m_backing ? m_backing->capacityBytes() : 0;
}

// ============================================================================
// VALIDATION (ALPHA ARCHITECTURAL RULES)
// ============================================================================

AXP_HOT AXP_FLATTEN bool SafeMemory::isValidOffset(quint64 offset, quint64 size) const noexcept
{
    if (size == 0) return false;
    if (offset >= m_sizeBytes) return false;
    if (offset > (std::numeric_limits<quint64>::max() - size)) return false;
    return (offset + size) <= m_sizeBytes;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::checkRange(quint64 offset, quint64 size) const noexcept
{
    return isValidOffset(offset, size) ? MEM_STATUS::Ok : MEM_STATUS::OutOfRange;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::checkAlign(quint64 offset, quint8 size) const noexcept
{
    // Alpha alignment rules (SRM v6.0, Section 6.3.3)
    switch (size) {
    case 1: return MEM_STATUS::Ok;
    case 2: return ((offset & 0x1ULL) == 0) ? MEM_STATUS::Ok : MEM_STATUS::Misaligned;
    case 4: return ((offset & 0x3ULL) == 0) ? MEM_STATUS::Ok : MEM_STATUS::Misaligned;
    case 8: return ((offset & 0x7ULL) == 0) ? MEM_STATUS::Ok : MEM_STATUS::Misaligned;
    default: return MEM_STATUS::OutOfRange;
    }
}

// ============================================================================
// GENERIC LOAD/STORE (WITH ARCHITECTURAL CHECKS)
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::load(quint64 offset, quint8 size, quint64& out) const noexcept
{
    out = 0;

    if (!m_backing) {
        WARN_LOG("SafeMemory: Load from uninitialized memory");
        return MEM_STATUS::NotInitialized;
    }

    // Range check
    const MEM_STATUS r = checkRange(offset, static_cast<quint64>(size));
    if (r != MEM_STATUS::Ok) {
        TRACE_LOG(QString("SafeMemory: Load range check failed at offset=0x%1")
            .arg(offset, 16, 16, QChar('0')));
        return r;
    }

    // Alignment check
    const MEM_STATUS a = checkAlign(offset, size);
    if (a != MEM_STATUS::Ok) {
        TRACE_LOG(QString("SafeMemory: Load alignment fault at offset=0x%1 size=%2")
            .arg(offset, 16, 16, QChar('0'))
            .arg(size));
        return a;
    }

    // Delegate to backing (already handles little-endian)
    switch (size) {
    case 1:
        out = static_cast<quint64>(m_backing->load8(offset));
        return MEM_STATUS::Ok;

    case 2: {
        quint16 tmp;
        if (!m_backing->load16(offset, tmp)) {
            return MEM_STATUS::OutOfRange;
        }
        out = static_cast<quint64>(tmp);
        return MEM_STATUS::Ok;
    }

    case 4: {
        quint32 tmp;
        if (!m_backing->load32(offset, tmp)) {
            return MEM_STATUS::OutOfRange;
        }
        out = static_cast<quint64>(tmp);
        return MEM_STATUS::Ok;
    }

    case 8:
        if (!m_backing->load64(offset, out)) {
            return MEM_STATUS::OutOfRange;
        }
        return MEM_STATUS::Ok;

    default:
        return MEM_STATUS::OutOfRange;
    }
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::store(quint64 offset, quint8 size, quint64 value) noexcept
{
    if (!m_backing) {
        WARN_LOG("SafeMemory: Store to uninitialized memory");
        return MEM_STATUS::NotInitialized;
    }

    // Range check
    const MEM_STATUS r = checkRange(offset, static_cast<quint64>(size));
    if (r != MEM_STATUS::Ok) {
        TRACE_LOG(QString("SafeMemory: Store range check failed at offset=0x%1")
            .arg(offset, 16, 16, QChar('0')));
        return r;
    }

    // Alignment check
    const MEM_STATUS a = checkAlign(offset, size);
    if (a != MEM_STATUS::Ok) {
        TRACE_LOG(QString("SafeMemory: Store alignment fault at offset=0x%1 size=%2")
            .arg(offset, 16, 16, QChar('0'))
            .arg(size));
        return a;
    }

    // Delegate to backing (already handles little-endian)
    switch (size) {
    case 1:
        if (!m_backing->store8(offset, static_cast<quint8>(value))) {
            return MEM_STATUS::OutOfRange;
        }
        return MEM_STATUS::Ok;

    case 2:
        if (!m_backing->store16(offset, static_cast<quint16>(value))) {
            return MEM_STATUS::OutOfRange;
        }
        return MEM_STATUS::Ok;

    case 4:
        if (!m_backing->store32(offset, static_cast<quint32>(value))) {
            return MEM_STATUS::OutOfRange;
        }
        return MEM_STATUS::Ok;

    case 8:
        if (!m_backing->store64(offset, value)) {
            return MEM_STATUS::OutOfRange;
        }
        return MEM_STATUS::Ok;

    default:
        return MEM_STATUS::OutOfRange;
    }
}

// ============================================================================
// TYPED ACCESS (LEGACY WRAPPERS)
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::load8(quint64 offset, quint8& out) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS s = load(offset, 1, tmp);
    out = static_cast<quint8>(tmp);
    return s;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::load16(quint64 offset, quint16& out) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS s = load(offset, 2, tmp);
    out = static_cast<quint16>(tmp);
    return s;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::load32(quint64 offset, quint32& out) const noexcept
{
    quint64 tmp = 0;
    const MEM_STATUS s = load(offset, 4, tmp);
    out = static_cast<quint32>(tmp);
    return s;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::load64(quint64 offset, quint64& out) const noexcept
{
    return load(offset, 8, out);
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::store8(quint64 offset, quint8 value) noexcept
{
    return store(offset, 1, static_cast<quint64>(value));
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::store16(quint64 offset, quint16 value) noexcept
{
    return store(offset, 2, static_cast<quint64>(value));
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::store32(quint64 offset, quint32 value) noexcept
{
    return store(offset, 4, static_cast<quint64>(value));
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::store64(quint64 offset, quint64 value) noexcept
{
    return store(offset, 8, value);
}

// ============================================================================
// BLOCK OPERATIONS
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::readBlock(quint64 offset, void* dst, qsizetype size) const noexcept
{
    if (!m_backing) {
        return MEM_STATUS::NotInitialized;
    }

    if (dst == nullptr || size <= 0) {
        return MEM_STATUS::OutOfRange;
    }

    const quint64 uSize = static_cast<quint64>(size);
    const MEM_STATUS r = checkRange(offset, uSize);
    if (r != MEM_STATUS::Ok) {
        return r;
    }

    TRACE_LOG(QString("SafeMemory: readBlock offset=0x%1 size=%2")
        .arg(offset, 16, 16, QChar('0'))
        .arg(size));

    if (!m_backing->loadBlock(offset, dst, uSize)) {
        return MEM_STATUS::OutOfRange;
    }

    return MEM_STATUS::Ok;
}

AXP_HOT AXP_FLATTEN MEM_STATUS SafeMemory::writeBlock(quint64 offset, const void* src, qsizetype size) noexcept
{
    if (!m_backing) {
        return MEM_STATUS::NotInitialized;
    }

    if (src == nullptr || size <= 0) {
        return MEM_STATUS::OutOfRange;
    }

    const quint64 uSize = static_cast<quint64>(size);
    const MEM_STATUS r = checkRange(offset, uSize);
    if (r != MEM_STATUS::Ok) {
        return r;
    }

    DEBUG_LOG(QString("SafeMemory: writeBlock offset=0x%1 size=%2")
        .arg(offset, 16, 16, QChar('0'))
        .arg(size));

    if (!m_backing->storeBlock(offset, src, uSize)) {
        return MEM_STATUS::OutOfRange;
    }

    return MEM_STATUS::Ok;
}

// ============================================================================
// SPAN ACCESS (PREFERRED FOR BUFFERS)
// ============================================================================

MemorySpan SafeMemory::getSpan(quint64 offset, quint64 requestedLen, AccessIntent intent) const noexcept
{
    // Validate backing
    if (!m_backing) {
        ERROR_LOG("SafeMemory: getSpan() called on uninitialized memory");
        return {nullptr, 0, false};
    }

    // Validate offset
    if (offset >= m_sizeBytes) {
        TRACE_LOG(QString("SafeMemory: getSpan() offset 0x%1 >= size 0x%2")
            .arg(offset, 16, 16, QChar('0'))
            .arg(m_sizeBytes, 16, 16, QChar('0')));
        return {nullptr, 0, false};
    }

    // Truncate to page boundary (64 KB)
    constexpr quint64 pageSize = 64 * 1024;
    const quint64 offsetInPage = offset % pageSize;
    const quint64 bytesAvailInPage = pageSize - offsetInPage;

    // Also truncate to end of memory
    const quint64 bytesAvailTotal = m_sizeBytes - offset;

    // Actual length is minimum of: requested, page boundary, total available
    const quint64 actualLen = std::min({requestedLen, bytesAvailInPage, bytesAvailTotal});

    // Get page index
    const qsizetype pageIdx = static_cast<qsizetype>(offset / pageSize);

    // Ensure page exists (allocate if needed)
    quint8* page = m_backing->ensurePage(pageIdx);
    if (!page) {
        ERROR_LOG(QString("SafeMemory: Failed to ensure page %1").arg(pageIdx));
        return {nullptr, 0, false};
    }

    // Return span
    return {
        .data = page + offsetInPage,
        .len = actualLen,
        .writable = (intent != AccessIntent::ReadOnly)
    };
}
