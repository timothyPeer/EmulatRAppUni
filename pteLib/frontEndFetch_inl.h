// ============================================================================
// frontEndFetch_inl.h - Prewarm instruction stream
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
#include "types_core.h"
#include "Ev6SiliconTLB_Singleton.h"
#include "alpha_pte_core.h"

static_assert(sizeof(Ev6SiliconTLB_Singleton) > 0, "Ev6SiliconTLB_Singleton is incomplete2!");
inline void frontEndFetch(CPUIdType cpuId, VAType va)
{
	/*		auto& silicon = Ev6SiliconTLB_Singleton().instance();*/
	auto& silicon = globalEv6Silicon();
	auto& mgr = silicon.spam();

	// Prewarm instruction stream
	mgr.prewarmTLB(cpuId, Realm::I,  va);

	// Optional: also prewarm the next page / block
	// tlb.prewarmTLB(m_cpuId, Realm::I, pc + 0x1000);
}