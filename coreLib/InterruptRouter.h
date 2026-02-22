// ============================================================================
// InterruptRouter.h - ============================================================================
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

#ifndef INTERRUPT_ROUTER_H
#define INTERRUPT_ROUTER_H
// ============================================================================
// InterruptRouter.h  (header-only, ASCII-128 clean)
// ============================================================================
// System-level interrupt routing: thin wiring table between devices and
// per-CPU IRQPendingState instances.
//
// OWNERSHIP:
//   Owned by Platform / SystemBoard. One instance per system.
//   NOT globally accessible. Injected into devices at construction.
//   Holds raw pointers to each CPU's IRQPendingState (registered at init).
//
// RESPONSIBILITIES:
//   - Source registration: sourceId -> (ipl, vector, trigger, affinity)
//   - Route raise/clear to the correct CPU's IRQPendingState
//   - SMP routing policy (affinity, round-robin, broadcast for IPI)
//   - Device source ID allocation from the MMIO device range
//   - Instrumentation counters (optional)
//
// NON-RESPONSIBILITIES:
//   - No arbitration (that's IRQPendingState)
//   - No IPR knowledge (that's PalService)
//   - No SCB reads (that's PAL delivery)
//   - No IPL comparison (that's the CPU hot path)
//
// THREAD SAFETY:
//   Registration (registerCPU, registerSource) is init-only, not thread-safe.
//   raise() and clear() may be called from device threads; they delegate to
//   atomic operations on IRQPendingState.
//
// REFERENCE:
//   Alpha AXP System Reference Manual v6, 1994.
//   OpenVMS device interrupt conventions: IPL 20-23, SCB vectoring.
// ============================================================================

#include "IRQ_SourceId_core.h"
#include "IRQPendingState.h"

#include <array>
#include <QtGlobal>

#ifndef AXP_ALWAYS_INLINE
#define AXP_ALWAYS_INLINE inline
#endif
#ifndef AXP_HOT
#define AXP_HOT
#endif

// ============================================================================
// Routing policy (per-source, static configuration)
// ============================================================================
enum class IrqRoutingPolicy : quint8
{
    FixedCPU,       // Always deliver to affinityCpu
    RoundRobin,     // Rotate across online CPUs (future)
    Broadcast       // Deliver to all CPUs (IPI only)
};

// ============================================================================
// Per-source routing entry (internal to router)
// ============================================================================
struct IrqRouteEntry final
{
    bool            registered { false };
    IrqSourceId     sourceId   { 0 };
    quint8          ipl        { 0 };
    ScbVectorIndex  vector     { 0 };
    IrqTriggerMode  trigger    { IrqTriggerMode::Edge };
    IrqRoutingPolicy policy    { IrqRoutingPolicy::FixedCPU };
    qint32          affinityCpu{ 0 };

    // Round-robin state
    qint32          lastCpu    { -1 };

    // Instrumentation
    quint64         raiseCount { 0 };
    quint64         clearCount { 0 };
};

// ============================================================================
// InterruptRouter
// ============================================================================

class InterruptRouter final
{
public:
    static constexpr int MAX_CPUS = 32;

    InterruptRouter() noexcept = default;

    // Non-copyable, non-movable (system singleton)
    InterruptRouter(const InterruptRouter&)            = delete;
    InterruptRouter& operator=(const InterruptRouter&) = delete;
    InterruptRouter(InterruptRouter&&)                 = delete;
    InterruptRouter& operator=(InterruptRouter&&)      = delete;

    // ========================================================================
    // INITIALIZATION (called during system bring-up, NOT thread-safe)
    // ========================================================================

    // Register a CPU's IRQPendingState with the router.
    // Called once per CPU during platform init.
    // cpuId is the array index; state must outlive the router.
    AXP_ALWAYS_INLINE bool registerCPU(
        int cpuId,
        IRQPendingState* state) noexcept
    {
        if (cpuId < 0 || cpuId >= MAX_CPUS || !state) return false;
        m_cpuPending[cpuId] = state;
        if (cpuId >= m_cpuCount) m_cpuCount = cpuId + 1;
        return true;
    }

