// ============================================================================
// ScsiHostAdapterBackend.H  -  Abstract Backend Interface for Host Adapters
// ============================================================================
// This header provides an extremely lightweight abstraction that allows a
// ScsiHostAdapter to communicate with the *outside world* without requiring
// any knowledge of:
//
//   - AlphaCPU
//   - SafeMemory
//   - MMIOManager
//   - DMA engines
//   - PCI configuration space or BARs
//
// Instead, ScsiHostAdapterBackend is a neutral "bridge point" for:
//
//   - Supplying CDBs + data buffers into the adapter
//   - Receiving completion events or callbacks
//   - Integrating SCSI controllers with any platform (emulator, test harness,
//     synthetic driver, Qt GUI testing, future PCI layer)
//
// A real PCI controller (e.g., "NCR53C8xx", "QLogic 1040") would implement
// ScsiHostAdapterBackend and pass commands into a ScsiHostAdapter instance.
//
// Design constraints:
//   - Header-only
//   - QtCore + scsiCoreLib + controllerLib only
//   - Pure ASCII / UTF-8 (no BOM)
//   - Absolutely no coreLib references
//
// ============================================================================

#ifndef SCSI_HOST_ADAPTER_BACKEND_H
#define SCSI_HOST_ADAPTER_BACKEND_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QMutex>

#include "ScsiCoreLib.H"
#include "ScsiTransaction.H"
#include "ScsiHostAdapter.H"

// ============================================================================
// ScsiHostAdapterBackend
// ============================================================================
//
// Abstract class representing the "platform glue" surrounding a host adapter.
// The emulator's PCI/MMIO subsystem will eventually implement this interface.
// For now it allows SCSI to remain cleanly separated.
//
// Provides:
//   - A way to submit CDB/buffer requests upstream
//   - A callback mechanism when transactions complete
//   - Identity information for logging/tracing
//
// ============================================================================

class ScsiHostAdapterBackend
{
public:
	ScsiHostAdapterBackend() noexcept = default;
	virtual ~ScsiHostAdapterBackend() noexcept = default;

	// Human-readable name for debugging / GUI / tracing.
	virtual QString backendName() const noexcept = 0;

	// ------------------------------------------------------------------------
	// Request submission interface
	// ------------------------------------------------------------------------
	//
	// submit():
	//   Called by the emulator's I/O or PCI/MMIO layer (in the future)
	//   to submit an I/O request into the SCSI host adapter.
	//
	// Parameters:
	//   adapter    - target ScsiHostAdapter instance that will process this
	//                request via scheduler or immediate execute.
	//   txn        - a fully-initialized ScsiTransaction* with cmd populated.
	//
	// Ownership:
	//   Caller owns txn. Adapter does NOT delete it.
	//
	virtual void submit(ScsiHostAdapter* adapter,
		ScsiTransaction* txn) noexcept = 0;

	// ------------------------------------------------------------------------
	// Completion callback
	// ------------------------------------------------------------------------
	//
	// onTransactionComplete():
	//   Called by ScsiHostAdapter or higher-level code when a transaction
	//   completes, allowing the backend to notify platform-specific code.
	//
	// Examples of backend behavior:
	//   - Write completion status into a guest-visible register
	//   - Deliver an interrupt
	//   - Signal a future PCI/MMIO subsystem
	//   - Update GUI widgets or logs
	//
	virtual void onTransactionComplete(const ScsiTransaction& txn) noexcept = 0;
};

#endif // SCSI_HOST_ADAPTER_BACKEND_H
