// ============================================================================
// mmio_DMACoherencyManager.h - Tracks DMA-allocated buffer metadata.
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
#include <QHash>
#include <QMutex>
#include <QString>
#include <atomic>
#include <QDebug>
#include <QMutexLocker>
#include "mmiolib_global.h"
#include "define_helpers.h"
#include "../memoryLib/UnifiedDataCache.h"
#include "reservationManager_core.h"
#include "../coreLib/LoggingMacros.h"
#include "../coreLib/LoggingMacros.h"
#include "../memoryLib/GuestMemory.h"
#include "../memoryLib/global_guestMemory.h"
#include "../cpuCoreLib/ReservationManager.h"
#include "../cpuCoreLib/global_ReservationManager.h"



// Forward declarations
class SMPManager;
class UnifiedDataCache;

enum class DMADirection { DeviceToRAM, RAMToDevice };


// ============================================================================
// DMA BUFFER DESCRIPTOR
// ============================================================================

/**
 * @brief Tracks DMA-allocated buffer metadata.
 */
struct DMABufferDescriptor {
    quint64 pa;              // Physical address
    quint64 size;            // Size in bytes
    quint32 deviceUid;       // Owner device UID
    quint64 dmaMask;         // Address mask (for 32-bit vs 64-bit devices)
    bool coherent;           // Device participates in cache coherency

    DMABufferDescriptor()
        : pa(0), size(0), deviceUid(0), dmaMask(0xFFFFFFFFFFFFFFFFULL), coherent(false)
    {
    }
};

struct DeviceDescriptor {
	quint32 deviceUid;
	quint32 hoseId;
	bool    cacheCoherent;
	QString deviceName;
};

/*

void registerDevice(const DeviceDescriptor& desc, const MMIOHandlers& mmioHandlers) {
	m_mmioManager->registerRegion(mmioDesc, mmioHandlers);
	m_dmaCoherency->setDeviceCoherency(desc.deviceUid, desc.cacheCoherent);
}

*/
// ============================================================================
// DMA COHERENCY MANAGER
// ============================================================================

/**
 * @brief Manages DMA cache coherency between devices and CPUs.
 *
 * Responsibilities:
 * - Flush CPU caches before device reads from RAM (TX path)
 * - Invalidate CPU caches after device writes to RAM (RX path)
 * - Coordinate with SMPManager for multi-CPU cache operations
 * - Invalidate LL/SC reservations on DMA writes
 * - Track DMA buffer allocations (optional)
 *
 * Thread-safe: all methods use internal locking.
 */
class DMACoherencyManager {

	/*    SMPManager* m_smp;*/
	UnifiedDataCache* m_l3;
	ReservationManager* m_reservationManager;

	// Device coherency tracking (deviceUid -> coherent flag)
	QHash<quint32, bool> m_deviceCoherency;
	mutable QMutex m_coherencyLock;

	// DMA buffer tracking (optional)
	QHash<quint64, DMABufferDescriptor> m_dmaBuffers;  // pa -> descriptor
	mutable QMutex m_bufferLock;

	// Statistics
	mutable QMutex m_statsLock;
	Stats m_stats;


	GuestMemory* m_guestMemory{ nullptr };  // For address validation

public:
    /**
     * @brief Constructor.
     * @param smp SMP manager (for cache broadcasts)
     * @param l3 Shared L3 cache (optional, for direct invalidation)
     */

	 // ============================================================================
	 // CONSTRUCTION / DESTRUCTION
	 // ============================================================================
	DMACoherencyManager(UnifiedDataCache* l3 = nullptr) 
		: m_l3(l3)
		, m_guestMemory(&global_GuestMemory())
		, m_reservationManager(&global_ReservationManager())
	{
		// Initialize statistics
		m_stats = Stats();

		DEBUG_LOG("DMACoherencyManager initialized");
	}