    // Register an interrupt source with full routing configuration.
    // Propagates static config (trigger, vector, ipl) to all target CPUs'
    // IRQPendingState instances.
    AXP_ALWAYS_INLINE bool registerSource(
        IrqSourceId      sourceId,
        quint8           ipl,
        ScbVectorIndex   vector,
        IrqTriggerMode   trigger,
        IrqRoutingPolicy policy    = IrqRoutingPolicy::FixedCPU,
        qint32           affinityCpu = 0) noexcept
    {
        if (!IrqSource::isValid(sourceId) || ipl >= IrqIPL::NUM_LEVELS)
            return false;

        auto& entry         = m_routes[sourceId];
        entry.registered    = true;
        entry.sourceId      = sourceId;
        entry.ipl           = ipl;
        entry.vector        = vector;
        entry.trigger       = trigger;
        entry.policy        = policy;
        entry.affinityCpu   = affinityCpu;
        entry.lastCpu       = -1;
        entry.raiseCount    = 0;
        entry.clearCount    = 0;

        // Propagate static config to CPU pending states
        for (int cpu = 0; cpu < m_cpuCount; ++cpu)
        {
            if (m_cpuPending[cpu])
            {
                m_cpuPending[cpu]->registerSource(
                    sourceId, ipl, vector, trigger);
            }
        }

        return true;
    }

    // Allocate the next available MMIO device source ID.
    // Returns 0xFF if range exhausted.
    AXP_ALWAYS_INLINE IrqSourceId allocateDeviceSourceId() noexcept
    {
        return IrqSource::allocateDevice(m_nextDeviceId);
    }

    // ========================================================================
    // CONVENIENCE: Register + allocate in one call for MMIO devices
    // ========================================================================

    // Registers an MMIO device interrupt source.
    // Allocates a source ID, registers the route, returns the assigned ID.
    // Returns 0xFF on failure.
    AXP_ALWAYS_INLINE IrqSourceId registerDevice(
        quint8           ipl,
        ScbVectorIndex   vector,
        IrqTriggerMode   trigger    = IrqTriggerMode::Level,
        IrqRoutingPolicy policy     = IrqRoutingPolicy::FixedCPU,
        qint32           affinityCpu = 0) noexcept
    {
        IrqSourceId id = allocateDeviceSourceId();
        if (id == 0xFF) return 0xFF;

        if (!registerSource(id, ipl, vector, trigger, policy, affinityCpu))
            return 0xFF;

        return id;
    }

    // ========================================================================
    // PRE-REGISTER PLATFORM SOURCES (called during system init)
    // ========================================================================

    // Register all fixed platform sources (SW, AST, clock, IPI, etc.)
    // with their standard IPL assignments and trigger modes.
    AXP_ALWAYS_INLINE void registerPlatformSources() noexcept
    {
        // Software interrupt sources (SISR levels 1..15)
        // Each source ID = level number, edge-triggered
        for (quint8 lvl = 1; lvl <= IrqSource::SW_MAX; ++lvl)
        {
            registerSource(
                lvl,                        // sourceId = level (1:1 mapping)
                lvl,                        // ipl = same level
                static_cast<ScbVectorIndex>(0x1000 + lvl),  // SW vector convention
                IrqTriggerMode::Edge,
                IrqRoutingPolicy::FixedCPU,
                0                           // affinityCpu (SW is always local)
            );
        }

        // AST source
        registerSource(
            IrqSource::AST,
            IrqIPL::AST,
            0x0040,                          // SCB offset for AST (platform-specific)
            IrqTriggerMode::Edge,            // one-shot per AST delivery
            IrqRoutingPolicy::FixedCPU,
            0
        );

        // Clock / timer
        registerSource(
            IrqSource::CLOCK,
            IrqIPL::CLOCK,
            0x0060,                          // SCB offset for clock (platform-specific)
            IrqTriggerMode::Edge,            // each tick is a discrete event
            IrqRoutingPolicy::FixedCPU,
            0
        );

        // IPI
        registerSource(
            IrqSource::IPI,
            IrqIPL::IPI,
            0x00C0,                          // SCB offset for IPI (platform-specific)
            IrqTriggerMode::Edge,            // coalescing signal
            IrqRoutingPolicy::FixedCPU,      // overridden per-call by raiseIPI
            0
        );

        // Performance counter overflow
        registerSource(
            IrqSource::PERF_COUNTER,
            IrqIPL::PERF,
            0x0640,                          // SCB offset (platform-specific)
            IrqTriggerMode::Edge,
            IrqRoutingPolicy::FixedCPU,
            0
        );

        // Power fail
        registerSource(
            IrqSource::POWER_FAIL,
            IrqIPL::POWER,
            0x0020,                          // SCB offset (platform-specific)
            IrqTriggerMode::Edge,
            IrqRoutingPolicy::FixedCPU,
            0
        );

        // Machine check
        registerSource(
            IrqSource::MACHINE_CHECK,
            IrqIPL::MCHK,
            0x0010,                          // SCB offset (platform-specific)
            IrqTriggerMode::Edge,
            IrqRoutingPolicy::FixedCPU,
            0
        );
    }

