// ============================================================================
// IRQPendingState.h - ============================================================================
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef IRQ_PENDINGSTATE_H
#define IRQ_PENDINGSTATE_H
// ============================================================================
// IRQPendingState.h  (header-only, ASCII-128 clean)
// ============================================================================
// Per-CPU interrupt pending state and delivery arbitration.
//
// OWNERSHIP:
//   Owned by AlphaCPU. One instance per CPU. Not globally accessible.
//   The CPU thread is the primary consumer (claimNext, deliverableMask).
//   Device threads may call raise() via the InterruptRouter, which performs
//   atomic OR on the shared masks.
//
// HOT PATH:
//   deliverable = pendingLevelsMask & maskHigherThanIPL[psIpl]
//   One atomic load + one AND + one compare-to-zero per instruction boundary.
//
// INVARIANT (CROSS-THREAD SAFETY):
//   All cross-thread mutations of pending masks use single atomic RMW ops:
//     raise  uses atomic fetch_or
//     clear  uses atomic fetch_and
//     claim  uses atomic fetch_and (edge) or sets inService (level)
//   This prevents lost assertions when device threads race with CPU thread.
//
// BOUNDARY:
//   IRQPendingState has NO knowledge of IPRs, SCBB, PS, or guest memory.
//   It operates purely on source IDs, IPL levels, and bitmasks.
//   SISR is an IPR SSOT maintained by PalService; this struct only reflects
//   the pending levels into its masks.
//
// REFERENCE:
//   Alpha AXP System Reference Manual v6, 1994, II-A, Ch 6.4.
// ============================================================================

#include "IRQ_SourceId_core.h"

#include <array>
#include <atomic>

#ifndef AXP_ALWAYS_INLINE
#define AXP_ALWAYS_INLINE inline
#endif
#ifndef AXP_HOT
#define AXP_HOT
#endif

struct alignas(64) IRQPendingState final
{
    // ========================================================================
    // SHARED STATE (cross-thread visible, atomic)
    // ========================================================================

    // Summary bitset: bit L set => at least one pending source at IPL L.
    // This is the single fast SSOT for "what needs attention."
    std::atomic<quint32> pendingLevelsMask{ 0 };

    // Per-level source bitmask (quint64 for up to 64 sources).
    // Bit S set in pendingSourcesByLevel[L] => source S is pending at IPL L.
    std::array<std::atomic<quint64>, IrqIPL::NUM_LEVELS> pendingSourcesByLevel{};

    // Cached highest pending level for ultra-fast per-instruction check.
    // 0xFF means "nothing pending." Only set by raise/clear/claim.
    // Device threads update this via atomic store after raise.
    std::atomic<quint8> highestPendingLevel{ 0xFF };

    // ========================================================================
    // CPU-THREAD-ONLY STATE (not accessed by device threads)
    // ========================================================================

    // In-service mask: bit S set => source S has been claimed (level-triggered)
    // and is awaiting device deassert before it can be claimed again.
    // CPU-thread-only: set by claimNext, cleared by clearSource.
    quint64 inServiceMask{ 0 };

    // ========================================================================
    // STATIC CONFIGURATION (set once at init, read-only thereafter)
    // ========================================================================

    // Per-source trigger mode (Edge or Level).
    std::array<IrqTriggerMode, IrqSource::MAX_SOURCES> triggerMode{};

    // Per-source SCB vector index (static, set at registration).
    std::array<ScbVectorIndex, IrqSource::MAX_SOURCES> sourceVector{};

    // Per-source IPL assignment (static, set at registration).
    // Redundant with the level used in raise(), but useful for clear()
    // when the caller only knows the sourceId.
    std::array<quint8, IrqSource::MAX_SOURCES> sourceIPL{};

    // ========================================================================
    // PRECOMPUTED MASKS
    // ========================================================================

    // maskHigherThanIPL[ipl] has bits set for all levels strictly above ipl.
    //   maskHigherThanIPL[0]  = bits 1..31
    //   maskHigherThanIPL[31] = 0
    static constexpr std::array<quint32, 32> maskHigherThanIPL = []() constexpr
    {
        std::array<quint32, 32> t{};
        for (quint32 ipl = 0; ipl < 32; ++ipl)
        {
            if (ipl >= 31)
                t[ipl] = 0u;
            else
                t[ipl] = ~((1u << (ipl + 1u)) - 1u);
        }
        return t;
    }();

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    // Register a source with its static properties.
    // Called once per source during system init. Not thread-safe (init only).
    AXP_ALWAYS_INLINE void registerSource(
        IrqSourceId     src,
        quint8          ipl,
        ScbVectorIndex  vector,
        IrqTriggerMode  trigger) noexcept
    {
        if (src >= IrqSource::MAX_SOURCES || ipl >= IrqIPL::NUM_LEVELS) return;
        triggerMode[src]   = trigger;
        sourceVector[src]  = vector;
        sourceIPL[src]     = ipl;
    }

