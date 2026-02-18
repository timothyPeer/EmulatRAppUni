#ifndef IRQ_AST_AND_SCB_HELPERS_H
#define IRQ_AST_AND_SCB_HELPERS_H
// ============================================================================
// IrqAstAndScbHelpers.h  (header-only, ASCII clean)
// ============================================================================
//
// PURPOSE
//   1) updateAstEligibility(): cold-path AST gating (ASTSR/ASTEN/CM/IPL)
//   2) VMS SCB stackDisposition helpers: mask low 2 bits, decode disposition
//
// REFERENCES (ASA / Alpha SRM)
//   - Alpha AXP System Reference Manual (SRM) v6 (1994)
//     Chapter "Interrupts and Exceptions" (AST gating described by OS policy)
//     Note: ASTEN/ASTSR are PAL-managed per-OS conventions; SRM defines IPL/CM.
//   - OpenVMS Alpha SCB convention:
//     SCB entry is a quadword: handler address with low two bits as disposition.
//     00 kernel stack, 01 interrupt stack, 10 no frame, 11 reserved.
//
// DESIGN NOTES
//   - Your router/pending design already models AST as an IRQ source
//     (see IrqSource::AST and InterruptRouter::raiseAST/clearAST).
//   - This helper keeps AST eligibility logic out of hot path.
//   - For SCB disposition: implement the masking now (required), and leave
//     a TODO for ISP stack selection until you add an ISP field.
//
// ============================================================================

#include <QtGlobal>
#include "InterruptRouter.h"
#include "IRQ_SourceId_core.h"

// ----------------------------------------------------------------------------
// Mode encoding (matches your HWPCB::cm and PS[1:0])
// 0 = Kernel (most privileged)
// 1 = Executive
// 2 = Supervisor
// 3 = User (least privileged)
// ----------------------------------------------------------------------------
enum class AlphaCurrentMode : quint8
{
    Kernel     = 0,
    Executive  = 1,
    Supervisor = 2,
    User       = 3
};

// ----------------------------------------------------------------------------
// AST gating utilities
// ----------------------------------------------------------------------------
//
// Contract:
//   Alpha ASTs are deliverable only when ALL are true:
//
//   (1) ASTSR has a pending bit set for some mode M
//   (2) ASTEN has enable bit set for that same mode M
//   (3) Current mode CM is equal or less privileged than M
//       (numeric compare: CM >= M, since Kernel=0 .. User=3)
//   (4) IPL <= 2
//
// Delivery priority:
//   When multiple modes have deliverable ASTs, the MOST PRIVILEGED mode
//   is delivered first (Kernel before Executive before Supervisor before User).
//   This matches Alpha architectural intent: kernel ASTs (I/O completion)
//   take precedence over user ASTs (timer, process deletion).
//
// After delivery:
//   The caller must clear the ASTSR bit for the delivered mode:
//     iprHot->astsr &= ~(1u << result.targetMode)
//   Then call updateAstEligibility() again to refresh pending state
//   (another mode's AST may now be deliverable).
//
// NOTE:
//   - This function does not modify ASTSR or ASTEN; those are PAL/OS state.
//   - Clearing AST in pending-state is a delivery/arbitration decision only.
// ----------------------------------------------------------------------------

struct AstEligibilityResult final
{
    bool   eligible{ false };
    quint8 targetMode{ 0xFF };   // 0..3 when eligible; use to clear ASTSR after delivery
    quint8 reasonMask{ 0 };      // diagnostics bitfield (optional)
};

// Diagnostic reason bits (optional)
static constexpr quint8 AST_REASON_HAS_PENDING = 0x01;
static constexpr quint8 AST_REASON_ENABLED     = 0x02;
static constexpr quint8 AST_REASON_MODE_OK     = 0x04;
static constexpr quint8 AST_REASON_IPL_OK      = 0x08;

// Extract bit for a mode from ASTEN/ASTSR packed nibbles
// Bit 0 = kernel, bit 1 = exec, bit 2 = super, bit 3 = user
static inline bool astBit(quint8 nibble, quint8 mode) noexcept
{
    return ((nibble >> (mode & 0x3)) & 0x1u) != 0;
}

// Pure function: evaluate AST gating conditions.
// Scans from MOST PRIVILEGED (Kernel=0) to LEAST (User=3).
// Returns the first (most privileged) eligible mode.
//
static inline AstEligibilityResult computeAstEligibility(
    quint8 astenNibble,
    quint8 astsrNibble,
    quint8 currentMode,
    quint8 currentIpl) noexcept
{
    AstEligibilityResult out{};

    // Gate: IPL must be <= 2 for any AST delivery
    if (currentIpl > 2)
    {
        out.eligible   = false;
        out.reasonMask = 0;
        return out;
    }

    // Scan from most privileged (Kernel=0) to least privileged (User=3).
    // Deliver the most privileged eligible AST first.
    // Kernel ASTs (I/O completion) take precedence over User ASTs (timers).
    for (quint8 mode = 0; mode <= 3; ++mode)
    {
        const bool pending = astBit(astsrNibble, mode);
        const bool enabled = astBit(astenNibble, mode);
        const bool modeOk  = (currentMode >= mode); // CM >= target (equal or less privileged)

        quint8 reasons = AST_REASON_IPL_OK;
        if (pending) reasons |= AST_REASON_HAS_PENDING;
        if (enabled) reasons |= AST_REASON_ENABLED;
        if (modeOk)  reasons |= AST_REASON_MODE_OK;

        if (pending && enabled && modeOk)
        {
            out.eligible   = true;
            out.targetMode = mode;
            out.reasonMask = reasons;
            return out;
        }
    }

    out.eligible   = false;
    out.targetMode = 0xFF;
    out.reasonMask = 0;
    return out;
}

