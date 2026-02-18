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