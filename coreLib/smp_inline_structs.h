#pragma once
#include <QtGlobal>
#include <QHash>

// SMP Statistics structure
struct SMPStatistics
{
	quint64 totalAccesses = 0;
	quint64 reservationSets = 0;
	quint64 reservationClears = 0;
	quint64 cacheInvalidations = 0;
	quint64 memoryBarriers = 0;
	QHash<quint16, quint64> accessesPerCpu;

	void reset()
	{
		totalAccesses = 0;
		reservationSets = 0;
		reservationClears = 0;
		cacheInvalidations = 0;
		memoryBarriers = 0;
		accessesPerCpu.clear();
	}
};