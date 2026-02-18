// ============================================================================
// PciScsiController.H  -  Generic PCI-attached SCSI Controller Base
// ============================================================================
// This header begins the PCI integration layer that will sit *above*
// controllerLib (ScsiControllerLib.H) and *below* your emulator-specific
// PCI/MMIO/coreLib world.
//
// Goals:
//   - Provide a neutral base class for PCI SCSI controllers.
//   - Encapsulate PCI config-space and BAR layout for a single device.
//   - Connect a GenericScsiHostAdapter to the PCI-facing world via a
//     ScsiHostAdapterBackend implementation.
//   - Remain independent of AlphaCPU, SafeMemory, MMIOManager, etc.
//
// Non-goals (left for your emulator-specific code):
//   - Actual PCI bus enumeration
//   - MMIO read/write handlers
//   - IRQ wiring and delivery to CPUs
//   - DMA engines and guest memory mapping
//
// Design constraints:
//   - Header-only (no .CPP file).
//   - Depends on QtCore and ScsiControllerLib.H.
//   - Pure ASCII; UTF-8 (no BOM).
//
// Integration pattern:
//   1. Your emulator's PCI layer defines a PciDevice or similar class.
//   2. That PciDevice derives from PciScsiControllerBase and implements:
//        - configSpaceRead / configSpaceWrite hooks
//        - barRead / barWrite (via MMIO) that trigger SCSI I/O
//   3. The derived class also implements ScsiHostAdapterBackend and sets
//      itself as the backend for the GenericScsiHostAdapter.
//
// This keeps SCSI logic and PCI/MMIO logic decoupled while still giving you
// a clear bridge point between the two.
// ============================================================================

#ifndef PCI_SCSI_CONTROLLER_H
#define PCI_SCSI_CONTROLLER_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QMutex>

#include "ScsiControllerLib.H"
#include "ScsiHostAdapterBackend.H"

// ============================================================================
// PciBarDescriptor - describes one PCI BAR
// ============================================================================

struct PciBarDescriptor
{
	quint64 baseAddress;   // guest-physical address (MMIO or I/O space)
	quint32 size;          // size in bytes
	bool    isMemory;      // true = memory space, false = I/O space
	bool    is64Bit;       // true = BAR is 64-bit capable
	bool    isPrefetchable;// true = prefetchable memory

	PciBarDescriptor() noexcept
		: baseAddress(0)
		, size(0)
		, isMemory(true)
		, is64Bit(false)
		, isPrefetchable(false)
	{
	}
};

// ============================================================================
// PciConfigSpace - minimal config-space representation
// ============================================================================
//
// This struct captures only the standard header fields that are typically
// relevant for a SCSI controller device. Additional vendor-specific registers
// can be modeled by extending or embedding this struct in your PCI layer.
//
// ============================================================================

struct PciConfigSpace
{
	quint16 vendorId;
	quint16 deviceId;
	quint16 command;
	quint16 status;
	quint8  revisionId;
	quint8  progIf;
	quint8  subclass;
	quint8  classCode;
	quint8  cacheLineSize;
	quint8  latencyTimer;
	quint8  headerType;
	quint8  bist;

	// Up to 6 BARs (typical PCI).
	PciBarDescriptor bars[6];

	quint8  interruptLine;
	quint8  interruptPin;

	PciConfigSpace() noexcept
		: vendorId(0xFFFF)
		, deviceId(0xFFFF)
		, command(0)
		, status(0)
		, revisionId(0)
		, progIf(0)
		, subclass(0)
		, classCode(0)
		, cacheLineSize(0)
		, latencyTimer(0)
		, headerType(0)
		, bist(0)
		, interruptLine(0xFF)
		, interruptPin(0)
	{
	}
};

