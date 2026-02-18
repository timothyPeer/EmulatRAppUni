// ============================================================================
// IPRStorage_Hot.h
// ============================================================================
// Hot-path IPR storage structure for Alpha AXP emulation
// - Cache-line aligned (64 bytes)
// - Size constrained to ≤1024 bytes for L1 cache efficiency
// - Accessed on EVERY instruction retire, exception, or TLB lookup
// - NO dependencies on AlphaCPU or other complex types
// ============================================================================

#ifndef IPRSTORAGE_HOT_H
#define IPRSTORAGE_HOT_H

#include <QtGlobal>
#include <QMutex>
#include <cstring>
#include "types_core.h"
#include "Axp_Attributes_core.h"
#include "PS_helpers_inl.h"
#include "alpha_fpcr_core.h"



// Forward declarations for TLB scratch structures
struct DTB_Tag_Scratch;
struct ITB_Tag_Scratch;

// ============================================================================
// DTB Tag Scratch Structure
// ============================================================================
struct DTB_Tag_Scratch {
    quint64 raw{};
    quint64 vpn{};
    quint8 asn{};
    quint8 gh{};
    bool bank1{};
};

// ============================================================================
// ITB Tag Scratch Structure
// ============================================================================
struct ITB_Tag_Scratch {
    VAType va{};
    ASNType asn{};
};



#endif // IPRSTORAGE_HOT_H

/*
// ============================================================================
// VA (Virtual Address) Register - IPR vs HWPCB Clarification
// ============================================================================
//
// QUESTION: Do we need both IPRs.va and HWPCB.va_fault?
// ANSWER: Yes, but they serve DIFFERENT purposes and are updated at DIFFERENT times.
//

// ============================================================================
// 1. IPRs.va - Active Virtual Address IPR
// ============================================================================
//
// Purpose: Current fault virtual address (architected IPR)
// Updated: During exception delivery
// Read by: PAL handler during exception processing
// Scope: Per-CPU active state
//
// When updated:
//   - Exception occurs (ITB/DTB miss, ACV, unaligned, etc.)
//   - Exception preparation writes faultVA to IPRs.va
//   - PAL handler reads IPRs.va to get fault address
//
// Example:
//   DTB miss at 0x20000:
//     1. MBox creates event with faultVA = 0x20000
//     2. preparePendingEventForDelivery() sets IPRs.va = 0x20000
//     3. PAL handler reads IPRs.va to service the miss

// ============================================================================
// 2. HWPCB.va_fault - Saved Context Field
// ============================================================================
//
// Purpose: Saved VA from previous exception (for context switching)
// Updated: During SWPCTX (context switch) PAL call
// Read by: SWPCTX PAL handler when restoring context
// Scope: Per-process saved state
//
// When updated:
//   - SWPCTX saves current IPRs.va -> outgoing HWPCB.va_fault
//   - SWPCTX loads incoming HWPCB.va_fault -> IPRs.va
//
// Example:
//   Process A has pending fault at 0x10000:
//     1. IPRs.va currently = 0x10000
//     2. SWPCTX to process B:
//        - Save: IPRs.va (0x10000) -> HWPCB_A.va_fault
//        - Load: HWPCB_B.va_fault (0x30000) -> IPRs.va
//     3. Process B now has IPRs.va = 0x30000

// ============================================================================
// 3. Data Flow Diagram
// ============================================================================
//
//   EXCEPTION DELIVERY (this architecture):
//   ─────────────────────────────────────────
//   Pipeline detects fault at VA 0x20000
//           |
//   Create PendingEvent with faultVA = 0x20000
//           |
//   preparePendingEventForDelivery()
//       IPRs.va = 0x20000          <- Update IPR only
//       HWPCB.va_fault unchanged   <- Do NOT touch this
//           |
//   PAL handler reads IPRs.va (0x20000)
//
//
//   CONTEXT SWITCH (PAL SWPCTX handler):
//   ─────────────────────────────────────────
//   Process A: IPRs.va = 0x10000 (active)
//              HWPCB_A.va_fault = 0xXXXX (old/stale)
//           |
//   SWPCTX from A to B
//       Save context A:
//           HWPCB_A.va_fault = IPRs.va  <- Save to HWPCB
//       Restore context B:
//           IPRs.va = HWPCB_B.va_fault  <- Load from HWPCB
//           |
//   Process B: IPRs.va = 0x30000 (active)
//              HWPCB_B.va_fault = 0x30000 (saved)

// ============================================================================
// 4. CORRECT Usage in Exception Delivery
// ============================================================================
inline void saveFaultVirtualAddress(IPRStorage& iprs, quint64 faultVA) noexcept
{
	if (faultVA != 0) {
		iprs.va = faultVA;  // x Update IPR only
	}
}
*/