	~DMACoherencyManager()
	{
		QMutexLocker<QMutex> locker(&m_bufferLock);

		if (!m_dmaBuffers.isEmpty()) {
			WARN_LOG(QString("DMACoherencyManager: %1 DMA buffers still allocated at destruction")
				.arg(m_dmaBuffers.size()));
		}

		DEBUG_LOG("DMACoherencyManager destroyed");
	}

	void attachGuestMemory(GuestMemory* gm) { m_guestMemory = gm; }

    // ------------------------------------------------------------------------
    // DMA COHERENCY OPERATIONS (device ? RAM)
    // ------------------------------------------------------------------------

    /**
     * @brief Prepare memory range for device read (DMA from RAM).
     *
     * Called BEFORE device DMA-reads descriptors/buffers from RAM.
     * Ensures CPU dirty cache lines are written back to RAM.
     *
     * Example: NIC TX - guest writes TX descriptor, device DMA-reads it.
     *
     * @param pa Physical address base
     * @param size Size in bytes
     * @param deviceUid Device UID (for diagnostics)
     *
     * Operations:
     * 1. Flush dirty lines from all CPU caches (L1/L2/L3)
     * 2. Drain posted writes (ensure MMIO writes reach devices)
     * 3. Memory barrier (seq_cst)
     */

	 // ============================================================================
	 // DMA COHERENCY OPERATIONS
	 // ============================================================================
	void prepareForDeviceRead(quint64 pa, quint64 size, quint32 deviceUid)
	{
		if (size == 0) {
			WARN_LOG("DMACoherencyManager::prepareForDeviceRead: size=0");
			return;
		}

		if (!validateDMATarget(pa, size)) {
			ERROR_LOG(QString("DMA target 0x%1+0x%2 not in valid RAM range")
				.arg(pa, 16, 16, QChar('0')).arg(size, 0, 16));
			return;
		}
		// Check if device is cache-coherent (skip operations if so)
		{
			QMutexLocker locker(&m_coherencyLock);
			if (m_deviceCoherency.value(deviceUid, false)) {
				DEBUG_LOG(QString("Device UID=%1 is coherent, skipping cache flush").arg(deviceUid));
				return;
			}
		}

		DEBUG_LOG(QString("DMA prepareForDeviceRead: PA=0x%1 size=0x%2 device=%3")
			.arg(pa, 16, 16, QChar('0'))
			.arg(size, 0, 16)
			.arg(deviceUid));

		// 1. Flush dirty lines from all CPU caches
		flushCacheRange(pa, size, deviceUid);

		// 2. Memory barrier (ensure all flushes complete before device access)
		std::atomic_thread_fence(std::memory_order_seq_cst);

		// 3. Update statistics
		{
			QMutexLocker locker(&m_statsLock);
			m_stats.prepareForReadCount++;
			m_stats.bytesFlushed += size;
		}
	}

    /**
     * @brief Handle device write completion (DMA to RAM).
     *
     * Called AFTER device DMA-writes data/completions to RAM.
     * Ensures CPUs see fresh device data (not stale cache).
     *
     * Example: NIC RX - device DMA-writes packet, CPU reads it.
     *
     * @param pa Physical address base
     * @param size Size in bytes
     * @param deviceUid Device UID (for diagnostics)
     *
     * Operations:
     * 1. Invalidate CPU cache lines (L1/L2/L3) covering [pa, pa+size)
     * 2. Clear LL/SC reservations overlapping this range
     * 3. Memory barrier (seq_cst)
     */

