#pragma once
#include <QtGlobal>

#pragma once
#include <QtGlobal>

inline bool alphaIntegerAddOverflow(qint64 a, qint64 b, qint64 r) noexcept
{
	// Signed overflow:
	//   - adding two positives gives negative
	//   - adding two negatives gives positive
	return ((a >= 0 && b >= 0 && r < 0) ||
		(a < 0 && b < 0 && r >= 0));
}

// Compact bit-pattern version:
inline bool alphaIntegerAddOverflow_fast(quint64 a, quint64 b, quint64 r) noexcept
{
	constexpr quint64 SIGNBIT = 0x8000000000000000ULL;
	return (((a ^ r) & (b ^ r) & SIGNBIT) != 0);
}
