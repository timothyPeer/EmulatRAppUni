#ifndef IPR_STORAGE_HOT_OSF_H
#define IPR_STORAGE_HOT_OSF_H

#include <QtGlobal>
#include <cstring>

// ============================================================================
// IPRStorage_Hot_OSF - OSF/Tru64 Specific IPRs
// ============================================================================
// PALcode entry points and kernel global pointer (OSF/1, Tru64 Unix)
// Temperature: HOTEXT (PAL entry/exit only)
// ============================================================================

struct alignas(64) IPRStorage_Hot_OSF  
{
	quint64 vptptr_osf{};       // OSF / Tru64 Virtual Address of Page Table Pointer

	// WRENT_OSF Registers - PAL entry vectors
	quint64 ent_int{};          // Interrupt entry
	quint64 ent_arith{};        // Arithmetic exception entry
	quint64 ent_mm{};           // Memory management entry
	quint64 ent_fault{};        // Fault entry
	quint64 ent_una{};          // Unaligned access entry
	quint64 ent_sys{};          // System call entry
	quint64 wrkgp{};            // Write kernel global pointer
	/*quint64 perfmon{};		// IPRStorage_HotExt -- Performance Monitoring*/

	IPRStorage_Hot_OSF() { reset(); }

	void reset() noexcept {
		// Explicit assignment - safe, clear, future-proof
		vptptr_osf = 0;
		ent_int = 0;
		ent_arith = 0;
		ent_mm = 0;
		ent_fault = 0;
		ent_una = 0;
		ent_sys = 0;
		wrkgp = 0;
/*		perfmon = 0;*/
	}
};

// ============================================================================
// COMPILE-TIME VERIFICATION
// ============================================================================

static_assert(sizeof(IPRStorage_Hot_OSF) <= 128,
	"Hot_OSF must be exactly 128 bytes");

static_assert(alignof(IPRStorage_Hot_OSF) == 64,
	"Hot_OSF must be 64-byte aligned");

static_assert(sizeof(IPRStorage_Hot_OSF) % 64 == 0,
	"Hot_OSF size must be multiple of 64");

#endif // IPR_STORAGE_HOT_OSF_H
