#pragma once
#include <QtGlobal>
#include "enum_MCES.h"

/*
    Machine Check Error Summary (MCES) helper
    -----------------------------------------

    Architectural intent (Alpha AXP System Reference Manual,
    chapter "Exceptions, Interrupts, and Machine Checks",
    section "Machine Check Error Summary (MCES)"):

      - MCES records summary bits for machine check handling:
          * MIP  : Machine Check In Progress
          * WRE  : Write error summary
          * SCE  : System Correctable Error
          * PCE  : Processor Correctable Error
          * MME  : Machine Check Enable (control bit)
      - Software (PALcode / OS) typically:
          * Sets MIP and one or more summary bits when a machine
            check is taken.
          * Writes MCES to clear bits after handling.
      - The detailed syndrome is provided by other registers
        (e.g., platform-specific error syndrome IPRs), not by MCES.

    This helper does *not* try to model every implementation-specific
    MCES bit. It focuses on a portable subset that is useful for the
    emulator:

      - MIP:  latched when a machine check is raised.
      - SCE:  system-level correctable error.
      - PCE:  processor-local correctable error.
      - WRE:  write-side error indication.
      - MME:  preserved from the incoming value (enable mask).

    You can extend masks and the MachineCheckReason enum as you
    refine your EV6-specific modeling.
*/

// --------------------------------------------------------------------
// MCES bit positions (architectural subset)
// --------------------------------------------------------------------
static constexpr int MCES_BIT_MIP = 0;  // Machine check in progress
static constexpr int MCES_BIT_WRE = 1;  // Write error summary
static constexpr int MCES_BIT_SCE = 2;  // System correctable error
static constexpr int MCES_BIT_PCE = 3;  // Processor correctable error
static constexpr int MCES_BIT_MME = 7;  // Machine check enable (control)

// Masks
static constexpr quint64 MCES_MASK_MIP =
(quint64(1) << MCES_BIT_MIP);
static constexpr quint64 MCES_MASK_WRE =
(quint64(1) << MCES_BIT_WRE);
static constexpr quint64 MCES_MASK_SCE =
(quint64(1) << MCES_BIT_SCE);
static constexpr quint64 MCES_MASK_PCE =
(quint64(1) << MCES_BIT_PCE);
static constexpr quint64 MCES_MASK_MME =
(quint64(1) << MCES_BIT_MME);

// Convenience aggregate for summary bits (excluding MME)
static constexpr quint64 MCES_MASK_SUMMARY =
(MCES_MASK_MIP |
    MCES_MASK_WRE |
    MCES_MASK_SCE |
    MCES_MASK_PCE);

// ============================================================================
// MCES helpers (Machine Check Error Summary)
//
// Typical MCES bit usage (common Alpha practice, see ASA 13.3.9 and HRMs):
//   Bit 0  SCE  System correctable error occurred
//   Bit 1  PCE  Processor correctable error occurred
//   Bit 2  MCK  Machine check in progress
//   Bit 3  DPC  Disable processor correctable error reporting
//   Bit 4  DSC  Disable system correctable error reporting
//   Bit 5  DMK  Disable all machine checks
//
// Exact bit numbers can differ by implementation; if your HRM specifies
// different positions, adjust the SHIFT constants accordingly.
// ============================================================================

static constexpr quint64 MCES_SCE_SHIFT = 0;
static constexpr quint64 MCES_PCE_SHIFT = 1;
static constexpr quint64 MCES_MCK_SHIFT = 2;
static constexpr quint64 MCES_DPC_SHIFT = 3;
static constexpr quint64 MCES_DSC_SHIFT = 4;
static constexpr quint64 MCES_DMK_SHIFT = 5;

static constexpr quint64 MCES_FLAG_MASK = 0x1ULL;



// --------------------------------------------------------------------
// Small flag helper
// --------------------------------------------------------------------
inline quint64 mcesSetFlag(quint64 mces, quint64 mask, bool bValue) noexcept
{
    return bValue ? (mces | mask) : (mces & ~mask);
}

/*
    setMCESFields

    Purpose:
      Update the MCES register when a machine check is raised.

    Parameters:
      mces       - current MCES value (iprs.mces)
      reason     - high-level machine check classification
      mcDetails  - optional detail value (currently unused here;
                   reserved for future use if you want to encode
                   implementation-specific flags or a small syndrome)

    Semantics:
      - Preserves the MME (Machine Check Enable) bit from the incoming
        value.
      - Clears the summary bits (MIP, WRE, SCE, PCE).
      - Sets MIP and the appropriate summary bit(s) based on 'reason'.
      - Does not attempt to encode full syndrome information; that
        should live in separate error IPRs or platform-specific state.

    Typical usage in a machine check raiser:

        iprs.mces = setMCESFields(iprs.mces, reason, mcDetails);

    After PAL / OS has handled the machine check, it may clear MCES
    summary bits by writing to MCES via the emulated HW_MTPR path.
*/
inline quint64 setMCESFields(quint64      mces,
    MachineCheckReason reason,
    quint64      mcDetails /* reserved */)
{
    Q_UNUSED(mcDetails);

    // Preserve control bits (e.g., MME), clear summary bits.
    const quint64 preserved = (mces & MCES_MASK_MME);
    quint64 newMCES = preserved;

    switch (reason) {

    case MachineCheckReason::SYSTEM_CORRECTABLE_ERROR:
        // System-level correctable error (memory / bus).
        newMCES |= MCES_MASK_MIP;
        newMCES |= MCES_MASK_SCE;
        break;

    case MachineCheckReason::PROCESSOR_CORRECTABLE_ERROR:
        // CPU-local correctable error (cache / pipeline).
        newMCES |= MCES_MASK_MIP;
        newMCES |= MCES_MASK_PCE;
        break;

    case MachineCheckReason::BUFFER_WRITE_ERROR:
        // Write-side error (store path / write buffer).
        newMCES |= MCES_MASK_MIP;
        newMCES |= MCES_MASK_WRE;
        break;

    case MachineCheckReason::UNCORRECTABLE_ERROR:
        // Uncorrectable machine check: leave SCE/PCE/WRE clear and
        // just indicate "machine check in progress". Additional
        // syndrome must be obtained from other IPRs.
        newMCES |= MCES_MASK_MIP;
        break;

    case MachineCheckReason::UNKNOWN_MACHINE_CHECK:
        // Unknown classification: set MIP so PAL/OS knows a machine
        // check is active, but do not guess at SCE/PCE/WRE.
        newMCES |= MCES_MASK_MIP;
        break;

    case MachineCheckReason::NONE:
    default:
        // Clear summary bits and MIP, preserve only MME. This is
        // typically used when PAL / OS writes MCES to acknowledge
        // that handling is complete.
        // newMCES already equals 'preserved'.
        break;
    }

    return newMCES;
}
