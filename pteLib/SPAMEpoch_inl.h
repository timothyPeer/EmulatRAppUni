// ============================================================================
// SPAMEpoch_inl.h - Epoch-Based Lazy Invalidation for SPAM TLB
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
//
// Generation-counter infrastructure for lazy TLB invalidation.
//
// Two independent epoch axes:
//   1. globalEpoch       -- bumped on context switch / non-ASM flush.
//                           Kills all ASM=0 entries in O(1).
//   2. itb/dtbEpoch[ASN] -- bumped per-ASN on TBIAP / TBISI / TBISD.
//                           Kills entries for one ASN in O(1).
//
// An entry is live iff BOTH axes match.  Global (ASM=1) entries skip
// both checks and survive unconditionally.
//
// Memory ordering:
//   Bumps  -- memory_order_release  (publishes invalidation)
//   Reads  -- memory_order_relaxed  (bucket seqlock provides acquire)
//
// Audit fixes applied:
//   [E1] Bumps now use release (was relaxed).
//   [E2] Added bumpGlobal() -- manager should delegate here.
//   [E3] Added alignas(64) to PerCPUEpochTable.
//   [E4] Added Q_ASSERT bounds checks on ASN.
//   [E6] Added reset() for hard-reset / power-on.
//
// ============================================================================

#ifndef SPAMEPOCH_INL_H
#define SPAMEPOCH_INL_H

#include <QtCore/QtGlobal>
#include <atomic>

#include "alpha_pte_core.h"         // AlphaPTE, PFNType
#include "alpha_spam_types.h"       // Realm, ASNType, SC_Type
#include "coreLib/types_core.h"     // CPUIdType

// ============================================================================
// PerCPUEpochTable -- one instance per emulated CPU
// ============================================================================
//
// Layout: globalEpoch first (hot), then per-ASN arrays.
// alignas(64) prevents false-sharing between adjacent CPU tables.
// ============================================================================

struct alignas(64) PerCPUEpochTable
{
    static constexpr unsigned MAX_ASN = 256;

    // Global epoch -- bumped on non-ASM flush / context switch.
    // Placed at offset 0 of the cache-aligned struct.
    std::atomic<quint32> globalEpoch{ 0 };

    // Per-ASN, per-realm epochs.
    std::atomic<quint32> itbEpoch[MAX_ASN];     // I-stream generations
    std::atomic<quint32> dtbEpoch[MAX_ASN];     // D-stream generations

    PerCPUEpochTable() noexcept
    {
        for (unsigned i = 0; i < MAX_ASN; ++i) {
            itbEpoch[i].store(0, std::memory_order_relaxed);
            dtbEpoch[i].store(0, std::memory_order_relaxed);
        }
    }

    // Hard-reset all epochs to 0 (power-on / machine check).
    void reset() noexcept
    {
        globalEpoch.store(0, std::memory_order_relaxed);
        for (unsigned i = 0; i < MAX_ASN; ++i) {
            itbEpoch[i].store(0, std::memory_order_relaxed);
            dtbEpoch[i].store(0, std::memory_order_relaxed);
        }
    }
};

// ============================================================================
// SPAMEpoch namespace -- static helpers for reading / bumping epochs
// ============================================================================

namespace SPAMEpoch {

    // -- Reads (hot path) --------------------------------------------------------

    // Return the live per-ASN generation for a given realm.
    inline quint32 getCurrent(const PerCPUEpochTable& table,
        Realm realm,
        ASNType asn) noexcept
    {
        Q_ASSERT(asn < PerCPUEpochTable::MAX_ASN);
        if (realm == Realm::I)
            return table.itbEpoch[asn].load(std::memory_order_relaxed);
        else
            return table.dtbEpoch[asn].load(std::memory_order_relaxed);
    }

    // Return the live global epoch.
    inline quint32 getGlobal(const PerCPUEpochTable& table) noexcept
    {
        return table.globalEpoch.load(std::memory_order_relaxed);
    }

