// ============================================================================
// iGrain-DualCache_singleton.h - ============================================================================
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


#include "iGrain-Parameter-DecodeCache.h"
#include "iGrain-KeyIdenties.h"

// ============================================================================
// Global Decode Cache Singletons
// ============================================================================

// PC-indexed cache (software view)
inline DecodeCache<PcKey>& pcDecodeCache() noexcept {
	static DecodeCache<PcKey> instance;
	return instance;
}

// PA-indexed cache (hardware view)
inline DecodeCache<PaKey>& paDecodeCache() noexcept {
	static DecodeCache<PaKey> instance;
	return instance;
}