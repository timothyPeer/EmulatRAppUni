#pragma once
#include <QtGlobal>

// ============================================================================
// EV6 Silicon Configuration Constants
// ============================================================================
// These constants define the EV6 SPAM/TLB architecture parameters.
// They are used throughout the PTE subsystem and must be defined first
// to avoid circular dependencies.
// ============================================================================

namespace Ev6Constants {
	constexpr unsigned SPAM_ASSOC_WAYS = 4;
	constexpr unsigned SPAM_MAX_ASN = 256;
	constexpr unsigned SPAM_BUCKETS = 1024;
	constexpr bool     SPAM_SHARD_BY_SIZE = true;

	// Page sizes
	constexpr unsigned PAGE_SHIFT_8K = 13;
	constexpr unsigned PAGE_SIZE_8K = (1U << PAGE_SHIFT_8K);  // 8192
	constexpr unsigned PAGE_SHIFT_64K = 16;
	constexpr unsigned PAGE_SIZE_64K = (1U << PAGE_SHIFT_64K); // 65536
}

// Export to global namespace for backward compatibility
using namespace Ev6Constants;
constexpr unsigned EV6_SPAM_ASSOC_WAYS = Ev6Constants::SPAM_ASSOC_WAYS;
constexpr unsigned EV6_SPAM_MAX_ASN = Ev6Constants::SPAM_MAX_ASN;
constexpr unsigned EV6_SPAM_BUCKETS = Ev6Constants::SPAM_BUCKETS;
constexpr bool     EV6_SPAM_SHARD_BY_SIZE = Ev6Constants::SPAM_SHARD_BY_SIZE;