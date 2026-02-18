#pragma once
#include <QtCore/QAtomicInteger>
#include "ISpamTypes.h"

// ============================================================================
// Instruction SPAM Bucket - 4-way set associative
// ============================================================================

template<unsigned Ways = 4>
class ISpamBucket {
	static_assert(Ways >= 1 && Ways <= 8, "Ways must be 1-8");

public:
	ISpamBucket() = default;

	// Seqlock for lock-free reads
	QAtomicInteger<quint32> version{ 0 };
	QAtomicInteger<quint64> occupancy{ 0 };

	ISpamEntry iSpamEntries[Ways];

	static constexpr quint64 FULL_MASK =
		(Ways == 64) ? ~0ULL : ((1ULL << Ways) - 1ULL);

	// ========================================================================
	// Lookup - Lock-free read with seqlock
	// ========================================================================

	[[nodiscard]] ISpamEntry* find(const ISpamTag& tag) noexcept {
		for (;;) {
			const quint32 v0 = version.loadAcquire();
			if (v0 & 1u) continue;  // Writer active

			const quint64 occ = occupancy.loadRelaxed();

			for (unsigned i = 0; i < Ways; ++i) {
				if (!((occ >> i) & 1ULL)) continue;

				ISpamEntry* e = &iSpamEntries[i];
				if (!e->isValid()) continue;
				if (!(e->tag == tag)) continue;

				const quint32 v1 = version.loadAcquire();
				if (v0 == v1) {
					// Hot path: increment access count for LRU
					if (e->accessCount < 255) {
						e->accessCount++;
					}
					return e;
				}
				break;  // Retry
			}

			const quint32 v1 = version.loadAcquire();
			if (v0 == v1) return nullptr;  // Stable miss
		}
	}

	// ========================================================================
	// Insert - Lock-based write
	// ========================================================================

	bool insert(const ISpamEntry& entry) noexcept {
		unsigned slot;
		if (!tryClaimSlot(slot)) {
			return false;  // Cache full
		}

		beginWrite();
		iSpamEntries[slot] = entry;
		iSpamEntries[slot].valid = true;
		endWrite();

		return true;
	}

	// ========================================================================
	// Invalidate by PC or PA
	// ========================================================================

	void invalidateByPC(quint64 pc) noexcept {
		beginWrite();
		const quint64 occ = occupancy.loadRelaxed();

		for (unsigned i = 0; i < Ways; ++i) {
			if (!((occ >> i) & 1ULL)) continue;
			if (iSpamEntries[i].tag.pc == pc) {
				invalidateSlot(i);
			}
		}
		endWrite();
	}

	void invalidateByPA(quint64 pa) noexcept {
		beginWrite();
		const quint64 occ = occupancy.loadRelaxed();

		for (unsigned i = 0; i < Ways; ++i) {
			if (!((occ >> i) & 1ULL)) continue;
			if (iSpamEntries[i].tag.pa == pa) {
				invalidateSlot(i);
			}
		}
		endWrite();
	}

private:
	bool tryClaimSlot(unsigned& slot) noexcept {
		for (;;) {
			const quint64 cur = occupancy.loadRelaxed();
			const quint64 used = cur & FULL_MASK;

			if (used == FULL_MASK) {
				// Evict LRU entry
				slot = findLRU();
				return true;
			}

			const quint64 freeBits = (~used) & FULL_MASK;
			const int bit = qCountTrailingZeroBits(freeBits);
			const quint64 want = cur | (1ULL << bit);

			if (occupancy.testAndSetRelaxed(cur, want)) {
				slot = static_cast<unsigned>(bit);
				return true;
			}
		}
	}

	unsigned findLRU() const noexcept {
		unsigned lru = 0;
		quint8 minCount = 255;

		for (unsigned i = 0; i < Ways; ++i) {
			if (iSpamEntries[i].locked) continue;
			if (iSpamEntries[i].accessCount < minCount) {
				minCount = iSpamEntries[i].accessCount;
				lru = i;
			}
		}
		return lru;
	}

	void invalidateSlot(unsigned slot) noexcept {
		iSpamEntries[slot].valid = false;
		const quint64 mask = ~(1ULL << slot);
		quint64 cur;
		do {
			cur = occupancy.loadRelaxed();
		} while (!occupancy.testAndSetRelaxed(cur, cur & mask));
	}

	inline void beginWrite() noexcept { version.fetchAndAddRelease(1); }
	inline void endWrite() noexcept { version.fetchAndAddRelease(1); }
};