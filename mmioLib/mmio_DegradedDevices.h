#pragma once

#include <QString>
#include <QStringList>
#include <QMutexLocker>
#include <QVector>
#include <QDateTime>
#include "../CoreLib/mmio_core.h"
#include "mmiolib_global.h"

// ============================================================================
// DEGRADED DEVICES REPORT (collected during boot)
// ============================================================================

inline mmio_AllocationResult mmio_deviceMap(mmio_Reason rsn) {
	switch (rsn) {
	case mmio_Reason::OK:
		return mmio_AllocationResult::SUCCESS;
	case mmio_Reason::MMIO_EXHAUSTED:
		return mmio_AllocationResult::MMIO_EXHAUSTED;
	case mmio_Reason::IRQ_EXHAUSTED:
		return mmio_AllocationResult::IRQ_EXHAUSTED;
	case mmio_Reason::TEMPLATE_NOT_FOUND:
		return mmio_AllocationResult::TEMPLATE_NOT_FOUND;
	case mmio_Reason::DMA_UNSUPPORTED:
		return mmio_AllocationResult::DMA_NOT_SUPPORTED;
	case mmio_Reason::INIT_FAILED:
		return mmio_AllocationResult::FATAL_BOOT_ABORT;
	case mmio_Reason::PARENT_DISABLED:
		return mmio_AllocationResult::DEGRADED;
		break;
	default:
		break;

	}
}
class  DegradedDevicesReport {
public:
	static DegradedDevicesReport& instance() {
		static DegradedDevicesReport inst;
		return inst;
	}

	void addDegradedDevice(const DegradedDeviceEntry& entry) {
		QMutexLocker locker(&m_lock);
		m_entries.append(entry);
	}
	void addDegradedDevice(const DegradedDeviceInfo& info) {

		DegradedDeviceEntry entry;
		entry.deviceName = info.name;
		entry.location = info.location;
		entry.deviceClass = info.deviceClass;
		/*entry.resolvedTemplate = info.resolvedTemplate;*/
		//entry.reason = allocationResultFromString(info.reason);
		entry.m_degradeDeviceTimestamp = QDateTime::currentDateTime();

		QMutexLocker locker(&m_lock);
		m_entries.append(entry);
	}

	bool hasDegradedDevices() const {
		QMutexLocker locker(&m_lock);
		return !m_entries.isEmpty();
	}

	QString generateReport() const {
		QMutexLocker locker(&m_lock);

		if (m_entries.isEmpty()) {
			return QString();
		}

		QStringList lines;
		lines << "*** DEGRADED DEVICE REPORT ***";
		lines << QString("%1 device(s) disabled due to resource constraints:")
			.arg(m_entries.size());
		lines << "";

		for (const auto& entry : m_entries) {
			QString reasonStr;
			switch (entry.reason) {
			case mmio_Reason::MMIO_EXHAUSTED:
				reasonStr = "MMIO aperture exhausted";
				break;
			case mmio_Reason::IRQ_EXHAUSTED:
				reasonStr = "IRQ vectors exhausted";
				break;
			case mmio_Reason::DMA_UNSUPPORTED:
				reasonStr = "DMA requirements unmet";
				break;
			case mmio_Reason::INIT_FAILED:
				reasonStr = "Device initialization failed";
				break;
			case mmio_Reason::PARENT_DISABLED:
				reasonStr = "Parent controller unavailable";
				break;
			}

			lines << QString("  [%1] %2").arg(entry.deviceName).arg(entry.location);
			lines << QString("      Reason: %1").arg(reasonStr);
			if (!entry.details.isEmpty()) {
				lines << QString("      Details: %1").arg(entry.details);
			}
			lines << "";
		}

		return lines.join('\n');
	}

	void clear() {
		QMutexLocker locker(&m_lock);
		m_entries.clear();
	}

private:
	QVector<DegradedDeviceEntry> m_entries;
	mutable QMutex m_lock;

	DegradedDevicesReport() = default;
};