	void handleDeviceWrite(quint64 pa, quint64 size, quint32 deviceUid)
	{
		if (size == 0) {
			WARN_LOG("DMACoherencyManager::handleDeviceWrite: size=0");
			return;
		}

		// Check if device is cache-coherent
		{
			QMutexLocker<QMutex> locker(&m_coherencyLock);
			if (m_deviceCoherency.value(deviceUid, false)) {
				DEBUG_LOG(QString("Device UID=%1 is coherent, skipping cache invalidate").arg(deviceUid));
				return;
			}
		}

		DEBUG_LOG(QString("DMA handleDeviceWrite: PA=0x%1 size=0x%2 device=%3")
			.arg(pa, 16, 16, QChar('0'))
			.arg(size, 0, 16)
			.arg(deviceUid));

		// 1. Invalidate CPU cache lines (CPUs must see fresh device data)
		invalidateCacheRange(pa, size, deviceUid);

		// 2. Clear LL/SC reservations overlapping this range
		invalidateReservations(pa, size);

		// 3. Memory barrier (ensure invalidations visible before CPU access)
		std::atomic_thread_fence(std::memory_order_seq_cst);

		// 4. Update statistics
		{
			QMutexLocker locker(&m_statsLock);
			m_stats.handleWriteCount++;
			m_stats.bytesInvalidated += size;
		}
	}

    // ------------------------------------------------------------------------
    // DMA BUFFER ALLOCATION (optional, for OS-less testing)
    // ------------------------------------------------------------------------

    /**
     * @brief Allocate DMA-able buffer.
     *
     * Allocates physically contiguous buffer within device's address mask.
     * Optional feature - guest OS typically manages DMA buffers.
     *
     * @param size Size in bytes
     * @param dmaMask Device address mask (e.g., 0xFFFFFFFF for 32-bit)
     * @param deviceUid Device UID
     * @return Physical address, or 0 on failure
     */

	 // ============================================================================
	 // DMA BUFFER ALLOCATION (optional)
	 // ============================================================================
	quint64 allocateDMABuffer(quint64 size, quint64 dmaMask, quint32 deviceUid)
	{
		if (size == 0) {
			ERROR_LOG("DMACoherencyManager::allocateDMABuffer: size=0");
			return 0;
		}

		// Round size up to page boundary (4KB)
		const quint64 PAGE_SIZE = 4096;
		quint64 alignedSize = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

		// TODO: Actual physical memory allocation from SafeMemory
		// For now, return a dummy address within dmaMask
		// In real implementation, this would call SafeMemory::allocatePhysical()

		quint64 pa = 0x10000000;  // Dummy PA (would be allocated from memory manager)

		// Ensure PA is within device's address mask
		if ((pa & dmaMask) != pa) {
			ERROR_LOG(QString("DMACoherencyManager: Allocated PA 0x%1 exceeds device dmaMask 0x%2")
				.arg(pa, 16, 16, QChar('0'))
				.arg(dmaMask, 16, 16, QChar('0')));
			return 0;
		}

		// Track allocation
		QMutexLocker<QMutex> locker(&m_bufferLock);

		DMABufferDescriptor desc;
		desc.pa = pa;
		desc.size = alignedSize;
		desc.deviceUid = deviceUid;
		desc.dmaMask = dmaMask;
		desc.coherent = isDeviceCoherent(deviceUid);

		m_dmaBuffers[pa] = desc;

		// Update statistics
		{
			QMutexLocker<QMutex> statsLocker(&m_statsLock);
			m_stats.allocatedBuffers++;
			m_stats.totalAllocatedBytes += alignedSize;
		}

		DEBUG_LOG(QString("Allocated DMA buffer: PA=0x%1 size=0x%2 device=%3")
			.arg(pa, 16, 16, QChar('0'))
			.arg(alignedSize, 0, 16)
			.arg(deviceUid));

		return pa;
	}

    /**
     * @brief Free DMA buffer.
     * @param pa Physical address (from allocateDMABuffer)
     * @param size Size in bytes
     * @param deviceUid Device UID
     */

