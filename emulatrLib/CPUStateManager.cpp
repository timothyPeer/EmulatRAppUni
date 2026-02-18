// ============================================================================
// CPUStateManager_impl.h
// ============================================================================
// Complete implementation for CPUStateManager component
// ============================================================================

#ifndef CPUSTATEMANAGER_IMPL_H
#define CPUSTATEMANAGER_IMPL_H

#include "CPUStateManager.h"
#include "grainFactoryLib/MemoryBarrier_core.h"
#include <QMutex>
#include <QMutexLocker>
#include "../coreLib/types_core.h"
#include "../coreLib/LoggingMacros.h"
#include "coreLib/Axp_Attributes_core.h"

// ============================================================================
// CPU STATE MANAGER IMPLEMENTATIONS
// ============================================================================

AXP_HOT AXP_FLATTEN CPUStateManager::CPUStateManager() noexcept
{
	// Initialize CPU quiescence state
	for (int i = 0; i < MAX_CPUS; i++) {
		m_quiescenceState[i].drainRequested.store(false);
		m_quiescenceState[i].pendingStores.store(0);
		m_quiescenceState[i].state = CPUState::Running;
	}

	// Initialize CPU masks
	m_haltedCpuMask.store(0, std::memory_order_release);
	m_waitingCpuMask.store(0, std::memory_order_release);

	DEBUG_LOG("CPUStateManager: Initialized with all CPUs in running state");
}

AXP_HOT AXP_FLATTEN void CPUStateManager::setCPUHalted(CPUIdType cpuId, bool halted) noexcept
{
	if (!isValidCPU(cpuId)) {
		WARN_LOG(QString("CPUStateManager: Invalid CPU %1 for halt state").arg(cpuId));
		return;
	}

	updateCPUMask(m_haltedCpuMask, cpuId, halted);

	// Update CPU state
	auto& state = m_quiescenceState[cpuId];
	QMutexLocker locker(&state.mutex);
	state.state = halted ? CPUState::Halted : CPUState::Running;

	DEBUG_LOG(QString("CPUStateManager: CPU %1 %2").arg(cpuId).arg(halted ? "halted" : "resumed"));
}

AXP_HOT AXP_FLATTEN bool CPUStateManager::isCPUHalted(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return false;
	}

	quint64 mask = m_haltedCpuMask.load(std::memory_order_acquire);
	return (mask & (1ULL << cpuId)) != 0;
}

AXP_HOT AXP_FLATTEN void CPUStateManager::notifyCPUHalted(CPUIdType cpuId, quint32 haltCode) noexcept
{
	setCPUHalted(cpuId, true);

	INFO_LOG(QString("CPUStateManager: CPU %1 halted with code 0x%2")
		.arg(cpuId).arg(haltCode, 8, 16, QChar('0')));
}

AXP_HOT AXP_FLATTEN void CPUStateManager::setCPUWaiting(CPUIdType cpuId, bool waiting) noexcept
{
	if (!isValidCPU(cpuId)) {
		WARN_LOG(QString("CPUStateManager: Invalid CPU %1 for wait state").arg(cpuId));
		return;
	}

	updateCPUMask(m_waitingCpuMask, cpuId, waiting);

	// Update CPU state
	auto& state = m_quiescenceState[cpuId];
	QMutexLocker locker(&state.mutex);
	if (waiting && state.state == CPUState::Running) {
		state.state = CPUState::Waiting;
	}
	else if (!waiting && state.state == CPUState::Waiting) {
		state.state = CPUState::Running;
	}

	DEBUG_LOG(QString("CPUStateManager: CPU %1 %2").arg(cpuId).arg(waiting ? "waiting" : "resumed"));
}

AXP_HOT AXP_FLATTEN bool CPUStateManager::isCPUWaiting(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return false;
	}

	quint64 mask = m_waitingCpuMask.load(std::memory_order_acquire);
	return (mask & (1ULL << cpuId)) != 0;
}