    // ========================================================================
    // RAISE / CLEAR (called by devices, possibly from non-CPU threads)
    // ========================================================================

    // Assert an interrupt source. Routes to the appropriate CPU's pending state.
    // Thread-safe: delegates to atomic operations on IRQPendingState.
    AXP_HOT AXP_ALWAYS_INLINE void raise(IrqSourceId sourceId) noexcept
    {
        if (sourceId >= IrqSource::MAX_SOURCES) return;

        auto& entry = m_routes[sourceId];
        if (!entry.registered) return;

        ++entry.raiseCount;

        const int target = resolveTargetCPU(entry);
        if (target < 0 || target >= m_cpuCount) return;
        if (!m_cpuPending[target]) return;

        m_cpuPending[target]->raise(entry.sourceId, entry.ipl);
    }

    // Deassert an interrupt source. Routes to the appropriate CPU's pending state.
    // For level-triggered: called when guest services the device (MMIO write-to-clear).
    // Thread-safe: delegates to atomic operations on IRQPendingState.
    AXP_HOT AXP_ALWAYS_INLINE void clear(IrqSourceId sourceId) noexcept
    {
        if (sourceId >= IrqSource::MAX_SOURCES) return;

        auto& entry = m_routes[sourceId];
        if (!entry.registered) return;

        ++entry.clearCount;

        const int target = resolveTargetCPU(entry);
        if (target < 0 || target >= m_cpuCount) return;
        if (!m_cpuPending[target]) return;

        m_cpuPending[target]->clear(entry.sourceId, entry.ipl);
    }

    // ========================================================================
    // IPI (inter-processor interrupt)
    // ========================================================================

    // Send an IPI to a specific CPU.
    AXP_HOT AXP_ALWAYS_INLINE void raiseIPI(int targetCpu) noexcept
    {
        if (targetCpu < 0 || targetCpu >= m_cpuCount) return;
        if (!m_cpuPending[targetCpu]) return;

        auto& entry = m_routes[IrqSource::IPI];
        ++entry.raiseCount;

        m_cpuPending[targetCpu]->raise(IrqSource::IPI, IrqIPL::IPI);
    }

    // Broadcast IPI to all CPUs except the sender.
    AXP_HOT AXP_ALWAYS_INLINE void broadcastIPI(int senderCpu) noexcept
    {
        auto& entry = m_routes[IrqSource::IPI];

        for (int cpu = 0; cpu < m_cpuCount; ++cpu)
        {
            if (cpu == senderCpu) continue;
            if (!m_cpuPending[cpu]) continue;

            ++entry.raiseCount;
            m_cpuPending[cpu]->raise(IrqSource::IPI, IrqIPL::IPI);
        }
    }

    // Broadcast IPI to CPUs selected by a bitmask.
    AXP_HOT AXP_ALWAYS_INLINE void broadcastIPIMask(quint64 cpuMask) noexcept
    {
        auto& entry = m_routes[IrqSource::IPI];

        for (int cpu = 0; cpu < m_cpuCount; ++cpu)
        {
            if (!(cpuMask & (1ULL << cpu))) continue;
            if (!m_cpuPending[cpu]) continue;

            ++entry.raiseCount;
            m_cpuPending[cpu]->raise(IrqSource::IPI, IrqIPL::IPI);
        }
    }

    // ========================================================================
    // SOFTWARE INTERRUPTS (convenience, called by PalService on CPU thread)
    // ========================================================================

    // Raise a software interrupt at the given level on the specified CPU.
    // Used by MTPR_SIRR handler after updating IPR.SISR.
    AXP_HOT AXP_ALWAYS_INLINE void raiseSoftwareInterrupt(
        int cpuId, quint8 level) noexcept
    {
        if (cpuId < 0 || cpuId >= m_cpuCount) return;
        if (level < 1 || level > IrqSource::SW_MAX) return;
        if (!m_cpuPending[cpuId]) return;

        // Source ID = level (1:1 mapping)
        m_cpuPending[cpuId]->raise(static_cast<IrqSourceId>(level), level);
    }

