#pragma once
#include <QtGlobal>
#include "../coreLib/types_core.h"
#include "../coreLib/IprStorage_Hot.h"
//#include "../coreLib/globalIPR_hot_cold.h"
#include "../coreLib/globalIPR_hot_cold_new.h"

// ============================================================================
// FPCR - Canonical IPR Architecture
// ============================================================================
// FPCR is stored as quint64 in the IPR bank (globalIPRHot).
// The FpcrRegister class provides a structured view for manipulation.
//
// Usage:
//   1. Direct access: globalIPRHot64(cpuId).fpcr (raw quint64)
//   2. Structured access: FPCR::get(cpuId) returns FpcrRegister wrapper
//   3. Instruction implementation: MF_FPCR/MT_FPCR read/write raw value
//
//IPRStorage_Hot& globalIPRHot64(CPUIdType cpuId) noexcept;

// ============================================================================
// FpcrRegister - Structured View/Wrapper
// ============================================================================
// This class provides structured access to FPCR bits.
// It does NOT own the storage - it's a temporary view.
//

class FpcrRegister
{
public:
    using u64 = quint64;

    // ---------------------------------------------------------------------
    // Constructor - Initialize with current FPCR value
    // ---------------------------------------------------------------------
    explicit FpcrRegister(u64 value = 0) noexcept : m_value(value) {}

    // Reset to architectural default
    inline void reset() noexcept {
        m_value = 0;  // Round-to-nearest, all traps disabled
    }

    // Raw access
    inline u64 raw() const noexcept { return m_value; }
    inline void setRaw(u64 v) noexcept { m_value = v; }

    // ---------------------------------------------------------------------
    // Rounding Mode (bits 58:59)
    // ---------------------------------------------------------------------
    enum class RoundingMode : quint8 {
        ToNearest = 0,  // 00 - default
        Upward = 1,  // 01
        Downward = 2,  // 10
        TowardZero = 3   // 11
    };

    inline RoundingMode roundingMode() const noexcept {
        return static_cast<RoundingMode>((m_value >> 58) & 0x3u);
    }

    inline void setRoundingMode(RoundingMode rm) noexcept {
        m_value &= ~(0x3ull << 58);
        m_value |= (static_cast<u64>(rm) & 0x3ull) << 58;
    }

    // ---------------------------------------------------------------------
    // Exception Flags (bits 49-53)
    // ---------------------------------------------------------------------
    enum class FpFlag : quint8 {
        InvalidOp = 0,  // IV - bit 49
        DivZero = 1,  // DZ - bit 50
        Overflow = 2,  // OV - bit 51
        Underflow = 3,  // UN - bit 52
        Inexact = 4   // IN - bit 53
    };

    static constexpr u64 FlagBaseBit = 49;

    inline bool getFlag(FpFlag f) const noexcept {
        return (m_value >> (FlagBaseBit + static_cast<u64>(f))) & 1ull;
    }

    inline void setFlag(FpFlag f, bool v = true) noexcept {
        const u64 mask = 1ull << (FlagBaseBit + static_cast<u64>(f));
        if (v) m_value |= mask;
        else   m_value &= ~mask;
    }

    inline void clearFlag(FpFlag f) noexcept {
        setFlag(f, false);
    }

    inline void raise(FpFlag f) noexcept {
        setFlag(f, true);
    }

    // ---------------------------------------------------------------------
    // Software Detect Enable (bit 48)
    // ---------------------------------------------------------------------
    static constexpr u64 SDE_Bit = 48;

    inline bool softwareDetectEnabled() const noexcept {
        return (m_value >> SDE_Bit) & 1ull;
    }

    inline void setSoftwareDetectEnabled(bool v) noexcept {
        if (v) m_value |= (1ull << SDE_Bit);
        else   m_value &= ~(1ull << SDE_Bit);
    }

    // ---------------------------------------------------------------------
    // Exception Status Queries
    // ---------------------------------------------------------------------
    inline bool invalidOpFault()  const noexcept { return getFlag(FpFlag::InvalidOp); }
    inline bool divZeroFault()    const noexcept { return getFlag(FpFlag::DivZero); }
    inline bool overflowFault()   const noexcept { return getFlag(FpFlag::Overflow); }
    inline bool underflowFault()  const noexcept { return getFlag(FpFlag::Underflow); }
    inline bool inexactFault()    const noexcept { return getFlag(FpFlag::Inexact); }

    inline bool anyFault() const noexcept {
        return (m_value >> FlagBaseBit) & 0x1FULL;  // Any of bits 49-53 set
    }

    // ---------------------------------------------------------------------
    // Extract exception summary for EXC_SUM register
    // ---------------------------------------------------------------------
    inline u64 getExceptionSummary() const noexcept {
        return (m_value >> FlagBaseBit) & 0x1FULL;  // Bits 49-53
    }

    // ---------------------------------------------------------------------
    // Trap determination
    // ---------------------------------------------------------------------
    inline bool shouldTrapOn(FpFlag f) const noexcept {
        return getFlag(f) && softwareDetectEnabled();
    }

    inline bool shouldTrap() const noexcept {
        return anyFault() && softwareDetectEnabled();
    }

