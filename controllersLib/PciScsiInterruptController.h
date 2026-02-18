// ============================================================================
// PciScsiInterruptController.H  -  PCI SCSI Interrupt Status/Mask Helper
// ============================================================================
// Purpose:
//   This header defines a small, device-agnostic interrupt controller helper
//   for PCI SCSI controllers. It is intended to be used by derived classes
//   of PciScsiDevice (and friends) to manage:
//
//     • Interrupt reason bits (status register)
//     • Interrupt mask bits (enable/disable reasons)
//     • "Pending" evaluation based on (status & mask)
//     • Simple acknowledge/clear helpers
//
//   The *actual* wiring of interrupts to the emulator's IRQ subsystem is
//   still done by the PCI device itself (e.g., by calling raiseInterrupt()
//   or clearInterrupt() on its own implementation).
//
// Design constraints:
//   - Header-only (no CPP).
//   - Depends only on QtCore (QtGlobal, QMutex optional).
//   - Pure ASCII, UTF-8 (no BOM).
//   - No dependency on AlphaCPU, SafeMemory, MMIOManager, PAL, or PTE.
//   - Generic enough to be reused by different PCI SCSI controller models.
//
// Typical usage in a derived PCI SCSI device:
//
//   class MyPciScsi : public PciScsiDevice {
//       PciScsiInterruptController m_irqCtl;
//
//       void someInternalEvent() {
//           m_irqCtl.raiseReason(PciScsiInterruptReason::CommandComplete);
//           if (m_irqCtl.hasPending()) {
//               raiseInterrupt();   // PciScsiDevice's hook
//           }
//       }
//
//       void guestWritesAckRegister(quint32 ackBits) {
//           m_irqCtl.acknowledge(ackBits);
//           if (!m_irqCtl.hasPending()) {
//               clearInterrupt();
//           }
//       }
//   };
//
// ============================================================================

#ifndef PCI_SCSI_INTERRUPT_CONTROLLER_H
#define PCI_SCSI_INTERRUPT_CONTROLLER_H

#include <QtGlobal>
#include <QMutex>

// ============================================================================
// PciScsiInterruptReason
// ============================================================================
//
// These are generic reason codes that map to status bits. A concrete device
// is free to map them into its own register layout. The numeric values are
// chosen so they can be ORed into a 32-bit status word.
//
// ============================================================================
enum class PciScsiInterruptReason : quint32
{
	None = 0x00000000u,
	CommandComplete = 0x00000001u,  // One or more commands completed
	DeviceError = 0x00000002u,  // Error condition reported by target
	BusReset = 0x00000004u,  // SCSI bus reset completed or detected
	QueueFull = 0x00000008u,  // Internal queue is full or blocked
	HostAdapterError = 0x00000010u,  // Internal adapter or firmware error
	Custom0 = 0x00010000u,  // Reserved for device-specific use
	Custom1 = 0x00020000u   // Reserved for device-specific use
};

// Helper to convert reason to bit mask (quint32).
inline quint32 pciScsiReasonToMask(PciScsiInterruptReason reason) noexcept
{
	return static_cast<quint32>(reason);
}

// ============================================================================
// PciScsiInterruptController
// ============================================================================
//
// Simple interrupt status/mask helper:
//   - statusBits: set of asserted interrupt reasons.
//   - maskBits  : which reasons are enabled for signaling to host.
//
// hasPending() returns true if (statusBits & maskBits) != 0.
//
// This class is intentionally agnostic about how interrupts are actually
// delivered. A PCI device typically does:
//
//   - if (irqCtl.hasPending()) raiseInterrupt();
//   - if (!irqCtl.hasPending()) clearInterrupt();
//
// ============================================================================

class PciScsiInterruptController
{
public:
	explicit PciScsiInterruptController(bool threadSafe = false) noexcept
		: m_statusBits(0)
		, m_maskBits(0)
		, m_threadSafe(threadSafe)
	{
	}

	~PciScsiInterruptController() noexcept = default;

	// ------------------------------------------------------------------------
	// Mask control
	// ------------------------------------------------------------------------

	// Set the entire interrupt mask word.
	inline void setMask(quint32 mask) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_maskBits = mask;
	}

	// Get the current interrupt mask word.
	inline quint32 mask() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_maskBits;
	}

	// Enable a specific interrupt reason (set its bit in the mask).
	inline void enableReason(PciScsiInterruptReason reason) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_maskBits |= pciScsiReasonToMask(reason);
	}

	// Disable a specific interrupt reason (clear its bit in the mask).
	inline void disableReason(PciScsiInterruptReason reason) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_maskBits &= ~pciScsiReasonToMask(reason);
	}

	// ------------------------------------------------------------------------
	// Status control
	// ------------------------------------------------------------------------

	// Set the entire interrupt status word.
	inline void setStatus(quint32 status) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_statusBits = status;
	}

	// Get the current interrupt status word.
	inline quint32 status() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_statusBits;
	}

	// Raise (assert) a specific interrupt reason (set its status bit).
	inline void raiseReason(PciScsiInterruptReason reason) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_statusBits |= pciScsiReasonToMask(reason);
	}

	// Clear a specific interrupt reason (clear its status bit).
	inline void clearReason(PciScsiInterruptReason reason) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_statusBits &= ~pciScsiReasonToMask(reason);
	}

	// Clear all status bits.
	inline void clearAll() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_statusBits = 0;
	}

	// Acknowledge (clear) bits indicated by 'ackBits'. This is often used
	// when the guest writes to an interrupt acknowledge register.
	inline void acknowledge(quint32 ackBits) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_statusBits &= ~ackBits;
	}

	// ------------------------------------------------------------------------
	// Pending evaluation
	// ------------------------------------------------------------------------

	// Returns true if any *enabled* interrupt reason is currently set.
	inline bool hasPending() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return (m_statusBits & m_maskBits) != 0;
	}

	// Returns the masked status (statusBits & maskBits).
	inline quint32 pendingMaskedStatus() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return (m_statusBits & m_maskBits);
	}

private:
	quint32        m_statusBits;
	quint32        m_maskBits;
	bool           m_threadSafe;
	mutable QMutex m_mutex;
};

#endif // PCI_SCSI_INTERRUPT_CONTROLLER_H
