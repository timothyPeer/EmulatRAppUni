#ifndef EV6_ICTL_SHADOWHELPERS_H
#define EV6_ICTL_SHADOWHELPERS_H
#include "../coreLib/Axp_Attributes_core.h"
// -----------------------------------------------------------------------------
// EV6 / 21264 I_CTL (Ibox Control) helpers for PAL shadow registers.
//
// References:
//   - Alpha 21264 EV6 Specification Rev 2.0, Section 5.2.14 "I_CTL". [EV6-HRM-5.2.14]
//   - Alpha 21264 Microprocessor Data Sheet, Section 5.6 "PALshadow Registers". [EV6-DS-5.6]
//   - Compaq AlphaServer ES40 Service Guide, Appendix D, Tables D-7 and D-8. [ES40-SG-D.7/D.8]
//
// This header only defines bit positions and inline helpers. It does not
// define how I_CTL is stored; that is expected to live in AlphaProcessorContext
// or your IPR hot-path structure.
// -----------------------------------------------------------------------------


#include <QtGlobal>

// Raw I_CTL bit positions for fields we care about.
// See EV6 spec Section 5.2.14 "I_CTL". [EV6-HRM-5.2.14]

// Low-order fields:
static constexpr quint64 ICTL_SPCE_BIT         = 0;      // System perf counting enable.
static constexpr quint64 ICTL_IC_EN_SHIFT      = 1;      // IC_EN<1:0>.
static constexpr quint64 ICTL_IC_EN_MASK       = 0x3ULL << ICTL_IC_EN_SHIFT;

static constexpr quint64 ICTL_SPE_SHIFT        = 3;      // SPE<2:0>.
static constexpr quint64 ICTL_SPE_MASK         = 0x7ULL << ICTL_SPE_SHIFT;

// PALshadow Enable field SDE<1:0> at bits <7:6>.
static constexpr quint64 ICTL_SDE_SHIFT        = 6;
static constexpr quint64 ICTL_SDE_MASK         = 0x3ULL << ICTL_SDE_SHIFT;
static constexpr quint64 ICTL_SDE_GROUP0       = 0x1ULL << ICTL_SDE_SHIFT;      // EV6: R8-R11, R24-R27.
static constexpr quint64 ICTL_SDE_GROUP1       = 0x2ULL << ICTL_SDE_SHIFT;      // EV6: R4-R7, R20-R23.

// Istream stream buffer enable SBE<1:0> at bits <9:8>.
static constexpr quint64 ICTL_SBE_SHIFT        = 8;
static constexpr quint64 ICTL_SBE_MASK         = 0x3ULL << ICTL_SBE_SHIFT;

// Branch prediction mode BP_MODE<1:0> at bits <11:10>.
static constexpr quint64 ICTL_BP_MODE_SHIFT    = 10;
static constexpr quint64 ICTL_BP_MODE_MASK     = 0x3ULL << ICTL_BP_MODE_SHIFT;

// ... (other bit positions omitted for brevity; see EV6-HRM-5.2.14) ...

// Inline helpers for SDE field.
// -----------------------------------------------------------------------------
// NOTE: Architecturally, the PAL shadow registers are visible only when:
//   - The CPU is in PALmode, and
//   - The corresponding SDE bit is set in I_CTL.
//
// See Alpha 21264 Data Sheet, Section 5.6 "PALshadow Registers". [EV6-DS-5.6]
// -----------------------------------------------------------------------------

AXP_FLATTEN
    quint8 ictl_getSDE(quint64 ictlValue) noexcept
{
    // Returns raw SDE<1:0> bits (0..3).
    return static_cast<quint8>((ictlValue & ICTL_SDE_MASK) >> ICTL_SDE_SHIFT);
}

AXP_FLATTEN
    bool ictl_isShadowGroup0Enabled(quint64 ictlValue) noexcept
{
    // EV6 spec: SDE<0> enables PALshadow on R8-R11 and R24-R27. [EV6-HRM-5.2.14]
    // ES40 hardware note: SDE<0> may be ignored. [ES40-SG-D.8]
    return (ictlValue & ICTL_SDE_GROUP0) != 0;
}

AXP_FLATTEN
    bool ictl_isShadowGroup1Enabled(quint64 ictlValue) noexcept
{
    // EV6 spec: SDE<1> enables PALshadow on R4-R7 and R20-R23. [EV6-HRM-5.2.14]
    return (ictlValue & ICTL_SDE_GROUP1) != 0;
}

AXP_FLATTEN
    quint64 ictl_setSDE(quint64 ictlValue, quint8 sdeBits) noexcept
{
    // Overwrite SDE<1:0> with the given value (low 2 bits of sdeBits).
    // Caller is responsible for ensuring this is only done in PALcode,
    // via the MFPR/MTPR emulation of I_CTL. [EV6-HRM-5.2.14]
    quint64 newBits = static_cast<quint64>(sdeBits & 0x3U) << ICTL_SDE_SHIFT;
    ictlValue &= ~ICTL_SDE_MASK;
    ictlValue |= newBits;
    return ictlValue;
}

AXP_FLATTEN
    quint64 ictl_enableShadowGroup1(quint64 ictlValue) noexcept
{
    // Set SDE<1>. This is the PALshadow bank that overlays R4-R7 and R20-R23.
    return ictlValue | ICTL_SDE_GROUP1;
}

AXP_FLATTEN
    quint64 ictl_disableAllShadow(quint64 ictlValue) noexcept
{
    // Clear both SDE bits. PALshadow is disabled. [EV6-DS-5.6]
    return ictlValue & ~ICTL_SDE_MASK;
}

#endif // EV6_ICTL_SHADOWHELPERS_H
