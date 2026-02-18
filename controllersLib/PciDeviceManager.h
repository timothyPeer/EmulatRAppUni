// ============================================================================
// PciDeviceManager.H  -  PCI Device Registry and BAR Allocation Helper
// ============================================================================
// Purpose:
//   This header defines a small, header-only PCI device manager used to:
//
//     • Track PCI devices by (bus, slot, function)
//     • Assign non-overlapping BAR address ranges via PciBarAllocator
//     • Associate MMIO-capable devices with their BARs
//
//   It does NOT talk to SafeMemory or MMIOManager directly. Instead, it
//   records the BAR assignments so a higher-level component (such as your
//   EmulatorManager or MMIOManager integration code) can later map:
//
//       BAR base + size  ->  device.mmioReadXX / mmioWriteXX
//
// Design constraints:
//   - Header-only, no .CPP
//   - Depends only on QtCore, PciBarAllocator.H, and PciScsiMmioInterface.H
//   - Pure ASCII (UTF-8, no BOM)
//   - No dependency on AlphaCPU, SafeMemory, or PAL.
//
// Typical usage:
//   1) Construct a PciDeviceManager with a MMIO window:
//        PciDeviceManager pciMgr(0x80000000ULL, 0x90000000ULL, 0x1000ULL);
//
//   2) For each PCI SCSI controller (e.g., KZPBA_Controller) built by
//      PciScsiConfigBinder, register it:
//
//        auto* devRec = pciMgr.registerScsiDevice(
//                          "PKB0",
//                          cfg.pciBus, cfg.pciSlot, cfg.pciFunction,
//                          pciCtrl,                   // PciScsiMmioInterface*
//                          pciCtrl->barSize(),       // size of BAR0
//                          pciCtrl->associatedBarIndex());
//
//   3) Later, your MMIOManager can iterate over pciMgr.devices() and
//      map each BAR region to the device's MMIO handler.
// ============================================================================

#ifndef PCI_DEVICE_MANAGER_H
#define PCI_DEVICE_MANAGER_H

#include <QtGlobal>
#include <QString>
#include <QVector>

#include "PciBarAllocator.H"
#include "PciScsiMmioInterface.H"

// ============================================================================
// PciLocation
// ============================================================================
//
// Encodes a PCI device location using (bus, slot, function).
// ============================================================================
struct PciLocation
{
	quint8 bus;
	quint8 slot;
	quint8 logFunction;

	PciLocation() noexcept
		: bus(0)
		, slot(0)
		, logFunction(0)
	{
	}

	PciLocation(quint8 b, quint8 s, quint8 f) noexcept
		: bus(b)
		, slot(s)
		, logFunction(f)
	{
	}

	inline bool equals(quint8 b, quint8 s, quint8 f) const noexcept
	{
		return (bus == b) && (slot == s) && (logFunction == f);
	}
};

// ============================================================================
// PciRegisteredDevice
// ============================================================================
//
// Represents one PCI device in the system, including:
//
//   - Device name (e.g., "PKB0")
//   - Location (bus, slot, function)
//   - Pointer to a MMIO-capable device (PciScsiMmioInterface)
//   - One or more BAR windows allocated via PciBarAllocator
//
// NOTE:
//   The PciDeviceManager does NOT own the device pointer. Lifetime is
//   managed by a higher-level component (e.g., EmulatorManager).
// ============================================================================
struct PciRegisteredDevice
{
	QString                name;
	PciLocation            location;
	PciScsiMmioInterface* mmioDevice;   // non-owning
	QVector<PciBarInfo>    bars;         // indexed by BAR number (0..5)

	PciRegisteredDevice() noexcept
		: name()
		, location()
		, mmioDevice(nullptr)
		, bars()
	{
	}
};

// ============================================================================
// PciDeviceManager
// ============================================================================
//
// A simple registry and BAR allocator for PCI devices. It is intentionally
// generic but primarily used for MMIO-capable devices like your SCSI HBAs.
//
// This manager:
//
//   - Owns a PciBarAllocator for a MMIO window.
//   - Provides registerScsiDevice() to register SCSI controllers that
//     implement PciScsiMmioInterface.
//   - Stores BAR allocations for later use by MMIOManager.
//
// It does NOT:
//
//   - Map BARs into SafeMemory directly.
//   - Route interrupts (that is handled by your IRQController).
// ============================================================================
class PciDeviceManager
{
public:
	// ------------------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   windowBase    - starting physical address of the PCI MMIO window
	//   windowLimit   - exclusive end address of the window
	//   alignment     - minimum BAR alignment (e.g., 0x1000 for 4KiB)
	//
	PciDeviceManager(quint64 windowBase,
		quint64 windowLimit,
		quint64 alignment = 0x1000ULL) noexcept
		: m_allocator(windowBase, windowLimit, alignment)
	{
	}

