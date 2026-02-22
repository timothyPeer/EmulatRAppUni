// ============================================================================
// mmio_Manager.cpp - ============================================================================
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

#include "mmio_Manager.h"
#include "../coreLib/LoggingMacros.h"
#include <limits>
#include <QMutexLocker>

// ============================================================================
// Helper Functions
// ============================================================================

AXP_HOT AXP_FLATTEN quint32 MMIOManager::widthToMask(quint8 width) noexcept
{
	switch (width)
	{
	case 1: return WIDTH_8;
	case 2: return WIDTH_16;
	case 4: return WIDTH_32;
	case 8: return WIDTH_64;
	default: return 0;
	}
}

AXP_HOT AXP_FLATTEN bool MMIOManager::isNaturallyAligned(quint64 offset, quint8 width) noexcept
{
	switch (width)
	{
	case 1: return true;
	case 2: return ((offset & 0x1ULL) == 0);
	case 4: return ((offset & 0x3ULL) == 0);
	case 8: return ((offset & 0x7ULL) == 0);
	default: return false;
	}
}

// ============================================================================
// Initialization
// ============================================================================

AXP_HOT AXP_FLATTEN void MMIOManager::initialize() noexcept
{
	DEBUG_LOG("MMIOManager: Initializing");

	clear();

	{
		QMutexLocker locker(&m_pendingMutex);
		m_pendingCounts.clear();
	}

	m_finalized = false;

	DEBUG_LOG("MMIOManager: Initialized - ready for device registration");
}

// ============================================================================
// Region Management
// ============================================================================

AXP_HOT AXP_FLATTEN bool MMIOManager::registerRegion(const RegionDescriptor& desc, const Handlers& handlers) noexcept
{
	if (m_finalized) {
		return false;
	}
	if (desc.sizeBytes == 0) {
		return false;
	}
	if (handlers.read == nullptr || handlers.write == nullptr) {
		return false;
	}
	if (desc.basePA > (std::numeric_limits<quint64>::max() - desc.sizeBytes)) {
		return false;
	}

	Region r;
	r.basePA = desc.basePA;
	r.endPA = desc.basePA + desc.sizeBytes;
	r.flags = desc.flags;
	r.deviceUid = desc.deviceUid;
	r.hoseId = desc.hoseId;
	r.handlers = handlers;

	// Overlap check
	for (const auto& e : m_regions) {
		const bool overlap = !(r.endPA <= e.basePA || r.basePA >= e.endPA);
		if (overlap) {
			return false;
		}
	}

	m_regions.push_back(r);
	return true;
}

AXP_HOT AXP_FLATTEN void MMIOManager::finalize() noexcept
{
	if (m_finalized) {
		return;
	}

	std::sort(m_regions.begin(), m_regions.end(),
		[](const Region& a, const Region& b) noexcept {
			return a.basePA < b.basePA;
		});

	m_finalized = true;
}

AXP_HOT AXP_FLATTEN void MMIOManager::clear() noexcept
{
	m_regions.clear();
	m_finalized = false;
}

AXP_HOT AXP_FLATTEN void MMIOManager::reset() noexcept
{
	// Device-specific reset placeholder
}

// ============================================================================
// Region Lookup
// ============================================================================

AXP_HOT AXP_FLATTEN const MMIOManager::Region* MMIOManager::findRegion(quint64 pa) const noexcept
{
	if (m_regions.isEmpty()) {
		return nullptr;
	}

	if (m_finalized) {
		// Binary search
		auto it = std::upper_bound(
			m_regions.begin(),
			m_regions.end(),
			pa,
			[](quint64 value, const Region& r) noexcept {
				return value < r.basePA;
			});

		if (it == m_regions.begin()) {
			return nullptr;
		}
		--it;

		if (pa >= it->basePA && pa < it->endPA) {
			return &(*it);
		}
		return nullptr;
	}
	else {
		// Linear search
		for (const auto& r : m_regions) {
			if (pa >= r.basePA && pa < r.endPA) {
				return &r;
			}
		}
		return nullptr;
	}
}

