// ============================================================================
// fp_variant_core.h
// Floating-point variant decoding and per-instruction FPCR shaping.
// ============================================================================

#ifndef FP_VARIANT_CORE_H
#define FP_VARIANT_CORE_H

#include "types_core.h"
#include "alpha_fpcr_core.h"
#include "grainFactoryLib/DecodedInstruction.h"
#include "faultLib/PendingEvent_Refined.h"


// ============================================================================
// FpRoundingMode - Note: lowercase 'p' to match your existing code
// ============================================================================
enum class FpRoundingMode : quint8
{
    UseFPCR,          // Use architectural FPCR rounding mode
    RoundToNearest,   // Round to nearest (ties to even)
    RoundTowardZero,  // Truncate (chop)
    RoundUp,          // Round toward +infinity
    RoundDown         // Round toward -infinity
};

// ============================================================================
// FPTrapMode - For new-style variant decoding (optional, for grains)
// ============================================================================
enum class FPTrapMode : quint8
{
    None = 0,  // No suffix - Use FPCR trap enables
    Underflow = 1,  // /U - Enable underflow trap
    Software = 2,  // /S - Software completion
    SU = 3,  // /SU - Software + underflow suppression
    SUI = 4   // /SUI - Software + underflow + inexact suppression
};

// ============================================================================
// FPVariant - UNIFIED STRUCTURE
// ============================================================================
struct FPVariant
{
    // High-level derived properties
    FpRoundingMode roundingMode{ FpRoundingMode::UseFPCR };
    FPTrapMode trapMode{ FPTrapMode::None };

    // Exception/trap control flags
    bool suppressUnderflow{ false };   // /SU: suppress underflow exception
    bool suppressInexact{ false };     // /SUI: suppress inexact exception
    bool maskExceptions{ false };      // /M: no traps, but set exception flags
    bool vaxDenorm{ false };           // /D: VAX denormal handling (legacy)

    // Individual variant bit flags (from function field)
    bool chopped{ false };             // /C bit - Round toward zero
    bool minusInf{ false };            // /M bit - Round toward -infinity
    bool dynamic{ false };             // /D bit - Use FPCR rounding mode
    bool underflow{ false };           // /U bit - Underflow trap enable
    bool overflow{ false };            // /V bit - Overflow trap enable
    bool software{ false };            // /S bit - Software completion
    bool inexact{ false };             // /I bit - Inexact trap enable
    bool trapEnabled{ true };          // General trap enable flag
    bool inexactEnable{ false };       // Alias for grain compatibility

    // Default constructor
    FPVariant() noexcept = default;

    // Constructor for grain-style usage
    FPVariant(FpRoundingMode rm, FPTrapMode tm, bool ie) noexcept
        : roundingMode(rm)
        , trapMode(tm)
        , inexactEnable(ie)
        , inexact(ie)
    {
        suppressUnderflow = (tm == FPTrapMode::SU || tm == FPTrapMode::SUI);
        suppressInexact = (tm == FPTrapMode::SUI);
    }

    // Helper: Get effective rounding mode
    inline FpRoundingMode getEffectiveRoundingMode() const noexcept
    {
        if (roundingMode != FpRoundingMode::UseFPCR)
            return roundingMode;

        // Derive from individual flags
        if (chopped)
            return FpRoundingMode::RoundTowardZero;
        if (minusInf)
            return FpRoundingMode::RoundDown;
        if (dynamic)
            return FpRoundingMode::UseFPCR;

        return FpRoundingMode::RoundToNearest;
    }

    // Helper: Check if software completion enabled
    inline bool hasSoftwareCompletion() const noexcept
    {
        return software || suppressUnderflow || suppressInexact;
    }

    // ========================================================================
    // FACTORY METHODS - IEEE S-format (Single Precision)
    // ========================================================================

    static inline FPVariant makeIEEE_S_Normal() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_Chopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_MinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_Dynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_Underflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_UnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_UnderflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_UnderflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowInexact() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowInexactChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowInexactMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_S_SoftwareUnderflowInexactDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    // ========================================================================
    // FACTORY METHODS - IEEE T-format (Double Precision)
    // ========================================================================

