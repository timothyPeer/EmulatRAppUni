#ifndef Ev6SiliconTypes_h__
#define Ev6SiliconTypes_h__

// ============================================================================
// EV6 Silicon Concrete Types
// ============================================================================
// This header provides concrete type aliases for the EV6 SPAM/TLB subsystem.
// It requires full template definitions to be available.
// ============================================================================

#include "Ev6SiliconConfig_Constants.h"
#include "Ev6PTETraits.h"

// NOW include full template definitions
#include "alpha_spam_bucket.h"

// Need default policies
template<typename Entry, unsigned AssocWays>
class SRRIPPolicy;

template<typename Entry>
class DefaultInvalidationStrategy;

// Import the template
#include "alpha_spam_manager.h"
#include "TemplatePolicyBase.h"


// ============================================================================
// Concrete EV6 SPAM Types
// ============================================================================

using Ev6SPAMBucket = SPAMBucket<
Ev6PTETraits,
EV6_SPAM_ASSOC_WAYS,
EV6_SPAM_MAX_ASN
> ;

using Ev6SPAMEntry = Ev6SPAMBucket::Entry;

using Ev6SPAMShardManager = SPAMShardManager<
	Ev6PTETraits,
	EV6_SPAM_ASSOC_WAYS,
	EV6_SPAM_MAX_ASN,
	EV6_SPAM_BUCKETS,
	EV6_SPAM_SHARD_BY_SIZE,
	SRRIPPolicy<Ev6SPAMEntry, EV6_SPAM_ASSOC_WAYS>,
	DefaultInvalidationStrategy<Ev6SPAMEntry>
>;
#endif // Ev6SiliconTypes_h__
