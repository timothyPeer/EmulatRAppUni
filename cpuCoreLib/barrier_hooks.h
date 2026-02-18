#pragma once
#include "ReservationManager.h"
#include "Global_IPRInterface.h"
#include "HookVTable.h"
#include "barrier_hooks_core.h"

// Full Memory Barrier (most restrictive)
void hostMB(CPUStateIPRInterface* cpu) noexcept {
	std::atomic_thread_fence(std::memory_order_seq_cst);
}

// Write Memory Barrier
void hostWMB(CPUStateIPRInterface* cpu) noexcept {
	std::atomic_thread_fence(std::memory_order_release);
}

// Read Memory Barrier
void hostRMB(CPUStateIPRInterface* cpu) noexcept {
	std::atomic_thread_fence(std::memory_order_acquire);
}

// Store-Conditional Publish
void hostSCPublish(CPUStateIPRInterface* cpuState) noexcept {
	// Typically involves atomic compare-and-swap semantics
	if (cpuState) {
		cpuState->markStoreConditionalComplete();
	}
}

// Clear Reservation
void hostClearReservation(CPUStateIPRInterface* cpuState) noexcept {
	quint8 cpuId = cpuState->cpuId();
	if (cpuState) {
		cpuState->clearReservationSet();
		globalReservationManager::instance().clearAll(cpuId);  // global invalidation
	}
}

namespace DefaultBarriers {
	// Full Memory Barrier (most restrictive)
	void hostMB(CPUStateIPRInterface* cpu) noexcept {
		std::atomic_thread_fence(std::memory_order_seq_cst);
	}

	// Write Memory Barrier
	void hostWMB(CPUStateIPRInterface* cpu) noexcept {
		std::atomic_thread_fence(std::memory_order_release);
	}

	// Read Memory Barrier
	void hostRMB(CPUStateIPRInterface* cpu) noexcept {
		std::atomic_thread_fence(std::memory_order_acquire);
	}

	// Store-Conditional Publish
	void hostSCPublish(CPUStateIPRInterface* cpu) noexcept {
		// Typically involves atomic compare-and-swap semantics
		if (cpu) {
			cpu->markStoreConditionalComplete();
		}
	}

	// Clear Reservation
	void hostClearReservation(CPUStateIPRInterface* cpu) noexcept {
		if (cpu) {
			cpu->clearReservationSet();
		}
	}
}


	// Generation-specific barrier tables
namespace GenerationBarriers {
	// EV4 might have more basic, less sophisticated barriers
	extern const BarrierVTable kBarrierEV4;

	// EV5 with more advanced memory ordering
	extern const BarrierVTable kBarrierEV5;

	// EV6 with most sophisticated memory semantics
	extern const BarrierVTable kBarrierEV6;

	// Default host-based barriers
	extern const BarrierVTable kBarrierHost;

	// No-op barrier (for testing or minimal configurations)
	extern const BarrierVTable kBarrierNoop;
}