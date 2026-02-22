// ============================================================================
// SparseMemoryBacking.h - Sparse Memory Backing
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

#pragma once
#include <QtGlobal>
#include <QtEndian>
#include "../coreLib/LoggingMacros.h"
#include <memory>
#include <atomic>
#include <cstring>
#include <new>

#include "coreLib/Axp_Attributes_core.h"

class SparseMemoryBacking
{
public:
	static constexpr quint64 kPageSize = 64ULL * 1024ULL;
	static constexpr quint64 kPageMask = kPageSize - 1ULL;
	static constexpr qsizetype kPagesPerDirtyWord = 64;

	SparseMemoryBacking() = default;
	~SparseMemoryBacking() { release(); }

	SparseMemoryBacking(const SparseMemoryBacking&) = delete;
	SparseMemoryBacking& operator=(const SparseMemoryBacking&) = delete;

	bool allocate(quint64 bytes) noexcept
	{
		release();
		if (bytes == 0) {
			TRACE_LOG("SparseMemory: Zero-byte allocation requested");
			return true;
		}

		m_capacity = bytes;
		m_pageCount = static_cast<qsizetype>((bytes + kPageMask) / kPageSize);

		TRACE_LOG(QString("SparseMemory: Allocating capacity=%1 GB, pageCount=%2")
			.arg(bytes / (1024.0 * 1024.0 * 1024.0))
			.arg(m_pageCount));

		// Allocate page pointer array (raw array of atomics)
		try {
			m_pages.reset(new std::atomic<quint8*>[m_pageCount]);
			for (qsizetype i = 0; i < m_pageCount; ++i) {
				m_pages[i].store(nullptr, std::memory_order_relaxed);
			}
		}
		catch (const std::bad_alloc&) {
			ERROR_LOG("SparseMemory: Failed to allocate page table");
			m_pageCount = 0;
			m_capacity = 0;
			return false;
		}

		// Allocate dirty tracking if enabled
		if (m_enableDirtyTracking) {
			if (!initializeDirtyTracking()) {
				ERROR_LOG("SparseMemory: Failed to initialize dirty tracking");
				release();
				return false;
			}
		}

		INFO_LOG(QString("SparseMemory: Allocated %1 GB capacity in %2 pages")
			.arg(bytes / (1024.0 * 1024.0 * 1024.0))
			.arg(m_pageCount));
		return true;
	}

	void release() noexcept
	{
		if (!m_pages) return;

		TRACE_LOG(QString("SparseMemory: Releasing %1 allocated pages")
			.arg(m_allocatedPages.load(std::memory_order_relaxed)));

		for (qsizetype i = 0; i < m_pageCount; ++i) {
			quint8* page = m_pages[i].load(std::memory_order_relaxed);
			if (page) {
				delete[] page;
				m_pages[i].store(nullptr, std::memory_order_relaxed);
			}
		}

		m_pages.reset();
		m_dirtyWords.reset();
		m_capacity = 0;
		m_pageCount = 0;
		m_dirtyWordCount = 0;
		m_allocatedPages.store(0, std::memory_order_relaxed);

		DEBUG_LOG("SparseMemory: Release complete");
	}

	// Stats
	quint64 capacityBytes() const noexcept { return m_capacity; }
	quint64 allocatedBytes() const noexcept {
		return static_cast<quint64>(m_allocatedPages.load(std::memory_order_relaxed)) * kPageSize;
	}
	quint64 residentBytes() const noexcept { return allocatedBytes(); }
	qsizetype pageCount() const noexcept { return m_pageCount; }
	qsizetype allocatedPageCount() const noexcept {
		return m_allocatedPages.load(std::memory_order_relaxed);
	}
	bool isAllocated() const noexcept { return m_pageCount > 0; }

	// Dirty tracking
	void enableDirtyTracking(bool enable) noexcept
	{
		m_enableDirtyTracking = enable;
		if (enable) {
			if (m_pageCount > 0) {
				if (!initializeDirtyTracking()) {
					WARN_LOG("SparseMemory: Failed to enable dirty tracking");
					m_enableDirtyTracking = false;
				}
			}
		}
		else {
			m_dirtyWords.reset();
			m_dirtyWordCount = 0;
			DEBUG_LOG("SparseMemory: Dirty tracking disabled");
		}
	}

	bool isDirty(qsizetype pageIdx) const noexcept
	{
		if (!m_enableDirtyTracking || pageIdx >= m_pageCount) return false;

		const qsizetype wordIdx = pageIdx / kPagesPerDirtyWord;
		const quint64 bitMask = 1ULL << (pageIdx % kPagesPerDirtyWord);

		quint64 word = m_dirtyWords[wordIdx].load(std::memory_order_relaxed);
		return (word & bitMask) != 0;
	}

	void clearDirty() noexcept
	{
		if (!m_enableDirtyTracking) return;

		for (qsizetype i = 0; i < m_dirtyWordCount; ++i) {
			m_dirtyWords[i].store(0, std::memory_order_relaxed);
		}
		TRACE_LOG("SparseMemory: Dirty bits cleared");
	}

