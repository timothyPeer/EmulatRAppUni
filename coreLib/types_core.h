#ifndef TYPESCORE_H
#define TYPESCORE_H

#include <QtGlobal>


// ============================================================================
// Alpha EV6 Core Types
// ============================================================================

// -----------------------------------------------------------------------------
// Address Space Number (ASN)
// EV6 uses 8-bit ASNs (0..255).  Architectural maximum is 255.
// -----------------------------------------------------------------------------
using ASNType = quint16;

static constexpr ASNType ASN_INVALID = 0;
static constexpr ASNType ASN_MAX = 256;
static constexpr qint32 MAX_CPUS = 64;


using IPLType = quint8;
// ============================================================================
// VAType: Canonical Virtual Address Representation for EV6
// ----------------------------------------------------------------------------
// VAType is a 64-bit value representing a virtual address as seen by the
// Alpha EV6 architecture.  Implementations of EV6 typically support
// 43-48 bits of virtual address space, but virtual addresses are always
// treated as 64-bit sign-extended quantities in:
//
//    - Software page tables
//    - PALcode sequences
//    - ITB/DTB tablewalk logic
//    - Translation and protection checks
//    - Per-CPU context-switch operations
//
// The virtual address is separated into:
//
//    - VPN      = VA >> PAGE_SHIFT
//    - offset   = VA & PAGE_OFFSET_MASK
//    - region   (implicitly from high bits, used in region-class checks)
//    - mode     (user/kernel, derived via PS and region mapping)
//
// VAType provides the raw address from which VPN extraction, region
// testing, canonicality checks, and alignment validation are performed.
//
// VAType is always 64 bits, even if hardware uses fewer virtual bits,
// to provide correct sign-extension, uniform TLB hashing, and safe
// manipulation across SMP CPUs and PAL-mode transitions.
// ============================================================================
using VAType = quint64;


// -----------------------------------------------------------------------------
// Physical Address (PA)
// EV6 supports up to 44-bit physical addresses (16 TB).
// PA is always represented in a 64-bit container for simplicity.
// -----------------------------------------------------------------------------
using PAType = quint64;


// -----------------------------------------------------------------------------
// Virtual Page Number (VPN)
// For EV6 base 8 KB pages:
//   VPN = VA >> 13
// Storing VPN in quint64 avoids overflow for large VA spaces.
// -----------------------------------------------------------------------------
using VPNType = quint64;


using FPCRType = quint64;

// ============================================================================
// TagType: Canonical TLB Tag Representation for EV6
// ----------------------------------------------------------------------------
// TagType is a 64-bit value that encodes the identifying fields used to
// match a TLB entry during virtual-address translation.  Each TLB entry
// in the EV6 SPAM-based design is uniquely determined by the combination
// of:
//
//    (1) VPN       : Virtual Page Number (VA >> PAGE_SHIFT)
//    (2) ASN       : Address Space Number (8-bit EV6 ASN)
//    (3) realm     : I-stream or D-stream (Instruction/Data realm select)
//    (4) gh        : Encoded Granularity Hint / superpage class
//
// The Tag is maintained as a single 64-bit quantity for consistency
// across:
//
//    - SPAMBucket key comparisons
//    - Hash partitioning in SPAMShardManager
//    - TLB lookup(), insert(), and invalidation routines
//    - PALcode-driven TBIS/TBIA/TBCHK operations
//    - SMP-wide invalidations and ASN rollover
//
// Although only lower bits are architecturally meaningful (VPN width,
// sizeClass, realm bit, ASN), the Tag is stored in a 64-bit container
// to provide stable hashing, correct wrap behavior, and alignment with
// the internal EV6 tablewalk microarchitecture.
//
// TagType is the canonical "key" used by the emulator for all TLB
// operations.
// ============================================================================
using TAGType = quint64;

// -----------------------------------------------------------------------------
// Page Frame Number (PFN)
// EV6 allows up to 28 PFN bits (bits 59..32 in the PTE).
// PFN is always stored in a 64-bit container for uniformity.
// -----------------------------------------------------------------------------
using PFNType = quint64;

// PFN Bit Structure

static constexpr int PFN_WIDTH = 28;
static constexpr unsigned PFN_BITS = 32;      // Now parameterized
static constexpr unsigned PFN_SHIFT = 32;      // EV6 default

// -----------------------------------------------------------------------------
// Page geometry (EV6 standard 8 KB pages)
// -----------------------------------------------------------------------------
// For Alpha (8KB pages) - EV4/EV5/EV6 all use 8 KB minimum page size
constexpr int PAGE_SHIFT = 13;                     // Number of bits for page offset
static quint64 PAGE_SIZE = 1ULL << PAGE_SHIFT;     // 8192 bytes
static quint64 PAGE_OFFSET_MASK = PAGE_SIZE - 1;     // 0x1FFF (13 bits)

// CM Type

using CMType = quint8;

// CPUId Type
using CPUIdType = quint8;
// constants
static constexpr CPUIdType CPU_ID_INVALID = 0xFF;

// PTE

using PTEType = quint64;

// SizeClass 
using SC_Type = quint8;

#endif // types_core_h__