    // Reset all pending state (e.g., on CPU reset or INITPAL).
    AXP_ALWAYS_INLINE void reset() noexcept
    {
        pendingLevelsMask.store(0, std::memory_order_release);
        for (auto& lvl : pendingSourcesByLevel)
            lvl.store(0, std::memory_order_release);
        highestPendingLevel.store(0xFF, std::memory_order_release);
        inServiceMask = 0;
    }

    // ========================================================================
    // RAISE / CLEAR (called by InterruptRouter, possibly from device threads)
    // ========================================================================

    // Assert an interrupt source at a given IPL.
    // Thread-safe: uses atomic OR. May be called from any thread.
    AXP_HOT AXP_ALWAYS_INLINE void raise(IrqSourceId src, quint8 ipl) noexcept
    {
        if (ipl >= IrqIPL::NUM_LEVELS || src >= IrqSource::MAX_SOURCES) return;

        const quint64 srcBit = (1ULL << src);
        pendingSourcesByLevel[ipl].fetch_or(srcBit, std::memory_order_release);
        pendingLevelsMask.fetch_or(1u << ipl, std::memory_order_release);

        // Update cached highest (relaxed: eventual visibility is fine)
        quint8 prev = highestPendingLevel.load(std::memory_order_relaxed);
        while (prev == 0xFF || ipl > prev)
        {
            if (highestPendingLevel.compare_exchange_weak(
                    prev, ipl, std::memory_order_release, std::memory_order_relaxed))
                break;
            // prev reloaded by CAS failure, loop retries
        }
    }

    // Deassert an interrupt source.
    // For level-triggered sources: called when device is serviced (MMIO clear).
    // For edge-triggered sources: called by claimNext automatically.
    // Thread-safe: uses atomic AND. Clears inService if on CPU thread.
    AXP_HOT AXP_ALWAYS_INLINE void clear(IrqSourceId src, quint8 ipl) noexcept
    {
        if (ipl >= IrqIPL::NUM_LEVELS || src >= IrqSource::MAX_SOURCES) return;

        const quint64 srcBit = (1ULL << src);

        // Clear source from per-level mask
        const quint64 remaining =
            pendingSourcesByLevel[ipl].fetch_and(~srcBit, std::memory_order_release)
            & ~srcBit;

        // If no sources remain at this level, clear the level summary bit
        if (remaining == 0)
        {
            pendingLevelsMask.fetch_and(~(1u << ipl), std::memory_order_release);
        }

        // Clear inService (CPU-thread-only; safe because device clear
        // runs on CPU thread via MMIO write execution path).
        inServiceMask &= ~srcBit;

        // Recompute cached highest
        recomputeHighestCached();
    }

    // ========================================================================
    // HOT-PATH QUERY (CPU thread only)
    // ========================================================================

    // Ultra-fast per-instruction check using cached highest.
    // Returns true if any pending interrupt is deliverable at the given PS.IPL.
    AXP_HOT AXP_ALWAYS_INLINE bool hasDeliverable(quint8 psIpl) const noexcept
    {
        const quint8 h = highestPendingLevel.load(std::memory_order_acquire);
        if (h == 0xFF) return false;
        return h > psIpl;
    }

    // Full deliverable mask (for diagnostics or when you need the set of levels).
    AXP_HOT AXP_ALWAYS_INLINE quint32 deliverableMask(quint8 psIpl) const noexcept
    {
        if (psIpl >= 31) return 0;
        return pendingLevelsMask.load(std::memory_order_acquire)
             & maskHigherThanIPL[psIpl];
    }

