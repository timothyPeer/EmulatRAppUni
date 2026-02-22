// ============================================================================
// Ev6SiliconTypes_Fwd.h - ============================================================================
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
#include "Ev6SiliconTLB.h"
#include "alpha_spam_bucket.h"
#include "Ev6SiliconTypes.h"
#include "alpha_spam_manager.h"
#include "Ev6PTETraits.h"

// ============================================================================
// Forward Declarations for EV6 Silicon Types
// ============================================================================
// This header provides forward declarations only, allowing other headers
// to refer to these types without pulling in full template instantiations.
// ============================================================================


template<typename Traits, unsigned AssocWays, unsigned MaxASN>
class SPAMBucket;

// template<typename Traits, unsigned AssocWays, unsigned MaxASN, unsigned BucketCount,
// 	bool ShardBySize, typename VictimPolicy, typename InvalidationPolicy>
// class SPAMShardManager;
