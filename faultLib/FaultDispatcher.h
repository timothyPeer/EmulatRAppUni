// ============================================================================
// FaultDispatcher.h - Per-CPU Fault/Exception Dispatcher (Header-Only)
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

#ifndef FAULTDISPATCHER_H
#define FAULTDISPATCHER_H
// ============================================================================
// FaultDispatcher.h - Per-CPU Fault/Exception Dispatcher (Header-Only)
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Purpose:
//   Per-CPU fault, exception, and interrupt event tracker.
//   Checked EVERY instruction cycle - must be extremely fast.
//
// Design:
//   - Header-only for maximum performance
//   - NO atomics (per-CPU, no contention)
//   - NO default constructor (explicit cpuId required)
//   - NO TLS (not thread-safe)
//   - Simple flags for fast eventPending() check
//
// Performance Critical:
//   eventPending() called EVERY instruction (~1 cycle overhead)
// ============================================================================



#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/LoggingMacros.h"
#include "coreLib/global_RegisterMaster_hot.h"
#include "coreLib/global_cBoxState.h"
#include "exceptionLib/ExceptionClass_EV6.h"
#include "exceptionLib/PendingEventKind.h"
#include "faultLib/PendingEvent_Refined.h"


// ============================================================================
// FaultDispatcher - Per-CPU Exception Tracking (Header-Only, No Atomics)
// ============================================================================

class FaultDispatcher final {
public:
    // ====================================================================
    // Construction (NO default constructor - require explicit cpuId)
    // ====================================================================

    explicit FaultDispatcher(CPUIdType cpuId) noexcept
        : m_cpuId(cpuId)
        , m_pending{}
        , m_pendingFlags(0)
        , m_iprGlobalMaster(getCPUStateView(cpuId))
    {
        DEBUG_LOG(QString("FaultDispatcher[%1]: Initialized").arg(cpuId));
    }

    ~FaultDispatcher() = default;

    // Disable copy/move
    FaultDispatcher(const FaultDispatcher&) = delete;
    FaultDispatcher& operator=(const FaultDispatcher&) = delete;
    FaultDispatcher(FaultDispatcher&&) = delete;
    FaultDispatcher& operator=(FaultDispatcher&&) = delete;

    // ====================================================================
    // Event Query Interface (HOT PATH - Called Every Cycle)
    // ====================================================================

