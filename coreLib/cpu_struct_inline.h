#pragma once
#include <QtGlobal>


// CPU Access Information
struct CPUAccessInfo
{
	quint16 cpuId = 0;
	qint64 lastAccessTime = 0;
	quint64 accessCount = 0;
	bool hasReservation = false;
	quint64 reservationAddr = 0;
};