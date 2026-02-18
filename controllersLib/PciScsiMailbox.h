// ============================================================================
// PciScsiMailbox.H  -  Generic Mailbox / Doorbell Interface for PCI SCSI
// ============================================================================
// Purpose:
//   Many PCI SCSI controllers provide a memory-mapped "mailbox" or
//   doorbell-style register mechanism whereby the guest OS submits:
//
//       - CDBs
//       - Buffer descriptors
//       - Task management commands
//       - Queue depth settings
//       - Interrupt acknowledge commands
//
//   This header defines a device-agnostic abstraction suitable for use by
//   any PCI SCSI controller emulation:
//
//       - A mailbox command descriptor
//       - A decoded command type enumeration
//       - A mailbox queue interface
//       - No assumptions about device architecture
//
// Design constraints:
//   - Header-only
//   - QtCore + controllerLib only
//   - Pure ASCII (UTF-8, no BOM)
//   - No AlphaCPU, No SafeMemory, No MMIOManager, No PALcode
//
// ============================================================================

#ifndef PCI_SCSI_MAILBOX_H
#define PCI_SCSI_MAILBOX_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QVector>

// ============================================================================
// PciScsiMailboxCommandType
// ============================================================================
// These are HIGH-LEVEL decoded mailbox command types expected from a controller.
// Each concrete device will add specific subcommands, but these are universal.
// ============================================================================
enum class PciScsiMailboxCommandType : quint8
{
	Invalid = 0,
	SubmitCdb = 1,    // Guest submits a CDB ? controller creates ScsiTransaction
	AbortTask = 2,    // Abort a queued task
	ResetBus = 3,    // Reset the SCSI bus
	ResetDevice = 4,    // RESET LUN
	FlushQueue = 5,    // Drop/abort all pending tasks
	InterruptAcknowledge = 6,    // Guest acknowledges an interrupt
	HostAdapterInquiry = 7     // Guest reads adapter capabilities
};

// ============================================================================
// PciScsiMailboxCommand
// ============================================================================
// Generic decoded mailbox command as seen by a PCI SCSI controller.
// Device-specific classes may extend or embed custom metadata.
// ============================================================================
struct PciScsiMailboxCommand
{
	PciScsiMailboxCommandType type;

	quint8  targetId;      // For CDB submission, aborts, resets
	quint8  lun;           // Logical unit number
	quint64 guestAddress;  // Optional: buffer pointer guest provided
	quint32 length;        // Optional: buffer length
	QByteArray cdb;        // Optional CDB contents (0–16 bytes typically)

	// Additional opaque bits the device may interpret.
	quint32 flags;

	PciScsiMailboxCommand() noexcept
		: type(PciScsiMailboxCommandType::Invalid)
		, targetId(0)
		, lun(0)
		, guestAddress(0)
		, length(0)
		, flags(0)
	{
	}
};

// ============================================================================
// PciScsiMailboxQueue
// ============================================================================
// A purely software mailbox FIFO queue used by PCI SCSI controllers.
//
// The controller calls:
//     enqueueCommand(...)
//     dequeueCommand()
// The PCI/MMIO layer decodes mailbox writes and produces commands.
// ============================================================================
class PciScsiMailboxQueue
{
public:
	PciScsiMailboxQueue() noexcept = default;

	inline void enqueue(const PciScsiMailboxCommand& cmd) noexcept
	{
		m_queue.append(cmd);
	}

	inline bool hasPending() const noexcept
	{
		return !m_queue.isEmpty();
	}

	inline PciScsiMailboxCommand dequeue() noexcept
	{
		if (m_queue.isEmpty())
		{
			return PciScsiMailboxCommand();
		}
		return m_queue.takeFirst();
	}

	inline void clear() noexcept
	{
		m_queue.clear();
	}

	inline int count() const noexcept
	{
		return m_queue.size();
	}

private:
	QVector<PciScsiMailboxCommand> m_queue;
};

#endif // PCI_SCSI_MAILBOX_H