    // -- Per-ASN bumps -----------------------------------------------------------

    // Bump I-stream epoch for one ASN (TBISI).  Returns NEW generation.
    inline quint32 bumpITB(PerCPUEpochTable& table, ASNType asn) noexcept
    {
        Q_ASSERT(asn < PerCPUEpochTable::MAX_ASN);
        return table.itbEpoch[asn].fetch_add(1, std::memory_order_release) + 1;
    }

    // Bump D-stream epoch for one ASN (TBISD).  Returns NEW generation.
    inline quint32 bumpDTB(PerCPUEpochTable& table, ASNType asn) noexcept
    {
        Q_ASSERT(asn < PerCPUEpochTable::MAX_ASN);
        return table.dtbEpoch[asn].fetch_add(1, std::memory_order_release) + 1;
    }

    // Bump a single realm's epoch for one ASN.  Convenience dispatcher.
    inline quint32 bump(PerCPUEpochTable& table,
        Realm realm,
        ASNType asn) noexcept
    {
        return (realm == Realm::I) ? bumpITB(table, asn)
            : bumpDTB(table, asn);
    }

    // Bump both ITB and DTB for one ASN (TBIAP / TBIS).
    inline void bumpBoth(PerCPUEpochTable& table, ASNType asn) noexcept
    {
        Q_ASSERT(asn < PerCPUEpochTable::MAX_ASN);
        table.itbEpoch[asn].fetch_add(1, std::memory_order_release);
        table.dtbEpoch[asn].fetch_add(1, std::memory_order_release);
    }

    // Alias for bumpBoth.
    inline void bumpASN(PerCPUEpochTable& table, ASNType asn) noexcept
    {
        bumpBoth(table, asn);
    }

    // -- Global epoch bump -------------------------------------------------------

    // O(1) context-switch / non-ASM invalidation.
    // After this call every ASM=0 entry with stale globalGenAtFill is dead.
    inline void bumpGlobal(PerCPUEpochTable& table) noexcept
    {
        table.globalEpoch.fetch_add(1, std::memory_order_release);
    }

    // -- Bulk bump (TBIA) --------------------------------------------------------

    // Bump every per-ASN epoch (both realms).  O(512 atomic ops).
    // Pair with bumpGlobal() for full TBIA coverage.
    inline void bumpAll(PerCPUEpochTable& table) noexcept
    {
        for (unsigned asn = 0; asn < PerCPUEpochTable::MAX_ASN; ++asn) {
            table.itbEpoch[asn].fetch_add(1, std::memory_order_release);
            table.dtbEpoch[asn].fetch_add(1, std::memory_order_release);
        }
    }

    // -- Predicates (sweep / debug) ----------------------------------------------

    // Is the entry still live w.r.t. the per-ASN epoch?
    inline bool isAsnAlive(const PerCPUEpochTable& table,
        Realm realm, ASNType asn,
        quint32 genAtFill) noexcept
    {
        return genAtFill == getCurrent(table, realm, asn);
    }

    // Is the entry still live w.r.t. the global epoch?
    // Caller must check flags.global first.
    inline bool isGlobalAlive(const PerCPUEpochTable& table,
        quint32 globalGenAtFill) noexcept
    {
        return globalGenAtFill == table.globalEpoch.load(std::memory_order_relaxed);
    }

    // Combined liveness check (both axes).
    inline bool isAlive(const PerCPUEpochTable& table,
        bool isGlobal, quint32 globalGen,
        quint32 asnGen, Realm realm,
        ASNType asn) noexcept
    {
        if (!isGlobal && !isGlobalAlive(table, globalGen))
            return false;
        if (isGlobal)
            return true;
        return isAsnAlive(table, realm, asn, asnGen);
    }

    // Simple scalar staleness test.
    inline bool isStale(quint32 entryGen, quint32 currentGen) noexcept
    {
        return entryGen != currentGen;
    }

} // namespace SPAMEpoch

#endif // SPAMEPOCH_INL_H