    // ========================================================================
    // CLAIM (CPU thread only)
    // ========================================================================
    //
    // Selects the highest deliverable interrupt, resolves one source at that
    // level, and either clears it (edge) or marks it in-service (level).
    //
    // Returns ClaimedInterrupt with valid=false if nothing deliverable.
    //
    // DOES NOT modify PS.IPL or IPR.SISR. Those are the caller's responsibility.
    //
    AXP_HOT AXP_ALWAYS_INLINE ClaimedInterrupt claimNext(quint8 psIpl) noexcept
    {
        ClaimedInterrupt out{};

        const quint32 dmask = deliverableMask(psIpl);
        if (dmask == 0) return out;

        const quint8 lvl = highestSetBit(dmask);
        if (lvl >= IrqIPL::NUM_LEVELS) return out;

        // Load sources pending at this level
        const quint64 srcMask =
            pendingSourcesByLevel[lvl].load(std::memory_order_acquire);

        if (srcMask == 0)
        {
            // Inconsistent: summary says pending but no sources.
            // Repair by clearing level bit.
            pendingLevelsMask.fetch_and(~(1u << lvl), std::memory_order_release);
            recomputeHighestCached();
            return out;
        }

        // Exclude already in-service sources (level-triggered guard)
        const quint64 claimable = srcMask & ~inServiceMask;
        if (claimable == 0)
        {
            // All sources at this level are in-service. Nothing deliverable here.
            // Note: we do NOT clear the level bit because sources are still
            // asserted; they're just temporarily blocked by inService.
            // Try next lower level? For simplicity, return nothing this cycle.
            // The next poll after a device clears inService will deliver.
            return out;
        }

        // Pick one source (lowest-numbered for determinism)
        const IrqSourceId src = lowestSetBit64(claimable);
        const quint64 srcBit = (1ULL << src);

        // Determine trigger mode
        const IrqTriggerMode mode = triggerMode[src];

        if (mode == IrqTriggerMode::Edge)
        {
            // Edge-triggered: consume (clear) the source atomically
            const quint64 remaining =
                pendingSourcesByLevel[lvl].fetch_and(~srcBit, std::memory_order_release)
                & ~srcBit;

            if (remaining == 0)
            {
                pendingLevelsMask.fetch_and(~(1u << lvl), std::memory_order_release);
            }

            recomputeHighestCached();
        }
        else
        {
            // Level-triggered: do NOT clear pending. Mark in-service instead.
            // Source remains asserted until device calls clear().
            inServiceMask |= srcBit;
        }

        out.valid   = true;
        out.ipl     = lvl;
        out.source  = src;
        out.vector  = sourceVector[src];
        out.trigger = mode;
        return out;
    }

    // ========================================================================
    // DIAGNOSTICS (CPU thread only)
    // ========================================================================

    // Read the current pending sources at a given IPL level.
    AXP_ALWAYS_INLINE quint64 pendingSourcesAt(quint8 ipl) const noexcept
    {
        if (ipl >= IrqIPL::NUM_LEVELS) return 0;
        return pendingSourcesByLevel[ipl].load(std::memory_order_acquire);
    }

    // Check if a specific source is currently in-service.
    AXP_ALWAYS_INLINE bool isInService(IrqSourceId src) const noexcept
    {
        if (src >= IrqSource::MAX_SOURCES) return false;
        return (inServiceMask & (1ULL << src)) != 0;
    }

    // Check if a specific source is pending at its registered IPL.
    AXP_ALWAYS_INLINE bool isSourcePending(IrqSourceId src) const noexcept
    {
        if (src >= IrqSource::MAX_SOURCES) return false;
        const quint8 ipl = sourceIPL[src];
        if (ipl >= IrqIPL::NUM_LEVELS) return false;
        return (pendingSourcesByLevel[ipl].load(std::memory_order_acquire)
                & (1ULL << src)) != 0;
    }

private:

    // ========================================================================
    // BIT-SCAN HELPERS
    // ========================================================================

    static AXP_HOT AXP_ALWAYS_INLINE quint8 lowestSetBit64(quint64 v) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx = 0;
        _BitScanForward64(&idx, v);
        return static_cast<quint8>(idx);
#else
        return static_cast<quint8>(__builtin_ctzll(v));
#endif
    }

    static AXP_HOT AXP_ALWAYS_INLINE quint8 highestSetBit(quint32 v) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx = 0;
        _BitScanReverse(&idx, v);
        return static_cast<quint8>(idx);
#else
        return static_cast<quint8>(31u - __builtin_clz(v));
#endif
    }

    // Recompute cached highestPendingLevel from the summary mask.
    // Called after clear or claim when the highest may have changed.
    AXP_HOT AXP_ALWAYS_INLINE void recomputeHighestCached() noexcept
    {
        const quint32 mask = pendingLevelsMask.load(std::memory_order_acquire);
        if (mask == 0)
            highestPendingLevel.store(0xFF, std::memory_order_release);
        else
            highestPendingLevel.store(highestSetBit(mask), std::memory_order_release);
    }
};

#endif // IRQ_PENDINGSTATE_H