    // Clear a software interrupt at the given level on the specified CPU.
    // Used by PAL delivery code after initiating the software interrupt.
    AXP_HOT AXP_ALWAYS_INLINE void clearSoftwareInterrupt(
        int cpuId, quint8 level) noexcept
    {
        if (cpuId < 0 || cpuId >= m_cpuCount) return;
        if (level < 1 || level > IrqSource::SW_MAX) return;
        if (!m_cpuPending[cpuId]) return;

        m_cpuPending[cpuId]->clear(static_cast<IrqSourceId>(level), level);
    }

    // ========================================================================
    // AST (convenience, called by CPU-thread AST evaluation)
    // ========================================================================

    // Raise AST pending on specified CPU. Called when AST gating
    // conditions are met (ASTSR pending AND ASTEN enabled AND mode permits).
    AXP_HOT AXP_ALWAYS_INLINE void raiseAST(int cpuId) noexcept
    {
        if (cpuId < 0 || cpuId >= m_cpuCount) return;
        if (!m_cpuPending[cpuId]) return;

        m_cpuPending[cpuId]->raise(IrqSource::AST, IrqIPL::AST);
    }

    // Clear AST pending on specified CPU. Called when gating conditions
    // are no longer met or when AST is delivered and consumed.
    AXP_HOT AXP_ALWAYS_INLINE void clearAST(int cpuId) noexcept
    {
        if (cpuId < 0 || cpuId >= m_cpuCount) return;
        if (!m_cpuPending[cpuId]) return;

        m_cpuPending[cpuId]->clear(IrqSource::AST, IrqIPL::AST);
    }

    // ========================================================================
    // QUERY (diagnostics, not hot-path)
    // ========================================================================

    // Read instrumentation counters for a source.
    AXP_ALWAYS_INLINE bool sourceStats(
        IrqSourceId sourceId,
        quint64& outRaiseCount,
        quint64& outClearCount) const noexcept
    {
        if (sourceId >= IrqSource::MAX_SOURCES) return false;
        const auto& entry = m_routes[sourceId];
        if (!entry.registered) return false;
        outRaiseCount = entry.raiseCount;
        outClearCount = entry.clearCount;
        return true;
    }

    // Check if a source is registered.
    AXP_ALWAYS_INLINE bool isRegistered(IrqSourceId sourceId) const noexcept
    {
        if (sourceId >= IrqSource::MAX_SOURCES) return false;
        return m_routes[sourceId].registered;
    }

    // Get the number of registered CPUs.
    AXP_ALWAYS_INLINE int cpuCount() const noexcept { return m_cpuCount; }

    // Direct access to a CPU's pending state (for diagnostics only).
    AXP_ALWAYS_INLINE IRQPendingState* cpuPendingState(int cpuId) const noexcept
    {
        if (cpuId < 0 || cpuId >= m_cpuCount) return nullptr;
        return m_cpuPending[cpuId];
    }

private:

    // ========================================================================
    // ROUTING RESOLUTION
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE int resolveTargetCPU(IrqRouteEntry& entry) const noexcept
    {
        switch (entry.policy)
        {
        case IrqRoutingPolicy::FixedCPU:
            return entry.affinityCpu;

        case IrqRoutingPolicy::RoundRobin:
        {
            int next = entry.lastCpu + 1;
            if (next >= m_cpuCount) next = 0;
            entry.lastCpu = next;
            return next;
        }

        case IrqRoutingPolicy::Broadcast:
            // Broadcast handled specially by caller (raiseIPI, broadcastIPI).
            // For single-raise path, fall back to CPU 0.
            return 0;

        default:
            return 0;
        }
    }

    // ========================================================================
    // STATE
    // ========================================================================

    // Per-CPU pending state pointers (non-owning, set during init)
    std::array<IRQPendingState*, MAX_CPUS> m_cpuPending{};

    // Per-source routing table
    std::array<IrqRouteEntry, IrqSource::MAX_SOURCES> m_routes{};

    // Number of registered CPUs
    int m_cpuCount{ 0 };

    // Next allocatable device source ID
    IrqSourceId m_nextDeviceId{ IrqSource::DEVICE_BASE };
};

#endif // INTERRUPT_ROUTER_H