    static inline FPVariant makeIEEE_T_Normal() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_Chopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_MinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_Dynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_Underflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_UnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_UnderflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_UnderflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowInexact() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowInexactChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowInexactMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareUnderflowInexactDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    // ========================================================================
    // FACTORY METHODS - IEEE T-format Overflow Variants (for CVTTQ)
    // ========================================================================

    static inline FPVariant makeIEEE_T_Overflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.overflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_OverflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.overflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_OverflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.overflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_OverflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.overflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.overflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.overflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.overflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.overflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowInexact() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.overflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowInexactChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.overflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowInexactMinusInf() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundDown;
        v.minusInf = true;
        v.software = true;
        v.overflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeIEEE_T_SoftwareOverflowInexactDynamic() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::UseFPCR;
        v.dynamic = true;
        v.software = true;
        v.overflow = true;
        v.suppressInexact = true;
        v.inexact = true;
        v.trapEnabled = false;
        return v;
    }

    // ========================================================================
    // FACTORY METHODS - VAX F-format
    // ========================================================================

    static inline FPVariant makeVAX_F_Normal() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_F_Chopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_F_Underflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_F_UnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_F_SoftwareChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeVAX_F_SoftwareUnderflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeVAX_F_SoftwareUnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    // ========================================================================
    // FACTORY METHODS - VAX G-format
    // ========================================================================

    static inline FPVariant makeVAX_G_Normal() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_G_Chopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_G_Underflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_G_UnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.underflow = true;
        v.trapEnabled = true;
        return v;
    }

    static inline FPVariant makeVAX_G_SoftwareChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeVAX_G_SoftwareUnderflow() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    static inline FPVariant makeVAX_G_SoftwareUnderflowChopped() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundTowardZero;
        v.chopped = true;
        v.software = true;
        v.suppressUnderflow = true;
        v.trapEnabled = false;
        return v;
    }

    // ========================================================================
    // SPECIAL VARIANTS
    // ========================================================================

    static inline FPVariant makeIEEE_S_Software() noexcept {
        FPVariant v;
        v.roundingMode = FpRoundingMode::RoundToNearest;
        v.software = true;
        v.trapEnabled = false;
        return v;
    }
};

// ============================================================================
// extractFPVariantFromBits - Extract from raw instruction bits
// ============================================================================
inline FPVariant extractFPVariantFromBits(quint32 raw) noexcept
{
    FPVariant variant{};

    // Extract function field (bits 5-15 for FP operations)
    const quint16 func = (raw >> 5) & 0x7FF;

    // Rounding mode flags
    variant.chopped = (func & 0x400) != 0;     // /C bit (bit 10)
    variant.minusInf = (func & 0x200) != 0;     // /M bit (bit 9) 
    variant.dynamic = (func & 0x100) != 0;     // /D bit (bit 8)

    // Trap enable flags  
    variant.underflow = (func & 0x080) != 0;    // /U bit (bit 7)
    variant.overflow = (func & 0x040) != 0;    // /V bit (bit 6)
    variant.software = (func & 0x020) != 0;    // /S bit (bit 5)
    variant.inexact = (func & 0x010) != 0;    // /I bit (bit 4)
    variant.inexactEnable = variant.inexact;    // Alias

    // Derive high-level flags from individual bits
    if (variant.chopped)
        variant.roundingMode = FpRoundingMode::RoundTowardZero;
    else if (variant.minusInf)
        variant.roundingMode = FpRoundingMode::RoundDown;
    else if (variant.dynamic)
        variant.roundingMode = FpRoundingMode::UseFPCR;
    else
        variant.roundingMode = FpRoundingMode::RoundToNearest;

    // Software completion flags
    if (variant.software && variant.underflow)
        variant.suppressUnderflow = true;
    if (variant.software && variant.underflow && !variant.inexact)
        variant.suppressInexact = true;

    return variant;
}

// ============================================================================
// extractFunctionCode - Get function code from DecodedInstruction
// ============================================================================
inline quint16 extractFunctionCode(const DecodedInstruction& di) noexcept
{
    // Extract raw instruction bits from semantics field (high 32 bits)
    const quint32 rawBits = static_cast<quint32>(di.semantics >> 32);

    // Function code is bits 5-15 for FP instructions
    return static_cast<quint16>((rawBits >> 5) & 0x7FF);
}