	void freeDMABuffer(quint64 pa, quint64 size, quint32 deviceUid)
	{
		QMutexLocker<QMutex> locker(&m_bufferLock);

		auto it = m_dmaBuffers.find(pa);
		if (it == m_dmaBuffers.end()) {
			WARN_LOG(QString("DMACoherencyManager::freeDMABuffer: PA=0x%1 not found")
				.arg(pa, 16, 16, QChar('0')));
			return;
		}

		const DMABufferDescriptor& desc = *it;

		// Validate ownership
		if (desc.deviceUid != deviceUid) {
			ERROR_LOG(QString("DMACoherencyManager::freeDMABuffer: PA=0x%1 owned by device %2, not %3")
				.arg(pa, 16, 16, QChar('0'))
				.arg(desc.deviceUid)
				.arg(deviceUid));
			return;
		}

		// Update statistics
		{
			QMutexLocker<QMutex>  statsLocker(&m_statsLock);
			m_stats.allocatedBuffers--;
			m_stats.totalAllocatedBytes -= desc.size;
		}

		DEBUG_LOG(QString("Freed DMA buffer: PA=0x%1 size=0x%2 device=%3")
			.arg(pa, 16, 16, QChar('0'))
			.arg(desc.size, 0, 16)
			.arg(deviceUid));

		m_dmaBuffers.erase(it);

		// TODO: Return physical memory to SafeMemory allocator
	}

    // ------------------------------------------------------------------------
    // CONFIGURATION
    // ------------------------------------------------------------------------

    /**
     * @brief Set device coherency mode.
     * @param deviceUid Device UID
     * @param coherent true if device participates in cache coherency
     *
     * Coherent devices: Skip cache operations (device snoops CPU caches)
     * Non-coherent devices: Require explicit flush/invalidate (default)
     */

	 // ============================================================================
	 // CONFIGURATION
	 // ============================================================================
	void setDeviceCoherency(quint32 deviceUid, bool coherent)
	{
		QMutexLocker<QMutex> locker(&m_coherencyLock);

		m_deviceCoherency[deviceUid] = coherent;

		DEBUG_LOG(QString("Device UID=%1 coherency: %2")
			.arg(deviceUid)
			.arg(coherent ? "COHERENT" : "NON-COHERENT"));
	}

    /**
     * @brief Check if device is coherent.
     * @param deviceUid Device UID
     * @return true if device participates in cache coherency
     */

	bool isDeviceCoherent(quint32 deviceUid) const
	{
		QMutexLocker locker(&m_coherencyLock);
		return m_deviceCoherency.value(deviceUid, false);
	}

    // ------------------------------------------------------------------------
    // DIAGNOSTICS
    // ------------------------------------------------------------------------

    /**
     * @brief Dump DMA buffer allocations (for debugging).
     * @return Human-readable string
     */

	 // ============================================================================
	 // DIAGNOSTICS
	 // ============================================================================
	QString dumpDMABuffers() const
	{
		QMutexLocker<QMutex> locker(&m_bufferLock);

		QStringList lines;
		lines << "=== DMA Buffers ===";
		lines << QString("%1 buffer(s) allocated:").arg(m_dmaBuffers.size());
		lines << "";

		for (auto it = m_dmaBuffers.begin(); it != m_dmaBuffers.end(); ++it) {
			const DMABufferDescriptor& desc = *it;

			lines << QString("  PA=0x%1: size=0x%2 device=%3 dmaMask=0x%4 %5")
				.arg(desc.pa, 16, 16, QChar('0'))
				.arg(desc.size, 0, 16)
				.arg(desc.deviceUid)
				.arg(desc.dmaMask, 16, 16, QChar('0'))
				.arg(desc.coherent ? "COHERENT" : "NON-COHERENT");
		}

		return lines.join('\n');
	}

    /**
     * @brief Get statistics.
     */
    struct Stats {
        quint64 prepareForReadCount;   // Total prepareForDeviceRead calls
        quint64 handleWriteCount;      // Total handleDeviceWrite calls
        quint64 bytesFlushed;          // Total bytes flushed (TX)
        quint64 bytesInvalidated;      // Total bytes invalidated (RX)
        quint64 allocatedBuffers;      // Currently allocated DMA buffers
        quint64 totalAllocatedBytes;   // Total allocated DMA memory
    };

