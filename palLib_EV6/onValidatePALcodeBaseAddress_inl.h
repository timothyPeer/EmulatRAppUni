// ============================================================================
// onValidatePALcodeBaseAddress_inl.h - \brief Validate PALcode base address
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

#ifndef ONVALIDATEPALCODEBASEADDRESS_INL_H
#define ONVALIDATEPALCODEBASEADDRESS_INL_H
#include "../memoryLib/global_GuestMemory.h"
#include "../coreLib/LoggingMacros.h"
#include "../memoryLib/GuestMemory.h"
#include "../cpuCoreLib/AlphaCPU.h"
#include <QtGlobal>


/// \brief Validate PALcode base address
/// \details Ensure PAL_BASE points to valid physical memory
inline bool onValidatePALcodeBaseAddress(AlphaCPU* argCpu, quint64 palBase)
{
    if (!argCpu) return false;

    const quint64 palStart = palBase & 0xFFFFFFFFFFFFFFF0ULL;
    const quint64 palEnd = palStart + 0x10000ULL;  // 64KB

    // ----------------------------------------------------------------
    // 1. Check alignment (already masked, but verify)
    // ----------------------------------------------------------------
    if ((palBase & 0xFULL) != 0) {
        WARN_LOG(QString("PAL_BASE not 16-byte aligned: %1").arg(0,16,palBase));
        return false;
    }

    // ----------------------------------------------------------------
    // 2. Check for physical memory limit
    //    PAL_BASE must not exceed installed RAM
    // ----------------------------------------------------------------
#ifdef CHECK_PHYSICAL_MEMORY_BOUNDS
    quint64 physicalMemoryLimit = globalMemoryManager().getPhysicalMemorySize();
    if (palEnd > physicalMemoryLimit) {
        qWarning() << "PAL_BASE exceeds physical memory:"
                   << Qt::hex << palBase
                   << "limit:" << physicalMemoryLimit;
        return false;
    }
#endif

    // ----------------------------------------------------------------
    // 3. Check for MMIO window overlap
    //    PAL should be in RAM, not mapped to I/O devices
    // ----------------------------------------------------------------
    auto gm = global_GuestMemory();

    if (gm.isMMIO(palStart, palEnd - palStart)) {
        WARN_LOG(QString("PAL_BASE overlaps MMIO region: 0x%1").arg(0, 1, palBase));
    }
    // #ifdef CHECK_MMIO_CONFLICTS
    // 	if (globalMemoryManager().isMMIORegion(palStart, palEnd)) {
    // 		qWarning() << "PAL_BASE overlaps MMIO region:" << Qt::hex << palBase;
    // 		return false;
    // 	}
    // #endif

    // ----------------------------------------------------------------
    // 4. Verify region is RAM or ROM (not empty/unmapped)
    // ----------------------------------------------------------------
#ifdef VERIFY_PAL_MEMORY_TYPE
    MemoryType memType = globalMemoryManager().getMemoryType(palStart);
    if (memType != MemoryType::RAM && memType != MemoryType::ROM) {
        qWarning() << "PAL_BASE not in RAM/ROM:" << Qt::hex << palBase;
        return false;
    }
#endif

    // ----------------------------------------------------------------
    // 5. Check for CPU scratch area conflicts
    //    Some implementations reserve low physical memory
    // ----------------------------------------------------------------
#ifdef CHECK_SCRATCH_CONFLICTS
    if (palStart < 0x1000ULL) {
        qWarning() << "PAL_BASE conflicts with scratch area:" << Qt::hex << palBase;
        return false;
    }
#endif

    // ----------------------------------------------------------------
    // 6. Check for reasonable address range
    //    PAL typically lives in first few MB of physical memory
    // ----------------------------------------------------------------
#ifdef WARN_UNUSUAL_PAL_BASE
    if (palBase > 0x100000000ULL) {  // > 4GB is unusual
        qWarning() << "PAL_BASE unusually high:" << Qt::hex << palBase;
        // Not fatal, but suspicious
    }
#endif

    return true;
}
#endif // ONVALIDATEPALCODEBASEADDRESS_INL_H
