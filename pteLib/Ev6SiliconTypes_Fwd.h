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
