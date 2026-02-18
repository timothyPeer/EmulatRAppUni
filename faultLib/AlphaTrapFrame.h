/**
 * @file AlphaTrapFrame.h
 * @brief Exception trap frame (CPU-local, non-materialized)
 *
 * Captures the precise machine state at the moment of an exception.
 * Stored in CPU-local trap frame stack (depth 4); only materialized to
 * kernel memory when crossing PAL->OS boundary.
 *
 * Reference: ASA Vol I, Section 6.4 (Exception Frame)
 */

#pragma once

#include <QtGlobal>
#include <QString>
#include <cstring>
#include "enum_header.h"
#include "TrapsAndFaults_inl_helpers.h"
#include "types_core.h"
#include "../mmuLib/mmuLib_core.h"
#include "../exceptionLib/ExceptionClass_EV6.h"

 // =============================================================================
 // TRAP FRAME STRUCTURE
 // =============================================================================

 /**
  * @brief CPU-local exception trap frame
  *
  * Captures precise machine state at exception entry.
  * Small (approx. 128 bytes), stored in CPU-local stack (depth 4).
  *
  * Fields are grouped by logical section for clarity and cache efficiency.
  */
struct AlphaTrapFrame {
    // =========================================================================
    // EXECUTION CONTEXT (from faulting instruction)
    // =========================================================================
    quint64 pc;              // Program counter at fault (VA)
    quint64 npc;             // Next PC (if prefetch stage precomputed it; else pc+4)
    quint64 instr;           // Raw instruction word (or 0 if not available)
    quint64 vector;          // Exception Vector
    quint8  mode;
    AccessKind access;
    // =========================================================================
    // PRIVILEGE & MODE STATE
    // =========================================================================
    PrivilegeLevel priv;     // Privilege level at fault (kernel/exec/super/user)
    ASNType asn;             // Address Space Number
    quint8  ipl;             // Interrupt Priority Level (from PS[4:0])
    quint8  pad1;            // Padding for alignment

    // =========================================================================
    // FAULT SYNDROME
    // =========================================================================
    VAType faultVA;         // Faulting virtual address (for TLB/MMU faults)
    PAType faultPA;         // Faulting physical address (if known; else 0)

    MMUOperation mmuOp;      // Operation type (FETCH, READ, WRITE, PROBE)
    quint8 accessSize;       // Access size in bytes (1, 2, 4, 8, 16)
    quint8 pad2;             // Padding
    quint8 pad3;             // Padding

    FaultCause faultCause;   // Specific fault code (fine-grained diagnosis)
    ExceptionClass_EV6 exceptionClass; // Exception category (coarse-grained)

    // =========================================================================
    // TLB / MMU STATE AT FAULT
    // =========================================================================
    bool tlbHit;             // Was there a TLB hit? (may have failed permission check)
    bool writable;           // Is page marked writable?
    bool cow;                // Is page marked copy-on-write?
    bool globalPage;         // Is page global (ASN-independent)?

    // =========================================================================
    // BOOKKEEPING / ORDERING
    // =========================================================================
    quint64 trapId;          // Monotonically increasing trap ID (for verification)
    quint64 cycleStamp;      // Cycle count or timestamp when trapped
    quint64 pad4;            // Padding for future use

    // =========================================================================
    // DEFAULT CONSTRUCTOR
    // =========================================================================
    AlphaTrapFrame() noexcept
        : pc(0), npc(0), instr(0),
        priv(PrivilegeLevel::KERNEL), asn(0), ipl(0), pad1(0),
        faultVA(0), faultPA(0),
        mmuOp(MMUOperation::READ), accessSize(0), pad2(0), pad3(0),
        faultCause(FaultCause::FAULT_UNKNOWN), exceptionClass(ExceptionClass_EV6::None),
        tlbHit(false), writable(false), cow(false), globalPage(false),
        trapId(0), cycleStamp(0), pad4(0)
    {
    }

    // =========================================================================
    // PARAMETERIZED CONSTRUCTOR
    // =========================================================================
    AlphaTrapFrame(quint64 pc_, quint64 npc_, ExceptionClass_EV6 cat,
        quint64 faultVA_, PrivilegeLevel priv_, quint16 asn_,
        quint8 ipl_, MMUOperation op_, quint8 size_,
        FaultCause cause_) noexcept
        : pc(pc_), npc(npc_), instr(0),
        priv(priv_), asn(asn_), ipl(ipl_), pad1(0),
        faultVA(faultVA_), faultPA(0),
        mmuOp(op_), accessSize(size_), pad2(0), pad3(0),
        faultCause(cause_), exceptionClass(cat),
        tlbHit(false), writable(false), cow(false), globalPage(false),
        trapId(0), cycleStamp(0), pad4(0)
    {
    }


   

    // =========================================================================
    // NESTING DECISION
    // =========================================================================

    /**
     * @brief Can a new exception with higher IPL safely nest on this trapframe?
     *
     * Implements nesting rule: new IPL must be strictly higher than current PAL IPL
     * to safely preempt. This matches real Alpha firmware practice.
     *
     * @param incomingIPL IPL of the new exception
     * @return true if nesting is safe, false if would violate priority
     */
    bool isSafeToNest(quint8 incomingIPL) const noexcept {
        // Incoming must be strictly higher than current
        return incomingIPL > ipl;
    }

    // =========================================================================
    // DIAGNOSTIC / LOGGING
    // =========================================================================

  

    // =========================================================================
    // UTILITY: COPY WITH UPDATED FIELDS (for refinement during PAL handling)
    // =========================================================================

    /**
     * @brief Create a copy with updated fault information
     * Used by PAL handlers to refine fault diagnosis
     */
    AlphaTrapFrame withUpdatedFault(FaultCause newCause) const noexcept {
        AlphaTrapFrame copy = *this;
        copy.faultCause = newCause;
        return copy;
    }

    /**
     * @brief Create a copy with TLB hit information
     */
    AlphaTrapFrame withTLBInfo(bool hit, bool writable_, bool cow_, bool global_) const noexcept {
        AlphaTrapFrame copy = *this;
        copy.tlbHit = hit;
        copy.writable = writable_;
        copy.cow = cow_;
        copy.globalPage = global_;
        return copy;
    }
};

// =============================================================================
// STATIC ASSERTIONS (compile-time size check)
// =============================================================================

// Ensure trapframe is reasonably sized for stack storage
static_assert(sizeof(AlphaTrapFrame) <= 256, "TrapFrame too large for efficient storage");