	// 8-bit access
	AXP_FLATTEN quint8 load8(quint64 pa) const noexcept
	{
		if (pa >= m_capacity) return 0;

		const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
		const quint64 offset = pa & kPageMask;

		quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
		return page ? page[offset] : 0;
	}

	AXP_FLATTEN bool store8(quint64 pa, quint8 value) noexcept
	{
		if (pa >= m_capacity) return false;

		const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
		const quint64 offset = pa & kPageMask;

		quint8* page = ensurePage(pageIdx);
		if (!page) return false;

		page[offset] = value;
		markDirty(pageIdx);
		return true;
	}

	// 16/32/64-bit access (keep all the implementations from before)
	// ... (same as previous version, just change m_pages access)

	AXP_FLATTEN bool load16(quint64 pa, quint16& out) const noexcept
	{
		if (pa + 2 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 2 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
			if (page) {
				quint16 tmp;
				std::memcpy(&tmp, page + offset, 2);
				out = qFromLittleEndian(tmp);
				return true;
			}
			out = 0;
			return true;
		}

		return loadCrossing<quint16>(pa, out);
	}

	AXP_FLATTEN bool store16(quint64 pa, quint16 value) noexcept
	{
		if (pa + 2 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 2 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = ensurePage(pageIdx);
			if (!page) return false;

			quint16 tmp = qToLittleEndian(value);
			std::memcpy(page + offset, &tmp, 2);
			markDirty(pageIdx);
			return true;
		}

		return storeCrossing<quint16>(pa, value);
	}

	AXP_FLATTEN bool load32(quint64 pa, quint32& out) const noexcept
	{
		if (pa + 4 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 4 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
			if (page) {
				quint32 tmp;
				std::memcpy(&tmp, page + offset, 4);
				out = qFromLittleEndian(tmp);
				return true;
			}
			out = 0;
			return true;
		}

		return loadCrossing<quint32>(pa, out);
	}

	AXP_FLATTEN bool store32(quint64 pa, quint32 value) noexcept
	{
		if (pa + 4 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 4 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = ensurePage(pageIdx);
			if (!page) return false;

			quint32 tmp = qToLittleEndian(value);
			std::memcpy(page + offset, &tmp, 4);
			markDirty(pageIdx);
			return true;
		}

		return storeCrossing<quint32>(pa, value);
	}

	AXP_FLATTEN bool load64(quint64 pa, quint64& out) const noexcept
	{
		if (pa + 8 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 8 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
			if (page) {
				quint64 tmp;
				std::memcpy(&tmp, page + offset, 8);
				out = qFromLittleEndian(tmp);
				return true;
			}
			out = 0;
			return true;
		}

		return loadCrossing<quint64>(pa, out);
	}

	AXP_FLATTEN bool store64(quint64 pa, quint64 value) noexcept
	{
		if (pa + 8 > m_capacity) return false;

		const quint64 offset = pa & kPageMask;

		if (offset + 8 <= kPageSize) {
			const qsizetype pageIdx = static_cast<qsizetype>(pa / kPageSize);
			quint8* page = ensurePage(pageIdx);
			if (!page) return false;

			quint64 tmp = qToLittleEndian(value);
			std::memcpy(page + offset, &tmp, 8);
			markDirty(pageIdx);
			return true;
		}

		return storeCrossing<quint64>(pa, value);
	}

	// Bulk operations (keep from before, just change m_pages access)
	bool loadBlock(quint64 pa, void* dst, quint64 len) const noexcept
	{
		if (pa + len > m_capacity) return false;

		quint8* dstPtr = static_cast<quint8*>(dst);
		quint64 remaining = len;
		quint64 addr = pa;

		while (remaining > 0) {
			const qsizetype pageIdx = static_cast<qsizetype>(addr / kPageSize);
			const quint64 offset = addr & kPageMask;
			const quint64 bytesAvail = kPageSize - offset;
			const quint64 chunkLen = (remaining < bytesAvail) ? remaining : bytesAvail;

			quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
			if (page) {
				std::memcpy(dstPtr, page + offset, chunkLen);
			}
			else {
				std::memset(dstPtr, 0, chunkLen);
			}

			dstPtr += chunkLen;
			addr += chunkLen;
			remaining -= chunkLen;
		}

		return true;
	}

