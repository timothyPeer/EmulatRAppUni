#pragma once
#include "GrainResolver.h"

inline GrainResolver& global_GrainResolver() {
	static GrainResolver instance;    // header-only is fine
	return instance;
}
