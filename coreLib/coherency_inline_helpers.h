#pragma once
#include <QtGlobal>

// Cache coherency states for MESI protocol
enum class CoherencyState
{
	Invalid = 0,
	Shared = 1,
	Exclusive = 2,
	Modified = 3,
	Owned = 4 // if MOESI
};