	bool storeBlock(quint64 pa, const void* src, quint64 len) noexcept
	{
		if (pa + len > m_capacity) return false;

		const quint8* srcPtr = static_cast<const quint8*>(src);
		quint64 remaining = len;
		quint64 addr = pa;

		while (remaining > 0) {
			const qsizetype pageIdx = static_cast<qsizetype>(addr / kPageSize);
			const quint64 offset = addr & kPageMask;
			const quint64 bytesAvail = kPageSize - offset;
			const quint64 chunkLen = (remaining < bytesAvail) ? remaining : bytesAvail;

			quint8* page = ensurePage(pageIdx);
			if (!page) return false;

			std::memcpy(page + offset, srcPtr, chunkLen);
			markDirty(pageIdx);

			srcPtr += chunkLen;
			addr += chunkLen;
			remaining -= chunkLen;
		}

		return true;
	}

AXP_HOT AXP_ALWAYS_INLINE	quint8* ensurePage(qsizetype pageIdx) noexcept
	{
		quint8* page = m_pages[pageIdx].load(std::memory_order_acquire);
		if (page) return page;

		quint8* newPage = new (std::nothrow) quint8[kPageSize]();
		if (!newPage) {
			ERROR_LOG(QString("SparseMemory: Failed to allocate page %1").arg(pageIdx));
			return nullptr;
		}

		quint8* expected = nullptr;
		if (m_pages[pageIdx].compare_exchange_strong(
			expected, newPage,
			std::memory_order_release,
			std::memory_order_acquire)) {
			m_allocatedPages.fetch_add(1, std::memory_order_relaxed);
			TRACE_LOG(QString("SparseMemory: Allocated page %1").arg(pageIdx));
			return newPage;
		}

		delete[] newPage;
		return m_pages[pageIdx].load(std::memory_order_acquire);
	}

private:
	bool initializeDirtyTracking() noexcept
	{
		m_dirtyWordCount = (m_pageCount + kPagesPerDirtyWord - 1) / kPagesPerDirtyWord;

		try {
			m_dirtyWords.reset(new std::atomic<quint64>[m_dirtyWordCount]);
			for (qsizetype i = 0; i < m_dirtyWordCount; ++i) {
				m_dirtyWords[i].store(0, std::memory_order_relaxed);
			}
			DEBUG_LOG(QString("SparseMemory: Dirty tracking initialized with %1 words")
				.arg(m_dirtyWordCount));
			return true;
		}
		catch (const std::bad_alloc&) {
			ERROR_LOG("SparseMemory: Failed to allocate dirty tracking arrays");
			return false;
		}
	}



	AXP_FLATTEN void markDirty(qsizetype pageIdx) noexcept
	{
		if (!m_enableDirtyTracking) return;

		const qsizetype wordIdx = pageIdx / kPagesPerDirtyWord;
		const quint64 bitMask = 1ULL << (pageIdx % kPagesPerDirtyWord);

		m_dirtyWords[wordIdx].fetch_or(bitMask, std::memory_order_relaxed);
	}

	template<typename T>
	bool loadCrossing(quint64 pa, T& out) const noexcept
	{
		constexpr quint64 size = sizeof(T);

		const qsizetype page1Idx = static_cast<qsizetype>(pa / kPageSize);
		const qsizetype page2Idx = page1Idx + 1;
		const quint64 offset = pa & kPageMask;
		const quint64 bytesInPage1 = kPageSize - offset;
		const quint64 bytesInPage2 = size - bytesInPage1;

		quint8 bytes[sizeof(T)];

		quint8* page1 = m_pages[page1Idx].load(std::memory_order_acquire);
		if (page1) {
			std::memcpy(bytes, page1 + offset, bytesInPage1);
		}
		else {
			std::memset(bytes, 0, bytesInPage1);
		}

		quint8* page2 = m_pages[page2Idx].load(std::memory_order_acquire);
		if (page2) {
			std::memcpy(bytes + bytesInPage1, page2, bytesInPage2);
		}
		else {
			std::memset(bytes + bytesInPage1, 0, bytesInPage2);
		}

		T tmp;
		std::memcpy(&tmp, bytes, size);
		out = qFromLittleEndian(tmp);
		return true;
	}

	template<typename T>
	bool storeCrossing(quint64 pa, T value) noexcept
	{
		constexpr quint64 size = sizeof(T);

		const qsizetype page1Idx = static_cast<qsizetype>(pa / kPageSize);
		const qsizetype page2Idx = page1Idx + 1;
		const quint64 offset = pa & kPageMask;
		const quint64 bytesInPage1 = kPageSize - offset;
		const quint64 bytesInPage2 = size - bytesInPage1;

		T tmp = qToLittleEndian(value);
		quint8 bytes[sizeof(T)];
		std::memcpy(bytes, &tmp, size);

		quint8* page1 = ensurePage(page1Idx);
		if (!page1) return false;
		std::memcpy(page1 + offset, bytes, bytesInPage1);
		markDirty(page1Idx);

		quint8* page2 = ensurePage(page2Idx);
		if (!page2) return false;
		std::memcpy(page2, bytes + bytesInPage1, bytesInPage2);
		markDirty(page2Idx);

		return true;
	}

private:
	quint64 m_capacity{ 0 };
	qsizetype m_pageCount{ 0 };
	qsizetype m_dirtyWordCount{ 0 };
	std::atomic<qsizetype> m_allocatedPages{ 0 };

	// Raw arrays (MSVC compatible)
	std::unique_ptr<std::atomic<quint8*>[]> m_pages;
	std::unique_ptr<std::atomic<quint64>[]> m_dirtyWords;

	bool m_enableDirtyTracking{ false };
};