// ============================================================================
// PciScsiControllerBase
// ============================================================================
//
// This abstract base class provides:
//   - A PciConfigSpace instance and helpers for BAR setup.
//   - A GenericScsiHostAdapter instance for SCSI I/O.
//   - A pointer to a ScsiHostAdapterBackend used for transaction completion.
//
// Your emulator-specific PCI device class is expected to:
//   - Derive from PciScsiControllerBase.
//   - Also implement ScsiHostAdapterBackend (or aggregate one).
//   - Wire MMIO/IO accesses to SCSI command submission via the adapter.
//
// ============================================================================

class PciScsiControllerBase : public ScsiHostAdapterBackend
{
public:
	// Constructor
	//
	// Parameters:
	//   bus           - pointer to ScsiBus; the PCI device binds to this bus.
	//   initiatorName - logical name for the SCSI initiator (e.g. "PCI-SCSI0").
	//   initiatorWwn  - 64-bit identity; can be configured from JSON later.
	//
	PciScsiControllerBase(ScsiBus* bus,
		const QString& initiatorName,
		quint64    initiatorWwn) noexcept
		: m_adapter(bus, initiatorName, initiatorWwn, true)
		, m_name("PciScsiController")
	{
		// Bind this base as the adapter backend by default. A more complex
		// design might allow a different backend, but this works well when
		// your derived PCI device also extends this class.
		m_adapter.setBackend(this);
	}

	virtual ~PciScsiControllerBase() noexcept override
	{
		m_adapter.stopIoThread();
	}

	// ------------------------------------------------------------------------
	// ScsiHostAdapterBackend interface
	// ------------------------------------------------------------------------

	// Human-readable name (can be overridden).
	virtual QString backendName() const noexcept override
	{
		return m_name;
	}

	// submit():
	//   Default behavior: treat this as an async submission into the
	//   GenericScsiHostAdapter worker thread. Derived classes may override
	//   to implement doorbell semantics or different queueing behavior.
	virtual void submit(ScsiHostAdapter* adapter,
		ScsiTransaction* txn) noexcept override
	{
		GenericScsiHostAdapter* g =
			dynamic_cast<GenericScsiHostAdapter*>(adapter);
		if (!g || !txn)
		{
			if (txn)
			{
				txn->cmd.setCheckCondition(scsiSense_InternalHardwareError());
				txn->markCompleted(false);
			}
			return;
		}

		// Ensure the IO thread is running to service this request.
		if (!g->isIoThreadRunning())
		{
			g->startIoThread();
		}

		g->submitAsync(txn);
	}

	// onTransactionComplete():
	//   Default behavior: no-op. Derived PCI devices should override this
	//   to:
	//     - post completion into internal queues
	//     - update guest-visible status registers
	//     - assert an interrupt line via their PCI/IRQ interface
	virtual void onTransactionComplete(const ScsiTransaction& txn) noexcept override
	{
		Q_UNUSED(txn);
	}

	// ------------------------------------------------------------------------
	// Accessors
	// ------------------------------------------------------------------------

	inline GenericScsiHostAdapter& adapter() noexcept
	{
		return m_adapter;
	}

	inline const GenericScsiHostAdapter& adapter() const noexcept
	{
		return m_adapter;
	}

	inline PciConfigSpace& config() noexcept
	{
		return m_cfg;
	}

	inline const PciConfigSpace& config() const noexcept
	{
		return m_cfg;
	}

	inline void setName(const QString& n) noexcept
	{
		m_name = n;
	}

	inline QString name() const noexcept
	{
		return m_name;
	}

	// ------------------------------------------------------------------------
	// PCI BAR configuration helpers
	// ------------------------------------------------------------------------

	// Configure a BAR descriptor.
	//
	// Parameters:
	//   index        - BAR index 0..5
	//   size         - size of region in bytes (power-of-two typical)
	//   isMem        - true = memory space, false = I/O space
	//   is64         - true = 64-bit BAR
	//   prefetchable - true = prefetchable
	//
	inline void configureBar(int index,
		quint32 size,
		bool    isMem,
		bool    is64,
		bool    prefetchable) noexcept
	{
		if (index < 0 || index >= 6)
			return;

		m_cfg.bars[index].size = size;
		m_cfg.bars[index].isMemory = isMem;
		m_cfg.bars[index].is64Bit = is64;
		m_cfg.bars[index].isPrefetchable = prefetchable;
	}

