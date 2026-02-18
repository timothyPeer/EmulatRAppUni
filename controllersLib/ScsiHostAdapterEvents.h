// ============================================================================
// ScsiHostAdapterEvents.H  -  Event/Notification Structures for controllerLib
// ============================================================================
// This header defines simple event types used for:
//   - Logging
//   - Debug tracing
//   - State-change notifications (queue, start, completion)
//   - Future GUI or emulator monitoring
//
// It remains intentionally generic and header-only.
//
// Design constraints:
//   - Header-only (no CPP)
//   - Depends only on QtCore + scsiCoreLib + controllerLib headers
//   - Pure ASCII
//   - NO coreLib, NO MMIO, NO CPU, NO PALcode references
// ============================================================================

#ifndef SCSI_HOST_ADAPTER_EVENTS_H
#define SCSI_HOST_ADAPTER_EVENTS_H

#include <QtGlobal>
#include <QString>
#include <QDateTime>

#include "ScsiCoreLib.H"
#include "ScsiTransaction.H"

// ---------------------------------------------------------------------------
// Event type enum
// ---------------------------------------------------------------------------
enum class ScsiEventType : quint8
{
	TransactionQueued = 0,
	TransactionStarted,
	TransactionCompleted,
	BusReset,
	DeviceAttached,
	DeviceDetached,
	AdapterOnline,
	AdapterOffline
};

// ---------------------------------------------------------------------------
// Generic SCSI event structure
// ---------------------------------------------------------------------------
struct ScsiHostAdapterEvent
{
	ScsiEventType type;
	quint64       transactionId;
	QString       message;
	QDateTime     timestamp;

	ScsiHostAdapterEvent() noexcept
		: type(ScsiEventType::TransactionQueued)
		, transactionId(0)
		, message()
		, timestamp(QDateTime::currentDateTimeUtc())
	{
	}

	ScsiHostAdapterEvent(ScsiEventType t,
		quint64       id,
		const QString& msg) noexcept
		: type(t)
		, transactionId(id)
		, message(msg)
		, timestamp(QDateTime::currentDateTimeUtc())
	{
	}
};

#endif // SCSI_HOST_ADAPTER_EVENTS_H