AXP_HOT AXP_FLATTEN void CPUStateManager::requestQuiescence(CPUIdType cpuId) noexcept
{
	if (!isValidCPU(cpuId)) {
		return;
	}

	auto& state = m_quiescenceState[cpuId];
	state.drainRequested.store(true, std::memory_order_release);

	DEBUG_LOG(QString("CPUStateManager: Quiescence requested for CPU %1").arg(cpuId));
}

void CPUStateManager::waitForQuiescence(CPUIdType cpuId) noexcept {
	auto& state = m_quiescenceState[cpuId];
	QMutexLocker locker(&state.mutex);

	DEBUG_LOG(QString("CPUStateManager: Waiting for quiescence on CPU %1").arg(cpuId));

	constexpr int MAX_WAIT_MS = 1000;
	// Proper condition variable pattern: always check condition inside mutex
	while (state.pendingStores.load(std::memory_order_acquire) > 0 &&
		state.state != CPUState::Quiesced) {
		if (!state.quiescedCondition.wait(&state.mutex, MAX_WAIT_MS)) {
			WARN_LOG(QString("CPUStateManager: Quiescence wait timeout for CPU %1").arg(cpuId));
			break;
		}
	}

	// Update state under lock
	if (state.pendingStores.load(std::memory_order_acquire) == 0) {
		state.state = CPUState::Quiesced;
		DEBUG_LOG(QString("CPUStateManager: CPU %1 quiesced").arg(cpuId));
	}
}

AXP_HOT AXP_FLATTEN void CPUStateManager::signalQuiescence(CPUIdType cpuId) noexcept
{
	if (!isValidCPU(cpuId)) {
		return;
	}

	auto& state = m_quiescenceState[cpuId];
	QMutexLocker locker(&state.mutex);

	state.state = CPUState::Quiesced;
	state.drainRequested.store(false, std::memory_order_release);
	state.quiescedCondition.wakeAll();

	DEBUG_LOG(QString("CPUStateManager: Quiescence signaled for CPU %1").arg(cpuId));
}

AXP_HOT AXP_FLATTEN bool CPUStateManager::isQuiescent(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return false;
	}

	const auto& state = m_quiescenceState[cpuId];
	return state.pendingStores.load(std::memory_order_acquire) == 0 &&
		state.state == CPUState::Quiesced;
}

AXP_HOT AXP_FLATTEN void CPUStateManager::registerPendingStore(CPUIdType cpuId) noexcept
{
	if (!isValidCPU(cpuId)) {
		return;
	}

	auto& state = m_quiescenceState[cpuId];
	quint32 pending = state.pendingStores.fetch_add(1, std::memory_order_acq_rel);

	if (pending == 0) {
		DEBUG_LOG(QString("CPUStateManager: CPU %1 now has pending stores").arg(cpuId));
	}
}

void CPUStateManager::completePendingStore(CPUIdType cpuId) noexcept {
	auto& state = m_quiescenceState[cpuId];
	quint32 pending = state.pendingStores.fetch_sub(1, std::memory_order_acq_rel);

	if (pending == 1) {
		// Take mutex to check drainRequested atomically with signaling
		QMutexLocker locker(&state.mutex);
		if (state.drainRequested.load(std::memory_order_acquire)) {
			state.state = CPUState::Quiesced;
			state.drainRequested.store(false, std::memory_order_release);
			state.quiescedCondition.wakeAll();
			locker.unlock();  // Release before logging
			DEBUG_LOG(QString("CPUStateManager: CPU %1 quiesced").arg(cpuId));
		}
	}
}

AXP_HOT AXP_FLATTEN quint32 CPUStateManager::getPendingStoreCount(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return 0;
	}

	return m_quiescenceState[cpuId].pendingStores.load(std::memory_order_acquire);
}

AXP_HOT AXP_FLATTEN CPUStateManager::CPUState CPUStateManager::getCPUState(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return CPUState::Reset;
	}

	return m_quiescenceState[cpuId].state;
}

