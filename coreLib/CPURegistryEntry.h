#pragma once
#include <QtGlobal>
#include <QAtomicInt>
#include <QDateTime>
#include <QTimeZone>



/**
 * @brief SMP-aware CPU registration entry
 */
class CPURegistryEntry
{
private:
	AlphaCPU* m_alphaCpu;
	quint16 m_cpuId;
	bool m_isActive;
	bool m_isOnline;
	bool m_isDegraded = false;
	bool m_hasReservation = false;
	quint64 m_registrationAddress = 0;
	QAtomicInt m_pendingInterrupts;
	quint64 m_lastActivity;
	QAtomicInt m_accessCount;

public:
	// Add this equality operator
	bool operator==(const CPURegistryEntry& other) const noexcept {
		return m_cpuId == other.m_cpuId;
	}

	// Optionally add inequality operator
	bool operator!=(const CPURegistryEntry& other) const noexcept {
		return m_cpuId != other.m_cpuId;
	}

	operator quint16() const { return m_cpuId; }

	quint16 cpuId() const { return m_cpuId; }
	bool hasReservation() { return m_hasReservation; }
	void setHasReservation(bool bReserved) { m_hasReservation = bReserved; }
	void setReservationAddr(quint64 regAddress) { m_registrationAddress = regAddress; }
	quint64 registrationAddr() { return m_registrationAddress; }
	quint64 pendingInterrupts() const { m_pendingInterrupts.loadAcquire(); }
	AlphaCPU* alphaCPU() const { return m_alphaCpu; }
	AlphaCPU* alphaCPU() { return m_alphaCpu; }
	bool isActive() const { return m_isActive; }
	bool isOnline() const { return m_isOnline; }
	bool isDegraded() const {
		return m_isDegraded;
	}
	//QDateTime lastActivity() const { return QDateTime::fromSecsSinceEpoch( m_lastActivity,Qt::LocalTime); }
	QDateTime lastActivityUtc() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		return QDateTime::fromSecsSinceEpoch(m_lastActivity, QTimeZone::UTC);
#else
		return QDateTime::fromSecsSinceEpoch(m_lastActivity, Qt::UTC);
#endif
	}
	void setOnline(bool isOnline) { m_isOnline = isOnline; }
	void setActive(bool isActive) { m_isActive = isActive; }
	void setDegraded(bool isDegraded) {
		m_isDegraded = isDegraded;
	}
	void incrementAccessCnt() { ++m_accessCount; }
	quint64 accessCount() { return m_accessCount; }
	void updateLastActivity(quint64 secondsSinceEpoch) { m_lastActivity = secondsSinceEpoch; }
	CPURegistryEntry() : m_alphaCpu(nullptr), m_cpuId(0), m_isActive(false), m_isOnline(false) {}
	CPURegistryEntry(AlphaCPU* c, quint16 id)
		: m_alphaCpu(c), m_cpuId(id), m_isActive(true), m_isOnline(true), m_lastActivity(QDateTime::currentMSecsSinceEpoch())
	{
	}
};
