// ============================================================================
// CPURegistry.h
// ============================================================================
// Clean separation: CPU registration, lookup, and count management only
// Single Responsibility: Manage the collection of CPU instances
// ============================================================================

#ifndef CPUREGISTRY_H
#define CPUREGISTRY_H

#include <QVector>
#include "coreLib/types_core.h"
#include "coreLib/Axp_Attributes_core.h"

// Forward declarations
class AlphaCPU;

// ============================================================================
// CPU REGISTRY - Single responsibility: CPU collection management
// ============================================================================

class CPURegistry {
public:
    // Constructor/Destructor
    CPURegistry() = default;
    ~CPURegistry() = default;

    // Non-copyable
    Q_DISABLE_COPY(CPURegistry)

    // CPU registration
   AXP_HOT AXP_FLATTEN void addCPU(AlphaCPU* cpu) noexcept;
   AXP_HOT AXP_FLATTEN void removeCPU(CPUIdType cpuId) noexcept;

    // CPU lookup
   AXP_HOT AXP_FLATTEN AlphaCPU* getCPU(CPUIdType cpuId) const noexcept;
  AXP_HOT AXP_FLATTEN bool isValidCPU(CPUIdType cpuId) const noexcept;

    // CPU count management
  AXP_HOT AXP_FLATTEN quint16 getCPUCount() const noexcept { return m_cpuCount; }
  AXP_HOT AXP_FLATTEN void setCPUCount(quint16 count) noexcept;

    // CPU enumeration
  AXP_HOT AXP_FLATTEN const QVector<AlphaCPU*>& getAllCPUs() const noexcept { return m_cpus; }

    // Diagnostic
   AXP_HOT AXP_FLATTEN  QString getRegistryStatus() const noexcept;

private:
    quint16 m_cpuCount{1};
    QVector<AlphaCPU*> m_cpus;  // Non-owning pointers
};

#endif // CPUREGISTRY_H