	// Assign a guest-physical base address to a BAR (typically called by
	// your PCI bus when it allocates address space).
	inline void setBarBaseAddress(int index, quint64 base) noexcept
	{
		if (index < 0 || index >= 6)
			return;
		m_cfg.bars[index].baseAddress = base;
	}

	inline PciBarDescriptor bar(int index) const noexcept
	{
		if (index < 0 || index >= 6)
		{
			return PciBarDescriptor();
		}
		return m_cfg.bars[index];
	}

	// ------------------------------------------------------------------------
	// PCI config-space accessors (for your PCI bus layer)
	// ------------------------------------------------------------------------
	//
	// These helpers do NOT perform any guest-facing emulation. They just
	// read and write the local PciConfigSpace structure. Your PCI bus /
	// MMIO layer is free to call them when decoding guest config reads/writes.
	//
	// The expectation is that your PCI bus layer handles:
	//   - 8/16/32-bit partial reads/writes
	//   - Any side effects of enabling/disabling memory/IO/interrupts
	//
	// You can extend these helpers later, or implement your own in the
	// derived PCI device class.
	//
	// ------------------------------------------------------------------------

	// Return raw pointer to the config-space byte array.
	// Often useful when your PCI bus wants to memcpy 256 bytes.
	inline void exportConfigSpace(QByteArray& out) const noexcept
	{
		out.resize(64); // standard header is 64 bytes; extend as needed
		std::memset(out.data(), 0, static_cast<size_t>(out.size()));

		// Minimal population of standard header fields.
		// Offsets here follow PCI specification (Type 0 header).
		quint8* p = reinterpret_cast<quint8*>(out.data());

		// Vendor ID / Device ID
		p[0x00] = static_cast<quint8>(m_cfg.vendorId & 0xFF);
		p[0x01] = static_cast<quint8>((m_cfg.vendorId >> 8) & 0xFF);
		p[0x02] = static_cast<quint8>(m_cfg.deviceId & 0xFF);
		p[0x03] = static_cast<quint8>((m_cfg.deviceId >> 8) & 0xFF);

		// Command / Status
		p[0x04] = static_cast<quint8>(m_cfg.command & 0xFF);
		p[0x05] = static_cast<quint8>((m_cfg.command >> 8) & 0xFF);
		p[0x06] = static_cast<quint8>(m_cfg.status & 0xFF);
		p[0x07] = static_cast<quint8>((m_cfg.status >> 8) & 0xFF);

		// Revision / class codes
		p[0x08] = m_cfg.revisionId;
		p[0x09] = m_cfg.progIf;
		p[0x0A] = m_cfg.subclass;
		p[0x0B] = m_cfg.classCode;

		// Cache line size, latency timer, headerType, BIST
		p[0x0C] = m_cfg.cacheLineSize;
		p[0x0D] = m_cfg.latencyTimer;
		p[0x0E] = m_cfg.headerType;
		p[0x0F] = m_cfg.bist;

		// BARs - lower 32 bits only for now.
		for (int i = 0; i < 6; ++i)
		{
			const int off = 0x10 + i * 4;
			const quint32 barLo =
				static_cast<quint32>(m_cfg.bars[i].baseAddress & 0xFFFFFFFFu);
			p[off + 0] = static_cast<quint8>(barLo & 0xFF);
			p[off + 1] = static_cast<quint8>((barLo >> 8) & 0xFF);
			p[off + 2] = static_cast<quint8>((barLo >> 16) & 0xFF);
			p[off + 3] = static_cast<quint8>((barLo >> 24) & 0xFF);
		}

		// Interrupt line / pin
		p[0x3C] = m_cfg.interruptLine;
		p[0x3D] = m_cfg.interruptPin;
	}

protected:
	GenericScsiHostAdapter m_adapter;
	PciConfigSpace         m_cfg;
	QString                m_name;
};

#endif // PCI_SCSI_CONTROLLER_H