// ============================================================================
// MMIO Access
// ============================================================================

AXP_HOT AXP_FLATTEN MEM_STATUS MMIOManager::handleRead(quint64 pa, quint8 width, quint64& value) noexcept
{
	value = 0;

	const quint32 wmask = widthToMask(width);
	if (wmask == 0) {
		return MEM_STATUS::OutOfRange;
	}

	const Region* r = findRegion(pa);
	if (!r) {
		return MEM_STATUS::AccessViolation;
	}

	if (pa > (std::numeric_limits<quint64>::max() - width)) {
		return MEM_STATUS::OutOfRange;
	}
	if ((pa + width) > r->endPA) {
		return MEM_STATUS::AccessViolation;
	}

	if ((r->flags & wmask) == 0) {
		return MEM_STATUS::AccessViolation;
	}

	const quint64 offset = pa - r->basePA;

	if ((r->flags & ALLOW_UNALIGNED) == 0) {
		if (r->flags & REQUIRE_NATURAL_ALIGNMENT) {
			if (!isNaturallyAligned(offset, width)) {
				return MEM_STATUS::Misaligned;
			}
		}
	}

	value = r->handlers.read(r->handlers.ctx, offset, width);
	return MEM_STATUS::Ok;
}

AXP_HOT AXP_FLATTEN MEM_STATUS MMIOManager::handleWrite(quint64 pa, quint8 width, quint64 value) noexcept
{
	const quint32 wmask = widthToMask(width);
	if (wmask == 0) {
		return MEM_STATUS::OutOfRange;
	}

	const Region* r = findRegion(pa);
	if (!r) {
		return MEM_STATUS::AccessViolation;
	}

	if (pa > (std::numeric_limits<quint64>::max() - width)) {
		return MEM_STATUS::OutOfRange;
	}
	if ((pa + width) > r->endPA) {
		return MEM_STATUS::AccessViolation;
	}

	if ((r->flags & wmask) == 0) {
		return MEM_STATUS::AccessViolation;
	}

	const quint64 offset = pa - r->basePA;

	if ((r->flags & ALLOW_UNALIGNED) == 0) {
		if (r->flags & REQUIRE_NATURAL_ALIGNMENT) {
			if (!isNaturallyAligned(offset, width)) {
				return MEM_STATUS::Misaligned;
			}
		}
	}

	r->handlers.write(r->handlers.ctx, offset, value, width);
	return MEM_STATUS::Ok;
}

// ============================================================================
// Diagnostics
// ============================================================================

AXP_HOT AXP_FLATTEN QString MMIOManager::classifyPA(quint64 pa) const noexcept
{
	const Region* r = findRegion(pa);
	if (!r) {
		return QString("Unmapped MMIO");
	}

	return QString("MMIO Device UID:%1 Hose:%2 Offset:0x%3")
		.arg(r->deviceUid)
		.arg(r->hoseId)
		.arg(pa - r->basePA, 0, 16);
}

AXP_HOT AXP_FLATTEN void MMIOManager::flushPendingWrites(quint32 cpuId) noexcept
{
	QMutexLocker locker(&m_pendingMutex);
	m_pendingCounts[cpuId] = 0;
	DEBUG_LOG(QString("MMIOManager: Flushed pending writes for CPU %1").arg(cpuId));
}

AXP_HOT AXP_FLATTEN quint32 MMIOManager::getPendingMMIOCount(quint32 cpuId) const noexcept
{
	QMutexLocker locker(&m_pendingMutex);
	return m_pendingCounts.value(cpuId, 0);
}

AXP_HOT AXP_FLATTEN MEM_STATUS MMIOManager::writeRegister(quint64 pa, quint64 value, quint8 width) noexcept
{
	return handleWrite(pa, width, value);
}