	~PciDeviceManager() noexcept = default;

	// ------------------------------------------------------------------------
	// registerScsiDevice
	// ------------------------------------------------------------------------
	//
	// Registers a MMIO-capable PCI SCSI controller and allocates a single
	// BAR window for it. This is tailored to the common pattern where your
	// controller uses only BAR0 for MMIO, as described by:
	//
	//   - barIndex (usually 0)
	//   - barSize  (controller->barSize())
	//
	// Parameters:
	//   name      - human-readable device name ("PKB0", "PKC0")
	//   bus       - PCI bus number
	//   slot      - PCI slot number
	//   function  - PCI function number
	//   mmioDev   - pointer to the MMIO interface (PciScsiMmioInterface)
	//   barSize   - size of the BAR in bytes
	//   barIndex  - which BAR number (0..5) to use, default 0
	//   isMemory  - true for memory space, false for I/O space
	//   is64Bit   - true if 64-bit BAR
	//   prefetch  - PCI prefetchable attribute
	//
	// Returns:
	//   Pointer to the registered device record, or nullptr on failure
	//   (e.g., no MMIO space left).
	//
	PciRegisteredDevice* registerScsiDevice(const QString& name,
		quint8                   bus,
		quint8                   slot,
		quint8                   logFunction,
		PciScsiMmioInterface* mmioDev,
		quint32                  barSize,
		int                      barIndex = 0,
		bool                     isMemory = true,
		bool                     is64Bit = false,
		bool                     prefetch = false) noexcept
	{
		if (!mmioDev || barSize == 0 || barIndex < 0 || barIndex > 5)
		{
			return nullptr;
		}

		// Allocate BAR space from MMIO window.
		PciBarInfo barInfo = m_allocator.allocate(barSize, isMemory, is64Bit, prefetch);
		if (barInfo.size == 0)
		{
			// Allocation failed.
			return nullptr;
		}

		PciRegisteredDevice dev;
		dev.name = name;
		dev.location = PciLocation(bus, slot, logFunction);
		dev.mmioDevice = mmioDev;
		dev.bars.resize(barIndex + 1);
		dev.bars[barIndex] = barInfo;

		m_devices.append(dev);
		return &m_devices.last();
	}

	// ------------------------------------------------------------------------
	// Device lookup
	// ------------------------------------------------------------------------

	inline PciRegisteredDevice* findDevice(quint8 bus,
		quint8 slot,
		quint8 logFunction) noexcept
	{
		for (int i = 0; i < m_devices.size(); ++i)
		{
			if (m_devices[i].location.equals(bus, slot, logFunction))
			{
				return &m_devices[i];
			}
		}
		return nullptr;
	}

	inline const PciRegisteredDevice* findDevice(quint8 bus,
		quint8 slot,
		quint8 logFunction) const noexcept
	{
		for (int i = 0; i < m_devices.size(); ++i)
		{
			if (m_devices[i].location.equals(bus, slot, logFunction))
			{
				return &m_devices[i];
			}
		}
		return nullptr;
	}

	// ------------------------------------------------------------------------
	// Access all devices
	// ------------------------------------------------------------------------

	inline QVector<PciRegisteredDevice>& devices() noexcept
	{
		return m_devices;
	}

	inline const QVector<PciRegisteredDevice>& devices() const noexcept
	{
		return m_devices;
	}

	// ------------------------------------------------------------------------
	// Expose allocator state (for diagnostics)
	// ------------------------------------------------------------------------

	inline quint64 windowBase() const noexcept
	{
		return m_allocator.current() - usedBytes();
	}

	inline quint64 windowLimit() const noexcept
	{
		return m_allocator.limit();
	}

	inline quint64 alignment() const noexcept
	{
		return m_allocator.alignment();
	}

private:
	PciBarAllocator           m_allocator;
	QVector<PciRegisteredDevice> m_devices;

	// Computes total bytes allocated so far.
	quint64 usedBytes() const noexcept
	{
		quint64 total = 0;
		for (int i = 0; i < m_devices.size(); ++i)
		{
			const QVector<PciBarInfo>& bars = m_devices[i].bars;
			for (int j = 0; j < bars.size(); ++j)
			{
				total += static_cast<quint64>(bars[j].size);
			}
		}
		return total;
	}
};

#endif // PCI_DEVICE_MANAGER_H
