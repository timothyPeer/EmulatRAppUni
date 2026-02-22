// ============================================================================
// Ev6SiliconTLB.h - This header uses Ev6SPAMShardManager as a by-value member.
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

#ifndef Ev6SiliconTLB_h__
#define Ev6SiliconTLB_h__

#include <QtCore/QtGlobal>

// This header uses Ev6SPAMShardManager as a by-value member.
// Therefore we must include the header that defines Ev6SPAMShardManager.
#include "Ev6SiliconTypes.h"

// ============================================================================
// Ev6SiliconTLB - Silicon-level TLB container
// ============================================================================

class Ev6SiliconTLB
{
public:
	explicit Ev6SiliconTLB(int cpuCount)
		: m_spam(cpuCount) // Requires Ev6SPAMShardManager ctor(int)
	{
		// Silicon-level init
	}

	AXP_HOT inline Ev6SPAMShardManager& spam() noexcept { return m_spam; }
	AXP_HOT inline const Ev6SPAMShardManager& spam() const noexcept { return m_spam; }

private:
	Ev6SPAMShardManager m_spam;
};

#endif // Ev6SiliconTLB_h__

