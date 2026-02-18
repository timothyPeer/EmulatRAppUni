// ============================================================================
// PciSubsystem.H  -  PCI Root Complex / Subsystem Manager
// ============================================================================
// Purpose:
//   This header ties together:
//
//     • PciDeviceManager    (BAR allocation, device records)
//     • PciBus              (bus/slot/function + INTx routing)
//     • MMIO-capable devices (e.g., KZPBA_Controller via PciScsiMmioInterface)
//
//   It represents a simple PCI "root complex" with one global MMIO window,
//   multiple PCI buses, and a registry of devices. It does NOT talk to
//   SafeMemory, MMIOManager, AlphaCPU, or IRQController directly. Those are
//   wired in by your EmulatorManager using the exported state.
//
// Design constraints:
//   - Header-only, no .CPP
//   - Depends only on QtCore + PciDeviceManager.H + PciBus.H
//   - Pure ASCII (UTF-8, no BOM)
//
// Typical integration flow:
//
//   1) SettingsLoader loads ASAEmulatr.ini -> EmulatorSettings
//   2) PciScsiConfigBinder builds KZPBA/ISP1020 controllers
//   3) EmulatorManager constructs PciSubsystem:
//        PciSubsystem pci(0x80000000ULL, 0x90000000ULL, 0x1000ULL);
//   4) For each SCSI HBA:
//        auto* rec = pci.registerScsiController(
//                        controllerName,
//                        cfg.pciBus, cfg.pciSlot, cfg.pciFunction,
//                        pciCtrl,
//                        pciCtrl->barSize(),
//                        pciCtrl->associatedBarIndex());
//   5) EmulatorManager walks pci.devices() and pci.buses() to:
//        - map BARs into MMIOManager
//        - connect INTx routes into IRQController.
//
// ============================================================================

#ifndef PCI_SUBSYSTEM_H
#define PCI_SUBSYSTEM_H

#include <QtGlobal>
#include <QString>
#include <QMap>

#include "PciDeviceManager.H"
#include "PciBus.H"

// ============================================================================
// PciSubsystem
// ============================================================================
//
// Represents a group of PCI buses plus a shared MMIO window managed by
// PciDeviceManager. This is effectively a simple PCI root complex.
//
// Notes:
//   - The subsystem owns PciDeviceManager and all PciBus instances.
//   - It does NOT own the underlying device objects (e.g., KZPBA_Controller).
//     Those are owned by higher-level code (EmulatorManager / DeviceManager).
//
// ============================================================================
class PciSubsystem
{
public:
	// ------------------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------------------
	//
	// Parameters:
	//   windowBase  - starting physical address of PCI MMIO window
	//   windowLimit - exclusive end of window
	//   alignment   - minimum alignment for BAR allocations (e.g. 0x1000)
	//
	PciSubsystem(quint64 windowBase,
		quint64 windowLimit,
		quint64 alignment = 0x1000ULL) noexcept
		: m_deviceManager(windowBase, windowLimit, alignment)
	{
	}

	~PciSubsystem() noexcept = default;

	// ------------------------------------------------------------------------
	// Accessors
	// ------------------------------------------------------------------------

	inline PciDeviceManager& deviceManager() noexcept
	{
		return m_deviceManager;
	}

	inline const PciDeviceManager& deviceManager() const noexcept
	{
		return m_deviceManager;
	}

	inline QMap<quint8, PciBus>& buses() noexcept
	{
		return m_buses;
	}

	inline const QMap<quint8, PciBus>& buses() const noexcept
	{
		return m_buses;
	}

	// ------------------------------------------------------------------------
	// Ensure a bus exists (create if missing)
	// ------------------------------------------------------------------------
	inline PciBus* ensureBus(quint8 busNumber) noexcept
	{
		auto it = m_buses.find(busNumber);
		if (it != m_buses.end())
		{
			return &it.value();
		}

		// Insert a new bus
		PciBus bus(busNumber);
		m_buses.insert(busNumber, bus);
		return &m_buses[busNumber];
	}

	inline const PciBus* bus(quint8 busNumber) const noexcept
	{
		auto it = m_buses.find(busNumber);
		if (it == m_buses.end())
		{
			return nullptr;
		}
		return &it.value();
	}

	// ------------------------------------------------------------------------
	// Register a MMIO-capable PCI SCSI Controller
	// ------------------------------------------------------------------------
	//
	// This is a convenience wrapper that:
	//   - Allocates a BAR window via PciDeviceManager
	//   - Registers the resulting PciRegisteredDevice
	//   - Ensures the corresponding PciBus exists
	//   - Attaches the device record to that bus
	//
	// Parameters mirror those of PciDeviceManager::registerScsiDevice().
	//
	// Returns:
	//   Pointer to the registered PciRegisteredDevice, or nullptr on failure.
	//
	PciRegisteredDevice* registerScsiController(const QString& name,
		quint8                busNumber,
		quint8                slot,
		quint8                logFunction,
		PciScsiMmioInterface* mmioDev,
		quint32               barSize,
		int                   barIndex = 0,
		bool                  isMemory = true,
		bool                  is64Bit = false,
		bool                  prefetch = false) noexcept
	{
		// 1) Register with device manager (allocates BAR)
		PciRegisteredDevice* rec =
			m_deviceManager.registerScsiDevice(name,
				busNumber,
				slot,
				logFunction,
				mmioDev,
				barSize,
				barIndex,
				isMemory,
				is64Bit,
				prefetch);
		if (!rec)
		{
			return nullptr;
		}

		// 2) Ensure bus exists and attach device record
		PciBus* bus = ensureBus(busNumber);
		if (!bus)
		{
			// This should not occur, but if it does, caller can still use rec
			return rec;
		}

		bus->registerDevice(rec);
		return rec;
	}

	// ------------------------------------------------------------------------
	// Convenience: lookup device by (bus, slot, function)
	// ------------------------------------------------------------------------
	inline PciRegisteredDevice* findDevice(quint8 busNumber,
		quint8 slot,
		quint8 logFunction) noexcept
	{
		PciBus* bus = nullptr;
		auto it = m_buses.find(busNumber);
		if (it != m_buses.end())
		{
			bus = &it.value();
		}

		if (!bus)
		{
			return nullptr;
		}

		return bus->lookup(slot, logFunction);
	}

	inline const PciRegisteredDevice* findDevice(quint8 busNumber,
		quint8 slot,
		quint8 logFunction) const noexcept
	{
		const PciBus* bus = nullptr;
		auto it = m_buses.find(busNumber);
		if (it != m_buses.end())
		{
			bus = &it.value();
		}

		if (!bus)
		{
			return nullptr;
		}

		return bus->lookup(slot, logFunction);
	}

private:
	PciDeviceManager      m_deviceManager;
	QMap<quint8, PciBus>  m_buses;        // keyed by busNumber
};

#endif // PCI_SUBSYSTEM_H