// ============================================================================
// decodeVariant - For grain-style usage (function code bits 4:0)
// ============================================================================
inline FPVariant decodeVariant(quint16 functionCode) noexcept
{
    // Extract variant bits (bits 4:0 of function code)
    const quint8 variantBits = static_cast<quint8>(functionCode & 0x1F);

    // Bit 0: Inexact enable
    const bool inexactEnable = (variantBits & 0x01) != 0;

    // Bits 2:1: Rounding mode
    const quint8 roundBits = (variantBits >> 1) & 0x03;
    FpRoundingMode roundingMode;
    switch (roundBits)
    {
    case 0:  roundingMode = FpRoundingMode::RoundTowardZero; break;  // /C
    case 1:  roundingMode = FpRoundingMode::RoundDown; break;        // /M
    case 2:  roundingMode = FpRoundingMode::RoundToNearest; break;   // Normal
    case 3:  roundingMode = FpRoundingMode::UseFPCR; break;          // /D
    default: roundingMode = FpRoundingMode::RoundToNearest; break;
    }

    // Bits 4:3: Trap mode
    const quint8 trapBits = (variantBits >> 3) & 0x03;
    FPTrapMode trapMode;
    switch (trapBits)
    {
    case 0:  trapMode = FPTrapMode::None; break;
    case 1:  trapMode = FPTrapMode::Underflow; break;
    case 2:  trapMode = FPTrapMode::Software; break;
    case 3:  trapMode = inexactEnable ? FPTrapMode::SU : FPTrapMode::SUI; break;
    default: trapMode = FPTrapMode::None; break;
    }

    return FPVariant(roundingMode, trapMode, inexactEnable);
}

// ============================================================================
// decodeVariant - Overload for DecodedInstruction
// ============================================================================
inline FPVariant decodeVariant(const DecodedInstruction& di) noexcept
{
    const quint16 funcCode = extractFunctionCode(di);
    return decodeVariant(funcCode);
}

// ============================================================================
// deriveLocalFpcr - Derive FPCR for single operation
// ============================================================================
inline quint64 deriveLocalFpcr(quint64 fpcrArchitectural, const FPVariant& v) noexcept
{
    quint64 local = fpcrArchitectural;

    // Clear exception flags
    local &= ~AlphaFPCR::EXC_MASK;

    // Apply rounding mode override
    FpRoundingMode effectiveMode = v.getEffectiveRoundingMode();
    if (effectiveMode != FpRoundingMode::UseFPCR)
    {
        local &= ~AlphaFPCR::DYN_RM_MASK;

        quint64 rm = 0;
        switch (effectiveMode)
        {
        case FpRoundingMode::RoundToNearest:  rm = AlphaFPCR::RM_NORMAL;    break;
        case FpRoundingMode::RoundTowardZero: rm = AlphaFPCR::RM_CHOPPED;   break;
        case FpRoundingMode::RoundUp:         rm = AlphaFPCR::RM_PLUS_INF;  break;
        case FpRoundingMode::RoundDown:       rm = AlphaFPCR::RM_MINUS_INF; break;
        default: break;
        }
        local |= (rm << AlphaFPCR::DYN_RM_SHIFT);
    }

    return local;
}

// ============================================================================
// commitLocalFpcr, shouldRaiseFPTrap, getExceptionSummary
// ============================================================================
inline void commitLocalFpcr(CPUIdType cpuId, quint64 fpcrLocal) noexcept
{
    auto& iprs = globalFloatRegs(cpuId);
    quint64 localExceptions = fpcrLocal & AlphaFPCR::EXC_MASK;
    iprs.fpcr |= localExceptions;
}

inline bool shouldRaiseFPTrap(quint64 fpcrLocal, const FPVariant& variant) noexcept
{
    if (variant.maskExceptions)
        return false;

    quint64 exceptions = fpcrLocal & AlphaFPCR::EXC_MASK;
    if (exceptions == 0)
        return false;

    bool sdeEnabled = (fpcrLocal >> 48) & 1ULL;
    return sdeEnabled;
}

inline quint64 getExceptionSummary(quint64 fpcr) noexcept
{
    return (fpcr >> 49) & 0x1FULL;
}

#endif // FP_VARIANT_CORE_H