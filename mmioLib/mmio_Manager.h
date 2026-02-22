// ============================================================================
// mmio_Manager.h - ============================================================================
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

#ifndef MMIO_MANAGER_H
#define MMIO_MANAGER_H

#include "mmioLib_global.h"
#include <QtGlobal>
#include <QVector>
#include <QHash>
#include <QMutex>
#include <algorithm>
#include "../coreLib/Axp_Attributes_core.h"
#include "../coreLib/types_core.h"
#include "../memoryLib/memory_core.h"

class  MMIOManager final
{
public:
	MMIOManager() = default;
	~MMIOManager() = default;

	MMIOManager(const MMIOManager&) = delete;
	MMIOManager& operator=(const MMIOManager&) = delete;

	// ------------------------------------------------------------------------
	// Handler function pointer types
	// ------------------------------------------------------------------------
	typedef quint64(*ReadFn)(void* ctx, quint64 offset, quint8 width) noexcept;
	typedef void(*WriteFn)(void* ctx, quint64 offset, quint64 value, quint8 width) noexcept;

	struct Handlers
	{
		void* ctx{ nullptr };
		ReadFn read{ nullptr };
		WriteFn write{ nullptr };
	};

	// ------------------------------------------------------------------------
	// Per-region policy
	// ------------------------------------------------------------------------
	enum RegionFlags : quint32
	{
		WIDTH_8 = 1u << 0,
		WIDTH_16 = 1u << 1,
		WIDTH_32 = 1u << 2,
		WIDTH_64 = 1u << 3,
		REQUIRE_NATURAL_ALIGNMENT = 1u << 8,
		ALLOW_UNALIGNED = 1u << 9,
		HAS_SIDE_EFFECTS = 1u << 16
	};

	struct RegionDescriptor
	{
		quint64 basePA{ 0 };
		quint64 sizeBytes{ 0 };
		quint32 flags{ WIDTH_8 | WIDTH_16 | WIDTH_32 | WIDTH_64 | REQUIRE_NATURAL_ALIGNMENT };
		quint32 deviceUid{ 0 };
		quint32 hoseId{ 0 };
	};

	// ------------------------------------------------------------------------
	// Public API
	// ------------------------------------------------------------------------
	void initialize() noexcept;
	bool registerRegion(const RegionDescriptor& desc, const Handlers& handlers) noexcept;
	void finalize() noexcept;
	void clear() noexcept;
	void reset() noexcept;

	MEM_STATUS handleRead(quint64 pa, quint8 width, quint64& value) noexcept;
	MEM_STATUS handleWrite(quint64 pa, quint8 width, quint64 value) noexcept;

	bool isFinalized() const noexcept { return m_finalized; }
	int regionCount() const noexcept { return m_regions.size(); }
	QString classifyPA(quint64 pa) const noexcept;

	void flushPendingWrites(quint32 cpuId) noexcept;
	quint32 getPendingMMIOCount(quint32 cpuId) const noexcept;
	MEM_STATUS writeRegister(quint64 pa, quint64 value, quint8 width) noexcept;

private:
	struct Region
	{
		quint64 basePA{ 0 };
		quint64 endPA{ 0 };
		quint32 flags{ 0 };
		quint32 deviceUid{ 0 };
		quint32 hoseId{ 0 };
		Handlers handlers{};
	};

	static quint32 widthToMask(quint8 width) noexcept;
	static bool isNaturallyAligned(quint64 offset, quint8 width) noexcept;
	const Region* findRegion(quint64 pa) const noexcept;

	QVector<Region> m_regions;
	bool m_finalized{ false };
	mutable QHash<quint32, quint32> m_pendingCounts;
	mutable QMutex m_pendingMutex;
};

#endif // MMIO_MANAGER_H