#pragma once
#include <array>
#include <bitset>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <QtGlobal>


template<typename Entry>
class DefaultInvalidationStrategy {
public:
	void invalidate(Entry& entry) {
		entry.flags.valid = false;
	}
};


// Base policy interface
template<typename EntryT, unsigned AssocWays>
class ReplacementPolicyBase {
public:
	using Entry = EntryT;
	static constexpr unsigned WAYS = AssocWays;
	virtual ~ReplacementPolicyBase() = default;

	// Select victim in a bucket; returns [0..WAYS-1]
	virtual unsigned selectVictim(const Entry* entries) noexcept = 0;

	// Notify a hit/access at 'way'
	virtual void recordAccess(const Entry& entry, unsigned way) noexcept = 0;
};

// SRRIP (2-bit)
template<typename Entry, unsigned AssocWays>
class SRRIPPolicy : public ReplacementPolicyBase<Entry, AssocWays> {
	static constexpr unsigned RRPV_BITS = 2;
	static constexpr unsigned MAX_RRPV = (1u << RRPV_BITS) - 1u;
	std::array<quint8, (AssocWays* RRPV_BITS + 7) / 8> m_rrpv{};

	unsigned get(unsigned way) const noexcept {
		const unsigned byte = (way * RRPV_BITS) / 8, off = (way * RRPV_BITS) % 8;
		return (m_rrpv[byte] >> off) & ((1u << RRPV_BITS) - 1u);
	}
	void set(unsigned way, unsigned v) noexcept {
		const unsigned byte = (way * RRPV_BITS) / 8, off = (way * RRPV_BITS) % 8;
		m_rrpv[byte] &= ~(((1u << RRPV_BITS) - 1u) << off);
		m_rrpv[byte] |= (v << off);
	}
public:
	unsigned selectVictim(const Entry* entries) noexcept override {
		for (unsigned i = 0; i < AssocWays; ++i) if (!entries[i].isValid()) return i;
		// simple: pick the highest RRPV
		unsigned victim = 0, best = get(0);
		for (unsigned i = 1; i < AssocWays; ++i) {
			const unsigned r = get(i);
			if (r > best) { best = r; victim = i; }
		}
		return victim;
	}
	void recordAccess(const Entry&, unsigned way) noexcept override { set(way, 0); }
};

// CLOCK
template<typename Entry, unsigned AssocWays>
class ClockPolicy : public ReplacementPolicyBase<Entry, AssocWays> {
	std::bitset<AssocWays> m_ref{};
	unsigned m_hand = 0;
public:
	unsigned selectVictim(const Entry* entries) noexcept override {
		for (unsigned i = 0; i < AssocWays; ++i) if (!entries[i].isValid()) return i;
		for (;;) {
			if (!m_ref[m_hand]) { const unsigned v = m_hand; m_hand = (m_hand + 1) % AssocWays; return v; }
			m_ref[m_hand] = false; m_hand = (m_hand + 1) % AssocWays;
		}
	}
	void recordAccess(const Entry&, unsigned way) noexcept override { m_ref[way] = true; }
};

enum class ReplacementPolicyType { SRRIP, CLOCK, Random };

// Minimal Random
template<typename Entry, unsigned AssocWays>
class RandomPolicy : public ReplacementPolicyBase<Entry, AssocWays> {
public:
	unsigned selectVictim(const Entry* entries) noexcept override {
		for (unsigned i = 0; i < AssocWays; ++i) if (!entries[i].isValid()) return i;
		return std::rand() % AssocWays;
	}
	void recordAccess(const Entry&, unsigned) noexcept override {}
};

template<typename Entry, unsigned AssocWays>
class PolicySelector {
	std::unique_ptr<ReplacementPolicyBase<Entry, AssocWays>> m_impl;
public:
	explicit PolicySelector(ReplacementPolicyType t) {
		switch (t) {
		case ReplacementPolicyType::SRRIP: m_impl = std::make_unique< SRRIPPolicy<Entry, AssocWays> >(); break;
		case ReplacementPolicyType::CLOCK: m_impl = std::make_unique< ClockPolicy<Entry, AssocWays> >(); break;
		default:                           m_impl = std::make_unique< RandomPolicy<Entry, AssocWays> >(); break;
		}
	}
	unsigned selectVictim(const Entry* entries) noexcept { return m_impl->selectVictim(entries); }
	void recordAccess(const Entry& e, unsigned way) noexcept { m_impl->recordAccess(e, way); }
};


