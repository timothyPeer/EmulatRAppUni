// ============================================================================
// global_Ev6TLB_Singleton.h - Get per-CPU SPAM manager
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

#pragma once

// ============================================================================
// Global PTE/TLB Interface - SMP Coherent
// ============================================================================
// 
// Design Principles:
// 1. Per-CPU SPAM (Software TLB) - Each CPU has its own SPAM cache
// 2. Shared ASN Epoch Table - All CPUs coordinate on ASN generation
// 3. SMP Coherence - TLB shootdown across CPUs
// 4. Naming Convention: global_xxx(cpuId) for per-CPU access
// 
// Architecture:
//   CPU 0           CPU 1           CPU 2           CPU 3

//  +-----+         +-----+         +-----+         +-----+
//  | SPAM|         | SPAM|         | SPAM|         | SPAM|
//  | DTB |         | DTB |         | DTB |         | DTB |
//  | ITB |         | ITB |         | ITB |         | ITB |
//  +-----+         +-----+         +-----+         +-----+        
//     |              |               |               |
//  -------------------------------------------------------
//            \/           \/
//       +---------------------+ 
//       |  Shared ASN Epochs  |
//       |  (256 atomic ints)  |
//       +---------------------+
// 
// ============================================================================

#include <QtCore/QtGlobal>
#include <QtCore/QAtomicInteger>
#include <array>

#include "Ev6SiliconTypes.h"
#include "alpha_pte_core.h"
#include "../coreLib/types_core.h"
#include "pteLib/alpha_spam_manager.h"

// Forward declarations


template<typename Traits> struct SPAMTag;

// ============================================================================
// Per-CPU SPAM Manager Array
// ============================================================================

class GlobalPTESMP
{
public:
    static constexpr int MAX_CPUS = 64;  // Adjust as needed

private:
    // Per-CPU SPAM managers
    std::array<Ev6SPAMShardManager*, MAX_CPUS> m_spam;

    // Shared ASN epoch table (all CPUs coordinate on this)
    AsnGenTable m_asnEpochs;

    // Sweep coordination table
    SweepTable m_sweepFlags;

    // Number of active CPUs
    qsizetype m_cpuCount;

    // Singleton instance
    static GlobalPTESMP& instance() noexcept {
        static GlobalPTESMP inst;
        return inst;
    }

    GlobalPTESMP()
        : m_spam{}
        , m_cpuCount(1)
    {
        // Initialize all pointers to nullptr
        for (int i = 0; i < MAX_CPUS; ++i) {
            m_spam[i] = nullptr;
        }
    }

public:
    // ========================================================================
    // Initialization (called at system startup)
    // ========================================================================

    static void initialize(qsizetype cpuCount) noexcept {
        auto& inst = instance();
        inst.m_cpuCount = cpuCount;

        // Allocate per-CPU SPAM managers
        for (int i = 0; i < cpuCount; ++i) {
            inst.m_spam[i] = new Ev6SPAMShardManager(cpuCount);
            // NO attachAsnEpochs() - manager has its own epochs!
        }
    }

    static void shutdown() noexcept {
        auto& inst = instance();
        for (int i = 0; i < inst.m_cpuCount; ++i) {
            delete inst.m_spam[i];
            inst.m_spam[i] = nullptr;
        }
    }

    // ========================================================================
    // Per-CPU SPAM Access (PRIMARY INTERFACE)
    // ========================================================================

    /**
     * @brief Get per-CPU SPAM manager
     *
     * This is the PRIMARY accessor following your global_xxx(cpuId) pattern.
     * Each CPU has its own SPAM cache for fast lookups without contention.
     *
     * @param cpuId CPU identifier [0..cpuCount-1]
     * @return Reference to CPU's SPAM manager
     */
    static Ev6SPAMShardManager& getSPAM(CPUIdType cpuId) noexcept {
        auto& inst = instance();
        Q_ASSERT(cpuId < inst.m_cpuCount);
        Q_ASSERT(inst.m_spam[cpuId] != nullptr);
        return *inst.m_spam[cpuId];
    }