// Main helper that wires eligibility to your IRQ pending model.
//
// Call after any change to AST gating inputs:
//   - MTPR_ASTEN  (enable mask changed)
//   - MTPR_ASTSR  (pending mask changed)
//   - CM change   (mode switch, CHMK/CHME/CHMS/CHMU, REI)
//   - IPL change  (SWPIPL, MTPR_IPL, REI, DI/EI)
//   - SWPCTX      (all gating inputs reload from new HWPCB)
//
// This function raises or clears the AST source in IRQPendingState.
// It does NOT modify ASTEN, ASTSR, or any IPR. The caller is responsible
// for IPR mutations (e.g., clearing ASTSR bit after delivery).
//
static inline AstEligibilityResult updateAstEligibility(
    InterruptRouter& router,
    int cpuId,
    quint8 astenNibble,   // ASTEN (your HWPCB::aster low 4 bits)
    quint8 astsrNibble,   // ASTSR (your HWPCB::astsr low 4 bits)
    quint8 currentMode,   // HWPCB::cm (0=K, 1=E, 2=S, 3=U)
    quint8 currentIpl) noexcept
{
    const AstEligibilityResult r =
        computeAstEligibility(astenNibble, astsrNibble, currentMode, currentIpl);

    if (r.eligible)
        router.raiseAST(cpuId);
    else
        router.clearAST(cpuId);

    return r;
}

// ============================================================================
// VMS SCB stack disposition helpers
// ============================================================================
//
// OpenVMS convention:
//   SCB entry is a quadword. Low 2 bits are disposition, not address.
//     00  push frame on KSP  (kernel stack)     -- normal exceptions, syscalls
//     01  push frame on ISP  (interrupt stack)   -- device interrupts
//     10  do not push frame  (direct entry)      -- special vectors
//     11  reserved
//
// IMPORTANT:
//   The low 2 bits MUST be masked off the handler PC before jumping to it.
//   Even if ISP is not implemented, the masking is required for correct
//   instruction fetch.
//
// TODO (future, VMS device interrupts):
//   - Add an interrupt stack pointer "isp" per CPU (PALtemp or IPR field).
//   - When disposition is 01, push the frame onto ISP, not KSP.
//   - SRM bring-up does not require ISP; all SRM SCB entries use disposition 00.
//
// ============================================================================

enum class ScbStackDisposition : quint8
{
    KernelStack    = 0,   // 00
    InterruptStack = 1,   // 01
    NoFrame        = 2,   // 10
    Reserved       = 3    // 11
};

struct ScbDecodedHandler final
{
    quint64             handlerPc{ 0 };
    ScbStackDisposition disp{ ScbStackDisposition::KernelStack };
};

// Decode an SCB entry quadword into a clean handler PC and disposition.
static inline ScbDecodedHandler decodeScbHandler(quint64 scbEntryQword) noexcept
{
    ScbDecodedHandler out{};
    const quint8 low2 = static_cast<quint8>(scbEntryQword & 0x3ULL);
    out.disp      = static_cast<ScbStackDisposition>(low2);
    out.handlerPc = scbEntryQword & ~0x3ULL;
    return out;
}

// Convenience: just mask low bits (when disposition is not yet needed)
static inline quint64 maskScbHandlerPc(quint64 scbEntryQword) noexcept
{
    return scbEntryQword & ~0x3ULL;
}

// Select which stack pointer to use for frame push based on SCB disposition.
//
// Returns:
//   0    = KSP (kernel stack)
//   1    = ISP (interrupt stack) -- currently falls back to KSP (TODO)
//   0xFF = no frame push (disposition 10)
//
static inline quint8 selectStackForDisposition(ScbStackDisposition disp) noexcept
{
    switch (disp)
    {
    case ScbStackDisposition::KernelStack:
        return 0; // KSP

    case ScbStackDisposition::InterruptStack:
        // TODO: Return ISP index once interrupt stack pointer is implemented.
        // For SRM bring-up this falls back to KSP safely.
        // For VMS device interrupts this MUST be corrected.
        return 0; // KSP (temporary fallback)

    case ScbStackDisposition::NoFrame:
        return 0xFF; // Caller must skip frame push

    case ScbStackDisposition::Reserved:
    default:
        // Safety fallback; a real implementation might raise a PAL fault.
        return 0; // KSP
    }
}

#endif // IRQ_AST_AND_SCB_HELPERS_H