	Stats getStats() const
	{
		QMutexLocker<QMutex> locker(&m_statsLock);
		return m_stats;
	}

    /**
     * @brief Reset statistics counters.
     */

	void resetStats()
	{
		QMutexLocker locker(&m_statsLock);

		m_stats.prepareForReadCount = 0;
		m_stats.handleWriteCount = 0;
		m_stats.bytesFlushed = 0;
		m_stats.bytesInvalidated = 0;
		// Don't reset allocatedBuffers/totalAllocatedBytes (current state)

		DEBUG_LOG("DMACoherencyManager: Statistics reset");
	}

private:

    // Internal helpers


	bool validateDMATarget(quint64 pa, quint64 size) const
	{
		// DMA must ONLY target SafeMemory regions, never MMIO or AMS

		if (!m_guestMemory->isRAM(pa, size)) {
			ERROR_LOG(QString("DMA violation: Device %1 attempting %2 to non-RAM address 0x%3")
				.arg(deviceUid)
				.arg(dir == DMADirection::DeviceToRAM ? "write" : "read")
				.arg(pa, 16, 16, QChar('0')));
			return false;
		}
		return true;
	}
	// ============================================================================
	// INTERNAL HELPERS
	// ============================================================================
	void flushCacheRange(quint64 pa, quint64 size, quint32 originDeviceUid)
	{
		// Broadcast cache flush to all CPUs via SMPManager
		// originDeviceUid used to skip device's own CPU (if device has affinity)

		// TODO inject SMP Manager Global here. m_smp->broadcastCacheFlush(pa, originDeviceUid);

		// If L3 cache provided, flush directly (optimization)
		if (m_l3) {
			const quint64 CACHE_LINE_SIZE = 64;
			quint64 startLine = pa & ~(CACHE_LINE_SIZE - 1);
			quint64 endLine = (pa + size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

			for (quint64 linePA = startLine; linePA < endLine; linePA += CACHE_LINE_SIZE) {
				m_l3->flushLine(linePA);
			}
		}

		DEBUG_LOG(QString("Flushed cache range: PA=0x%1 size=0x%2")
			.arg(pa, 16, 16, QChar('0'))
			.arg(size, 0, 16));
	}

	void invalidateCacheRange(quint64 pa, quint64 size, quint32 originDeviceUid)
	{
		// Broadcast cache invalidate to all CPUs via SMPManager
		// TODO inject SMP Manager Global here. m_smp->broadcastCacheInvalidateRange(pa, size, originDeviceUid);

		// If L3 cache provided, invalidate directly (optimization)
		if (m_l3) {
			const quint64 CACHE_LINE_SIZE = 64;
			quint64 startLine = pa & ~(CACHE_LINE_SIZE - 1);
			quint64 endLine = (pa + size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

			for (quint64 linePA = startLine; linePA < endLine; linePA += CACHE_LINE_SIZE) {
				m_l3->invalidateLine(linePA);
			}
		}

		DEBUG_LOG(QString("Invalidated cache range: PA=0x%1 size=0x%2")
			.arg(pa, 16, 16, QChar('0'))
			.arg(size, 0, 16));
	}

	// TODO inject SMP Manager Global here.
	void invalidateReservations(quint64 pa, quint64 size)
	{
		if (!m_reservationManager) {
			WARN_LOG("DMACoherencyManager: No ReservationManager attached");
			return;
		}

		// Invalidate all LL/SC reservations overlapping [pa, pa+size)
		m_reservationManager->invalidateRange(pa, size);

		DEBUG_LOG(QString("Invalidated reservations: PA=0x%1 size=0x%2")
			.arg(pa, 16, 16, QChar('0'))
			.arg(size, 0, 16));
	}

};
