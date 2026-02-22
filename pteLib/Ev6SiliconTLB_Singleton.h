// ============================================================================
// Ev6SiliconTLB_Singleton.h - ============================================================================
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

#ifndef Ev6SiliconTLB_Singleton_h__
#define Ev6SiliconTLB_Singleton_h__

#include "Ev6SiliconTypes.h"
#include "Ev6SiliconTLB.h"

// ============================================================================
// Ev6SiliconTLB_Singleton - Global silicon backend
// ============================================================================


class Ev6SiliconTLB_Singleton
{
public:

	static Ev6SiliconTLB& silicon() noexcept
	{
		static Ev6SiliconTLB inst(s_cpuCount);
		return inst;
	}

	static Ev6SPAMShardManager& spam() noexcept
	{
		return silicon().spam();
	}

	static void initialize(int cpuCount) noexcept
	{
		s_cpuCount = cpuCount;
		
	}

private:
	static inline int s_cpuCount = 1;
};

// Convenience accessors (inline, can be in header)

// Convenience accessors
inline Ev6SiliconTLB& globalEv6Silicon() noexcept
{
	return Ev6SiliconTLB_Singleton::silicon();
}

inline Ev6SPAMShardManager& globalEv6SPAM() noexcept
{
	return Ev6SiliconTLB_Singleton::spam();
}





/*inline PTECache& globalPTECache() noexcept {
	static PTECache cache;
	return cache;
} */
#endif // Ev6SiliconTLB_Singleton_h__