    /**
     * @brief Check if any event is pending (HOTTEST PATH)
     *
     * Called by AlphaCPU::executeOneInstruction() every cycle.
     * Performance: ~1 cycle (simple load + compare, no atomics).
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool eventPending() const noexcept {
        return m_pendingFlags != 0;  // No atomic - per-CPU only
    }

    /**
     * @brief Check if trap/fault is pending (alias)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasPendingFault() const noexcept {
        return eventPending();
    }

    /**
     * @brief Check if trap is pending (alias)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasPendingTrap() const noexcept {
        return eventPending();
    }

    /**
     * @brief Check if arithmetic trap is pending (for TRAPB)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasPendingArithmeticTraps() const noexcept {
        return (m_pendingFlags & FLAG_ARITHMETIC_TRAP) != 0;
    }

    /**
     * @brief Check if memory fault is pending
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasMemoryFault() const noexcept {
        return (m_pendingFlags & (FLAG_DTB_MISS | FLAG_ITB_MISS)) != 0;
    }

    /**
     * @brief Check if interrupt is pending
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasInterrupt() const noexcept {
        return (m_pendingFlags & FLAG_INTERRUPT) != 0;
    }

    /**
     * @brief Check if critical event (machine check) is pending
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        bool hasCriticalEvent() const noexcept {
        return (m_pendingFlags & FLAG_MACHINE_CHECK) != 0;
    }

    // ====================================================================
    // Event Information Access
    // ====================================================================

    /**
     * @brief Get pending event details (const reference)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        const PendingEvent& getPendingEvents() const noexcept {
        return m_pending;
    }

    /**
     * @brief Get pending event state (alias for compatibility)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        const PendingEvent& eventPendingState() const noexcept {
        return m_pending;
    }

    /**
     * @brief Get pending trap class
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        ExceptionClass_EV6 getPendingTrapClass() const noexcept {
        return m_pending.exceptionClass;
    }

    /**
     * @brief Get pending trap faulting VA
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        uint64_t getPendingTrapVA() const noexcept {
        return m_pending.faultVA;
    }

    /**
     * @brief Get pending trap faulting PC
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        uint64_t getPendingTrapPC() const noexcept {
        return m_pending.faultPC;
    }

    // ====================================================================
    // Event Registration Interface
    // ====================================================================

    /**
     * @brief Raise a fault/exception
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        void raiseFault(const PendingEvent& ev) noexcept {
        setPendingEvent(ev);
    }

    /**
     * @brief Set pending event
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        void setPendingEvent(const PendingEvent& ev) noexcept {
        m_pending = ev;

        // Set appropriate flag based on exception class
        uint32_t flag = 0;

        switch (ev.exceptionClass) {
            // Arithmetic traps (TRAPB waits for these)
        case ExceptionClass_EV6::Arithmetic:
            flag = FLAG_ARITHMETIC_TRAP;
            break;

            // Data memory faults
        case ExceptionClass_EV6::Dtb_miss_single:
        case ExceptionClass_EV6::Dtb_miss_double_4:
        case ExceptionClass_EV6::Dfault:
        case ExceptionClass_EV6::DtbAcv:
            flag = FLAG_DTB_MISS;
            break;

            // Instruction memory faults
        case ExceptionClass_EV6::ItbMiss:
        case ExceptionClass_EV6::ItbAcv:
            flag = FLAG_ITB_MISS;
            break;

            // Interrupts
        case ExceptionClass_EV6::Interrupt:
            flag = FLAG_INTERRUPT;
            break;

            // Machine check / Critical
        case ExceptionClass_EV6::MachineCheck:
        case ExceptionClass_EV6::Reset:
        case ExceptionClass_EV6::BugCheck:
            flag = FLAG_MACHINE_CHECK;
            break;

            // Generic exceptions
        case ExceptionClass_EV6::Fen:
        case ExceptionClass_EV6::OpcDec:
        case ExceptionClass_EV6::OpcDecFault:
        case ExceptionClass_EV6::Unalign:
        case ExceptionClass_EV6::PerformanceMonitor:
            flag = FLAG_EXCEPTION;
            break;

        default:
            flag = FLAG_EXCEPTION;
            WARN_LOG(QString("FaultDispatcher[%1]: Unknown exception class %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(ev.exceptionClass)));
            break;
        }

        // Set flag (no atomic - per-CPU only)
        if (flag != 0) {
            m_pendingFlags |= flag;
        }

        // Notify CPU via IPR mailbox (this DOES need atomic - cross-thread write)
        auto& cbox = globalIPRCBox(m_cpuId);
        cbox.setHasPendingEvent(true);

        DEBUG_LOG(QString("FaultDispatcher[%1]: Event pending, class=%2, flag=0x%3")
            .arg(m_cpuId)
            .arg(static_cast<int>(ev.exceptionClass))
            .arg(flag, 0, 16));
    }

    /**
     * @brief Clear all pending events
     */
   AXP_HOT AXP_ALWAYS_INLINE
       void clearPendingEvents() noexcept {
       m_pending.clear();
       m_pendingFlags = 0;
       // Clear CBox master poll flag
       auto& cbox = globalCBox(m_cpuId);
       cbox.setHasPendingEvent(false);
   }

    /**
     * @brief Clear pending trap (after handling)
     */
   AXP_HOT AXP_ALWAYS_INLINE 
        void clearPendingTrap() noexcept {
        // For TRAPB: clear arithmetic traps only
        m_pendingFlags &= ~FLAG_ARITHMETIC_TRAP;

        // If the pending event was arithmetic, clear it
        if (m_pending.exceptionClass == ExceptionClass_EV6::Arithmetic) {
            m_pending.clear();
        }
    }

    /**
     * @brief Flush pending traps (for TRAPB)
     *
     * TRAPB waits for all pending arithmetic traps to be delivered.
     * In functional emulation, traps are delivered immediately,
     * so this is a NOP - just ensures memory ordering.
     */
	AXP_HOT AXP_FLATTEN
		void flushPendingTraps() noexcept {
		// Functional emulation: arithmetic traps are immediate.
		// TRAPB semantics: "wait for all pending arithmetic traps"
		// In our model, there's nothing to wait for - they're already delivered.

		// Optional: Add memory fence if needed for cross-CPU visibility
		// std::atomic_thread_fence(std::memory_order_seq_cst);
	}

    // ====================================================================
    // AST (Asynchronous System Trap) Checking
    // ====================================================================

