#include "CPURegistry.h"
#include <QString>
#include "coreLib/LoggingMacros.h"
#include "AlphaCPU.h"
// ============================================================================
// CPU REGISTRY IMPLEMENTATIONS
// ============================================================================

AXP_HOT AXP_FLATTEN  void CPURegistry::addCPU(AlphaCPU* cpu) noexcept
{
	if (!cpu) {
		WARN_LOG("CPURegistry: Attempted to add null CPU");
		return;
	}

	// Resize vector if needed
	if (m_cpus.size() <= static_cast<int>(m_cpuCount)) {
		m_cpus.resize(m_cpuCount);
	}

	// Find first available slot or append
	bool added = false;
	for (int i = 0; i < m_cpus.size(); ++i) {
		if (m_cpus[i] == nullptr) {
			m_cpus[i] = cpu;
			added = true;
			DEBUG_LOG(QString("CPURegistry: CPU added at slot %1").arg(i));
			break;
		}
	}

	if (!added) {
		m_cpus.append(cpu);
		DEBUG_LOG(QString("CPURegistry: CPU added at slot %1").arg(m_cpus.size() - 1));
	}

	DEBUG_LOG(QString("CPURegistry: Total CPUs registered: %1").arg(m_cpus.size()));
}

AXP_HOT AXP_FLATTEN  void CPURegistry::removeCPU(CPUIdType cpuId) noexcept
{
	if (!isValidCPU(cpuId)) {
		WARN_LOG(QString("CPURegistry: Invalid CPU ID %1 for removal").arg(cpuId));
		return;
	}

	if (cpuId < static_cast<CPUIdType>(m_cpus.size())) {
		if (m_cpus[cpuId] != nullptr) {
			m_cpus[cpuId] = nullptr;
			DEBUG_LOG(QString("CPURegistry: CPU %1 removed").arg(cpuId));
		}
		else {
			WARN_LOG(QString("CPURegistry: CPU %1 slot already empty").arg(cpuId));
		}
	}
}

AXP_HOT AXP_FLATTEN  AlphaCPU* CPURegistry::getCPU(CPUIdType cpuId) const noexcept
{
	if (!isValidCPU(cpuId)) {
		return nullptr;
	}

	if (cpuId < static_cast<CPUIdType>(m_cpus.size())) {
		return m_cpus[cpuId];
	}

	return nullptr;
}

AXP_HOT AXP_FLATTEN  bool CPURegistry::isValidCPU(CPUIdType cpuId) const noexcept
{
	return cpuId < m_cpuCount && cpuId < static_cast<CPUIdType>(m_cpus.size());
}

AXP_HOT AXP_FLATTEN  void CPURegistry::setCPUCount(quint16 count) noexcept
{
	if (count > MAX_CPUS) {
		WARN_LOG(QString("CPURegistry: Requested CPU count %1 exceeds maximum %2").arg(count).arg(MAX_CPUS));
		count = MAX_CPUS;
	}

	if (count != m_cpuCount) {
		DEBUG_LOG(QString("CPURegistry: CPU count changed from %1 to %2").arg(m_cpuCount).arg(count));
		m_cpuCount = count;

		// Resize CPU vector
		m_cpus.resize(count);

		// Initialize new slots to nullptr
		for (int i = 0; i < m_cpus.size(); ++i) {
			if (m_cpus[i] == nullptr) {
				m_cpus[i] = nullptr;  // Explicit initialization
			}
		}
	}
}

AXP_HOT AXP_FLATTEN  QString CPURegistry::getRegistryStatus() const noexcept
{
	QString status;
	status += QString("CPURegistry Status:\n");
	status += QString("  Configured CPUs: %1\n").arg(m_cpuCount);
	status += QString("  Registry size: %1\n").arg(m_cpus.size());

	quint16 activeCpus = 0;
	for (int i = 0; i < m_cpus.size(); ++i) {
		if (m_cpus[i] != nullptr) {
			activeCpus++;
			status += QString("  CPU %1: Active\n").arg(i);
		}
		else {
			status += QString("  CPU %1: Empty\n").arg(i);
		}
	}

	status += QString("  Active CPUs: %1/%2\n").arg(activeCpus).arg(m_cpuCount);
	return status;
}

