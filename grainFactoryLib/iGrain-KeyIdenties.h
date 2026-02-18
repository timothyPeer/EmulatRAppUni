#pragma once
#include <QtGlobal>

// ============================================================================
// PC Key - Software Identity (Virtual Address)
// ============================================================================
// Used when you care about the LOGICAL instruction stream
// - Exception handling (PC-based)
// - Profiling/debugging
// - Branch prediction history
// ============================================================================

struct PcKey {
	quint64 pc;  // Virtual PC >> 2 (word-aligned)

	[[nodiscard]] constexpr quint64 hash() const noexcept {
		quint64 x = pc;
		x ^= x >> 33;
		x *= 0xff51afd7ed558ccdULL;
		x ^= x >> 33;
		x *= 0xc4ceb9fe1a85ec53ULL;
		x ^= x >> 33;
		return x;
	}

	[[nodiscard]] constexpr bool operator==(const PcKey& other) const noexcept {
		return pc == other.pc;
	}

	[[nodiscard]] static constexpr PcKey fromVA(quint64 va) noexcept {
		return PcKey{ va >> 2 };  // 4-byte aligned
	}
	bool isValid() const noexcept {
		return pc != 0;  // Or whatever makes sense for your key encoding
	}
};

// ============================================================================
// PA Key - Hardware Identity (Physical Address)
// ============================================================================
// Used when you care about the PHYSICAL instruction location
// - Icache coherence (physical addresses)
// - Self-modifying code detection
// - Multi-CPU cache sharing (same PA = same physical instruction)
// ============================================================================

struct PaKey {
	quint64 paIndex;  // Physical address >> 2 (word-aligned)

	[[nodiscard]] constexpr quint64 hash() const noexcept {
		quint64 x = paIndex;
		x ^= x >> 33;
		x *= 0xff51afd7ed558ccdULL;
		x ^= x >> 33;
		x *= 0xc4ceb9fe1a85ec53ULL;
		x ^= x >> 33;
		return x;
	}

	[[nodiscard]] constexpr bool operator==(const PaKey& other) const noexcept {
		return paIndex == other.paIndex;
	}

	[[nodiscard]] static constexpr PaKey fromPA(quint64 pa) noexcept {
		return PaKey{ pa >> 2 };  // 4-byte aligned
	}
	bool isValid() const noexcept {
		return paIndex != 0;  // Or whatever makes sense for your key encoding
	}
};