#pragma once
#include <QtGlobal>
#include "types_core.h"

enum class TLBType : quint8 {
	ITB0,
	ITB1,
	DTB0,
	DTB1
};


struct TlbFlags
{
	quint8 gh;            // Granularity hint (GH)
	bool   matchAllAsn;   // ASM (match all ASNs)
	SC_Type sizeClass;     // Page size class (8 KB => 0)

	// Default constructor
	constexpr TlbFlags(
		quint8 gh_ = 0,
		bool matchAll = false,
		SC_Type sc = 0) noexcept
		: gh(gh_), matchAllAsn(matchAll), sizeClass(sc)
	{
	}
};