    // ========================================================================
    // Shared ASN Epoch Table Access
    // ========================================================================

    /**
     * @brief Get current ASN generation (shared across all CPUs)
     *
     * @param asn Address Space Number [0..255]
     * @return Current generation number for this ASN
     */
    static quint32 getASNGeneration(ASNType asn) noexcept {
        return instance().m_asnEpochs.gen[asn].loadRelaxed();
    }

    /**
     * @brief Increment ASN generation (triggers TLB flush across all CPUs)
     *
     * This is called when an ASN wraps or needs invalidation.
     * All CPUs will see stale entries for this ASN and refill.
     *
     * @param asn Address Space Number to increment
     * @return New generation number
     */
    static quint32 incrementASNGeneration(ASNType asn) noexcept {
        return instance().m_asnEpochs.gen[asn].fetchAndAddRelaxed(1) + 1;
    }

    /**
     * @brief Get shared ASN epoch table (for SPAM bucket attachment)
     *
     * @return Pointer to shared ASN epoch table
     */
    static const AsnGenTable* getASNEpochTable() noexcept {
        return &instance().m_asnEpochs;
    }

    // ========================================================================
    // Lazy Invalidation via ASN Epochs (PREFERRED - Fast!)
    // ========================================================================

    /**
     * @brief Lazily invalidate all TLB entries for ASN across ALL CPUs
     *
     * This is the PRIMARY invalidation mechanism. It:
     * 1. Increments the ASN generation atomically (~5 cycles)
     * 2. All CPUs discover stale entries naturally on next lookup
     * 3. NO inter-processor interrupts (IPIs) needed!
     *
     * Use this for:
     * - ASN changes (process context switch)
     * - Process termination
     * - Most page table modifications
     *
     * Cost: ~5 cycles (one atomic increment)
     *
     * @param asn Address Space Number to invalidate
     * @return New generation number
     */
    static quint32 lazyInvalidateASN(ASNType asn) noexcept {
        // Just increment generation - all CPUs will see stale entries
        return incrementASNGeneration(asn);
    }

    /**
     * @brief Lazily invalidate ALL ASNs across ALL CPUs
     *
     * Use this for:
     * - Page table base (PTBR) changes
     * - Major VM reconfiguration
     *
     * Cost: ~1280 cycles (256 ASNs - 5 cycles each)
     * Still much cheaper than eager shootdown!
     */
    static void lazyInvalidateAll() noexcept {
        // Increment all ASN generations
        for (int asn = 0; asn < 256; ++asn) {
            incrementASNGeneration(asn);
        }
    }
};

// ============================================================================
// Convenience Global Accessors (Following Your Naming Convention)
// ============================================================================

/**
 * @brief Get per-CPU SPAM manager (PRIMARY ACCESSOR)
 *
 * Usage: globalSPAM(cpuId).find(tag, asn);
 *
 * This follows your established pattern:
 * - globalIPRHot(cpuId)
 * - globalIPRCold(cpuId)
 * - globalSPAM(cpuId)   NEW!
 */
inline Ev6SPAMShardManager& globalSPAM(CPUIdType cpuId) noexcept {
    return GlobalPTESMP::getSPAM(cpuId);
}

/**
 * @brief Get shared ASN epoch table
 *
 * Usage: const AsnGenTable* epochs = globalASNEpochs();
 */
inline const AsnGenTable* globalASNEpochs() noexcept {
    return GlobalPTESMP::getASNEpochTable();
}

// ============================================================================
// High-Level PTE Operations (Recommended Interface)
// ============================================================================


// ============================================================================
// Initialization Hook (Call at System Startup)
// ============================================================================

inline void initializeGlobalPTE(qsizetype cpuCount) noexcept {
    GlobalPTESMP::initialize(cpuCount);
}

inline void shutdownGlobalPTE() noexcept {
    GlobalPTESMP::shutdown();
}