    // ---------------------------------------------------------------------
    // Clear all exception flags
    // ---------------------------------------------------------------------
    inline void clearAllFaults() noexcept {
        m_value &= ~(0x1FULL << FlagBaseBit);  // Clear bits 49-53
    }

private:
    u64 m_value;
};

// ============================================================================
// FPCR Canonical Access Functions
// ============================================================================
// These functions access FPCR in the IPR bank and provide structured access.
//

namespace FPCR {

    /**
     * @brief Get FPCR as structured register (creates view)
     * @param cpuId CPU index
     * @return FpcrRegister view of current FPCR value
     */
    AXP_FLATTEN quint64 get(CPUIdType cpuId) noexcept {
        return globalIPRHot64(cpuId).fpcr;
    }

    /**
     * @brief Set FPCR from structured register
     * @param cpuId CPU index
     * @param fpcr FpcrRegister to write back
     */
    inline void set(CPUIdType cpuId, const FpcrRegister& fpcr) noexcept {
        globalIPRHot64(cpuId).fpcr = fpcr.raw();
    }

    /**
     * @brief Get raw FPCR value
     * @param cpuId CPU index
     * @return Raw 64-bit FPCR value
     */
    inline quint64 getRaw(CPUIdType cpuId) noexcept {
        return globalIPRHot64(cpuId).fpcr;
    }

    /**
     * @brief Set raw FPCR value
     * @param cpuId CPU index
     * @param value Raw 64-bit FPCR value
     */
    inline void setRaw(CPUIdType cpuId, quint64 value) noexcept {
        globalIPRHot64(cpuId).fpcr = value;
    }

    /**
     * @brief Modify FPCR in-place using lambda
     * @param cpuId CPU index
     * @param modifier Lambda that modifies FpcrRegister
     *
     * Example: FPCR::modify(cpuId, [](FpcrRegister& fpcr) {
     *              fpcr.raise(FpcrRegister::FpFlag::InvalidOp);
     *          });
     */
    template<typename F>
    inline void modify(CPUIdType cpuId, F&& modifier) noexcept {
        FpcrRegister fpcr(globalIPRHot64(cpuId).fpcr);
        modifier(fpcr);
        globalIPRHot64(cpuId).fpcr = fpcr.raw();
    }

    /**
     * @brief Get exception summary for EXC_SUM register
     * @param cpuId CPU index
     * @return Exception summary bits (bits 49-53)
     */
    inline quint64 getExceptionSummary(CPUIdType cpuId) noexcept {
        return (globalIPRHot64(cpuId).fpcr >> 49) & 0x1FULL;
    }

    /**
     * @brief Check if any FP exception is pending
     * @param cpuId CPU index
     * @return true if any exception flag is set
     */
    inline bool anyException(CPUIdType cpuId) noexcept {
        return (globalIPRHot64(cpuId).fpcr >> 49) & 0x1FULL;
    }

    /**
     * @brief Check if FP trap should be taken
     * @param cpuId CPU index
     * @return true if exception pending AND SDE enabled
     */
    inline bool shouldTrap(CPUIdType cpuId) noexcept {
        quint64 fpcr = globalIPRHot64(cpuId).fpcr;
        bool hasException = (fpcr >> 49) & 0x1FULL;
        bool sdeEnabled = (fpcr >> 48) & 1ULL;
        return hasException && sdeEnabled;
    }

} // namespace FPCR

// ============================================================================
// Usage Examples
// ============================================================================

// Example 1: MF_FPCR instruction (move from FPCR)
inline quint64 executeMF_FPCR(CPUIdType cpuId) noexcept {
    return FPCR::getRaw(cpuId);
}

// Example 2: MT_FPCR instruction (move to FPCR)
inline void executeMT_FPCR(CPUIdType cpuId, quint64 value) noexcept {
    FPCR::setRaw(cpuId, value);
}

// Example 3: Floating-point instruction raises exception
inline void raiseInvalidOperation(CPUIdType cpuId) noexcept {
    FPCR::modify(cpuId, [](FpcrRegister& fpcr) {
        fpcr.raise(FpcrRegister::FpFlag::InvalidOp);
        });
}

// Example 4: Check if trap needed after FP operation
inline bool checkFPTrap(CPUIdType cpuId) noexcept {
    return FPCR::shouldTrap(cpuId);
}

// Example 5: Update EXC_SUM with FPCR exceptions
inline void updateExcSumFromFPCR(CPUIdType cpuId) noexcept {
    auto& iprs = globalIPRHotExt(cpuId);
    quint64 fpSummary = FPCR::getExceptionSummary(cpuId);
    iprs.exc_sum |= fpSummary;  // Merge FP exceptions into EXC_SUM
}

// Example 6: Complex modification with structured access
inline void setRoundingModeAndClearFaults(CPUIdType cpuId, quint8 mode) noexcept {
    FPCR::modify(cpuId, [mode](FpcrRegister& fpcr) {
        fpcr.setRoundingMode(static_cast<FpcrRegister::RoundingMode>(mode));
        fpcr.clearAllFaults();
        });
}
