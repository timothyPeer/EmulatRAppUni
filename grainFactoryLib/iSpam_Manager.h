#pragma once
#include <QVector>
#include "ISpamBucket.h"

// ============================================================================
// ISPAM Shard Manager - Per-CPU instruction cache
// ============================================================================

template<unsigned Ways = 4, unsigned Buckets = 2048>
class ISpamManager {
	static_assert((Buckets& (Buckets - 1)) == 0, "Buckets must be power of 2");

public:
	using Bucket = ISpamBucket<Ways>;

	explicit ISpamManager(int cpuCount) : m_cpuCount(cpuCount) {
		m_buckets.resize(cpuCount);
		for (int i = 0; i < cpuCount; ++i) {
			m_buckets[i].resize(Buckets);
		}
	}

	// ========================================================================
	// Lookup instruction grain
	// ========================================================================

	[[nodiscard]] ISpamEntry* lookup(
		CPUIdType cpuId,
		quint64 pc,
		quint64 pa,
		quint8 asn
	) noexcept {
		ISpamTag tag{ pc, pa, asn };
		const unsigned idx = bucketIndex(tag);
		return m_buckets[cpuId][idx].find(tag);
	}

	// ========================================================================
	// Insert decoded grain
	// ========================================================================

	bool insert(
		CPUIdType cpuId,
		quint64 pc,
		quint64 pa,
		quint8 asn,
		const InstructionGrain_iSpam& grain
	) noexcept {
		ISpamEntry entry{};
		entry.tag = { pc, pa, asn };
		entry.grain = grain;
		entry.generation = m_generation[cpuId].loadRelaxed();
		entry.valid = true;
		entry.locked = false;
		entry.transitioning = false;
		entry.accessCount = 0;

		const unsigned idx = bucketIndex(entry.tag);
		return m_buckets[cpuId][idx].insert(entry);
	}

	// ========================================================================
	// Invalidation - Self-modifying code or page unmap
	// ========================================================================

	void invalidateByPC(CPUIdType cpuId, quint64 pc) noexcept {
		// Invalidate all buckets that might contain this PC
		for (unsigned i = 0; i < Buckets; ++i) {
			m_buckets[cpuId][i].invalidateByPC(pc);
		}
	}

	void invalidateByPA(CPUIdType cpuId, quint64 pa) noexcept {
		// Called when a page is unmapped or modified
		for (unsigned i = 0; i < Buckets; ++i) {
			m_buckets[cpuId][i].invalidateByPA(pa);
		}
	}

	void invalidateAll(CPUIdType cpuId) noexcept {
		m_generation[cpuId].fetchAndAddRelaxed(1);
	}

private:
	static unsigned bucketIndex(const ISpamTag& tag) noexcept {
		return static_cast<unsigned>(tag.hash()) & (Buckets - 1);
	}

	int m_cpuCount;
	QVector<QVector<Bucket>> m_buckets;  // [CPU][Bucket]
	QVector<QAtomicInteger<quint16>> m_generation;
};
