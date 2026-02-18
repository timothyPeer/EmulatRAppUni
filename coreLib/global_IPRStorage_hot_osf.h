// ============================================================================
// globalIPRHot_osf.h - Global IPR Hot Storage Accessor (OSF/1)
// ============================================================================
// Global accessor for per-CPU hot IPR storage (OSF/1 variant)
// Provides fast access to frequently-used IPRs
// ============================================================================

#ifndef GLOBAL_IPRSTORAGE_HOT_OSF_H
#define GLOBAL_IPRSTORAGE_HOT_OSF_H

#include "types_core.h"

// Forward declaration
struct IPRStorage_Hot_OSF;

// ============================================================================
// Global Accessor
// ============================================================================
// Returns reference to hot IPR storage for specified CPU
// Fast path - no mutex, no validation
// Used by: PAL handlers, exception processing, critical paths
// ============================================================================

IPRStorage_Hot_OSF& globalIPRHot_OSF(CPUIdType cpuId) noexcept;

#endif // GLOBAL_IPR_HOT_OSF_H