    /**
     * @brief Check for pending AST
     *
     * Called at instruction boundaries when AST delivery is allowed.
     */
	AXP_HOT AXP_FLATTEN
		void checkAST() noexcept
	{
		

		// 1. Read ASTSR (SSOT)
        quint8 astsr = m_iprGlobalMaster->h->astsr;
		//quint8 astsr = cold.astsr;
		if (astsr == 0) return;

		// 2. Read enable mask
		quint8 astenMask = static_cast<quint8>(m_iprGlobalMaster->h->aster & 0x0F);
		if (astenMask == 0) return;

		// 3. Get current mode
		quint8 currentMode = m_iprGlobalMaster->h->getCM();

		// 4. Determine deliverable based on mode
		quint8 deliverable = 0;
		if (astsr & 0x01) deliverable |= 0x01;
		if ((astsr & 0x02) && (currentMode >= 1)) deliverable |= 0x02;
		if ((astsr & 0x04) && (currentMode >= 2)) deliverable |= 0x04;
		if ((astsr & 0x08) && (currentMode >= 3)) deliverable |= 0x08;

		// 5. Mask by enabled
		quint8 enabled = deliverable & astenMask;
		if (enabled == 0) return;

		// 6. Select highest priority (lowest bit)
		quint8 astMode = (enabled & 0x01) ? 0 :
			(enabled & 0x02) ? 1 :
			(enabled & 0x04) ? 2 : 3;

		// 7. Clear bit
	/*	cold.astsr &= ~(1 << astMode);*/
		quint8 current = m_iprGlobalMaster->h->astsr;
		quint8 updated = current & ~(1 << astMode);  // Clear the specific bit
        m_iprGlobalMaster->h->astsr = ( updated);


		// 8. Create event
		PendingEvent astEvent;
		astEvent.kind = PendingEventKind::Ast;
		astEvent.exceptionClass = ExceptionClass_EV6::Interrupt;
		astEvent.astMode = astMode;
		astEvent.faultPC = m_iprGlobalMaster->h->pc;
		astEvent.cm = currentMode;
		astEvent.palVectorId = PalVectorId_EV6::INTERRUPT;

		setPendingEvent(astEvent);
	}

    // ====================================================================
    // Code Modification Tracking
    // ====================================================================

    /**
     * @brief Report code modification (for decode cache invalidation)
     */
   AXP_HOT inline void reportCodeModification(uint64_t startPC, uint64_t endPC) const noexcept {
        DEBUG_LOG(QString("FaultDispatcher[%1]: Code modified 0x%2-0x%3")
            .arg(m_cpuId)
            .arg(startPC, 16, 16, QChar('0'))
            .arg(endPC, 16, 16, QChar('0')));

        // AlphaCPU should call IBox::invalidateVA() for the range
    }

    /**
     * @brief Handle code modification event (compatibility)
     */
   inline void handleCodeModificationEvent(uint64_t startPC, uint64_t endPC) noexcept {
        reportCodeModification(startPC, endPC);
    }

    // ====================================================================
    // Accessors
    // ====================================================================

    CPUIdType cpuId() const noexcept { return m_cpuId; }

private:
    // ====================================================================
    // Helper Methods
    // ====================================================================

    /**
     * @brief Find highest set bit (for AST priority)
     */
    static uint8_t findHighestSetBit(uint64_t value) noexcept {
        if (value == 0) return 0;

        // Find highest set bit (63 downto 0)
        uint8_t bit = 63;
        while (bit > 0 && ((value >> bit) & 1) == 0) {
            --bit;
        }
        return bit;
    }

    // ====================================================================
    // Member Data
    // ====================================================================

    CPUIdType m_cpuId;
    PendingEvent m_pending;       // Event details
    uint32_t m_pendingFlags;      // Per-CPU 

    CPUStateView  m_cpuView;                            // value member
    CPUStateView* m_iprGlobalMaster{ &m_cpuView };

    // ====================================================================
    // Pending Event Flags (Bitmask)
    // ====================================================================

    enum PendingFlags : uint32_t {
        FLAG_NONE = 0x00,
        FLAG_EXCEPTION = 0x01,        // Generic exception
        FLAG_ARITHMETIC_TRAP = 0x02,  // Arithmetic trap (TRAPB)
        FLAG_DTB_MISS = 0x04,         // Data TLB miss
        FLAG_ITB_MISS = 0x08,         // Instruction TLB miss
        FLAG_INTERRUPT = 0x10,        // Interrupt pending
        FLAG_MACHINE_CHECK = 0x20     // Machine check (highest priority)
    };
};

#endif // FAULTDISPATCHER_H