AXP_HOT AXP_FLATTEN quint64 CPUStateManager::getActiveCPUMask() const noexcept
{
	quint64 halted = m_haltedCpuMask.load(std::memory_order_acquire);
	quint64 waiting = m_waitingCpuMask.load(std::memory_order_acquire);

	// Active = not halted and not waiting
	quint64 inactive = halted | waiting;
	quint64 allCpus = (1ULL << MAX_CPUS) - 1;  // All possible CPUs

	return allCpus & ~inactive;
}

AXP_HOT AXP_FLATTEN quint16 CPUStateManager::getActiveCPUCount() const noexcept
{
	quint64 activeMask = getActiveCPUMask();

	// Count set bits (popcount)
	quint16 count = 0;
	while (activeMask) {
		count += (activeMask & 1);
		activeMask >>= 1;
	}

	return count;
}

AXP_HOT AXP_FLATTEN void CPUStateManager::resetCPU(CPUIdType cpuId) noexcept
{
	if (!isValidCPU(cpuId)) {
		return;
	}

	// Clear halted bit
	updateCPUMask(m_haltedCpuMask, cpuId, false);

	// Clear waiting bit
	updateCPUMask(m_waitingCpuMask, cpuId, false);

	// Clear quiescence state
	auto& state = m_quiescenceState[cpuId];
	QMutexLocker locker(&state.mutex);
	state.drainRequested.store(false, std::memory_order_release);
	state.pendingStores.store(0, std::memory_order_release);
	state.state = CPUState::Running;

	INFO_LOG(QString("CPUStateManager: CPU %1 reset and revived").arg(cpuId));
}

AXP_HOT AXP_FLATTEN void CPUStateManager::resetAllCPUs() noexcept
{
	DEBUG_LOG("CPUStateManager: Resetting all CPUs");

	for (CPUIdType cpuId = 0; cpuId < MAX_CPUS; ++cpuId) {
		resetCPU(cpuId);
	}

	INFO_LOG("CPUStateManager: All CPUs reset");
}

AXP_HOT AXP_FLATTEN QString CPUStateManager::getCPUStateString(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return "Invalid";
	}

	switch (getCPUState(cpuId)) {
	case CPUState::Running: return "Running";
	case CPUState::Halted: return "Halted";
	case CPUState::Waiting: return "Waiting";
	case CPUState::Quiesced: return "Quiesced";
	case CPUState::Reset: return "Reset";
	default: return "Unknown";
	}
}

AXP_HOT AXP_FLATTEN QString CPUStateManager::getAllCPUStatesString() const noexcept
{
	QString status;
	status += QString("CPUStateManager Status:\n");
	status += QString("  Active CPU mask: 0x%1\n").arg(getActiveCPUMask(), 16, 16, QChar('0'));
	status += QString("  Active CPU count: %1\n").arg(getActiveCPUCount());

	for (CPUIdType cpuId = 0; cpuId < MAX_CPUS; ++cpuId) {
		status += QString("  CPU %1: %2 (pending stores: %3)\n")
			.arg(cpuId)
			.arg(getCPUStateString(cpuId))
			.arg(getPendingStoreCount(cpuId));
	}

	return status;
}

// ============================================================================
// PRIVATE HELPER IMPLEMENTATIONS
// ============================================================================

void CPUStateManager::updateCPUMask(std::atomic<quint64>& mask,
	CPUIdType cpuId, bool set) noexcept {
	quint64 oldMask = mask.load(std::memory_order_acquire);
	quint64 cpuBit = 1ULL << cpuId;

	if (set) {
		mask.fetch_or(cpuBit, std::memory_order_acq_rel);
	}
	else {
		mask.fetch_and(~cpuBit, std::memory_order_acq_rel);
	}

	quint64 newMask = mask.load(std::memory_order_acquire);
	DEBUG_LOG(QString("CPU mask updated: CPU=%1 set=%2 old=0x%3 new=0x%4")
		.arg(cpuId).arg(set)
		.arg(oldMask, 16, 16, QChar('0'))
		.arg(newMask, 16, 16, QChar('0')));
}
#endif // CPUSTATEMANAGER_IMPL_H
