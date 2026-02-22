// ============================================================================
// syncPALcodeMemoryMapping_inl.h - \brief Synchronize PALcode memory mapping
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

#ifndef SYNCPALCODEMEMORYMAPPING_INL_H
#define SYNCPALCODEMEMORYMAPPING_INL_H

#include "AlphaCPU.h"
#include <QtGlobal>

/// \brief Synchronize PALcode memory mapping
/// \details Register PAL region as MMU-bypass for fast instruction fetch
inline void onSyncPALcodeMemoryMapping(AlphaCPU* argCpu, quint64 palBase)
{
	if (!argCpu) return;

	const quint8 cpuId = argCpu->cpuId();

	// ----------------------------------------------------------------
	// 1. Define PAL physical address region
	//    Alpha PAL occupies 64KB at PAL_BASE (physical address)
	// ----------------------------------------------------------------
	const quint64 palStart = palBase & 0xFFFFFFFFFFFFFFF0ULL;
	const quint64 palEnd = palStart + 0x10000ULL;  // 64KB

	DEBUG_LOG(QString("Registering PAL region: [0x%1, 0x%2)")
		.arg(palStart, 16, 16, QChar('0'))
		.arg(palEnd, 16, 16, QChar('0')));

	// ----------------------------------------------------------------
	// 2. Register PAL region as MMU-bypass
	//    PAL instruction fetches should bypass TLB and use physical addr
	// ----------------------------------------------------------------
#ifdef USE_MEMORY_REGIONS
// Register with your memory subsystem
	globalMemoryManager().registerPALRegion(cpuId, palStart, palEnd);
#endif

	// ----------------------------------------------------------------
	// 3. Update CPU's PAL fetch mode
	//    Instruction fetch checks this to bypass TLB for PAL addresses
	// ----------------------------------------------------------------
	auto& iprs = globalIPRBank()[cpuId];
	iprs.palRegionStart = palStart;
	iprs.palRegionEnd = palEnd;

	// ----------------------------------------------------------------
	// 4. Ensure SafeMemory uses physical reads for PAL region
	//    This is typically handled in your instruction fetch path:
	//    if (isInPalMode() && inPalRegion(pc)) {
	//        instruction = readPhysical(pc);
	//    }
	// ----------------------------------------------------------------

	// ----------------------------------------------------------------
	// 5. Allow PAL loads/stores to bypass DTB (optional)
	//    Some implementations give PAL physical memory access
	// ----------------------------------------------------------------
#ifdef PAL_BYPASS_DTB
	argCpu->setPALBypassDTB(true);
#endif
}
#endif // SYNCPALCODEMEMORYMAPPING_INL_H
