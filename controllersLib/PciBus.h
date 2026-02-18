// ============================================================================
// PciBus.H  -  PCI Bus Model (Bus / Slot / Function Registry + IRQ Routing)
// ============================================================================
// Purpose:
//   Represents ONE PCI bus in the emulator. Provides:
//     • Device lookup by (bus, slot, function)
//     • Enumeration of all slots
//     • INTx (INTA#/INTB#/INTC#/INTD#) routing to system IRQ lines
//
//   This is separate from PciDeviceManager:
//     - PciBus knows topology & routing
//     - PciDeviceManager knows BAR allocation & mmioDevice pointers
//     - EmulatorManager coordinates discovery and MMIO installation
//
//   Future expansion:
//     - Multi-hose support (hose0/hose1/…)
//     - Bridge devices (PCI-to-PCI bridge)
//     - PCI Express (optional)
//
// Dependencies:
//   - QtCore
//   - PciDeviceManager
//   - No SafeMemory / AlphaCPU / MMIOManager dependency.
//
// ============================================================================

#ifndef PCI_BUS_H
#define PCI_BUS_H

#include <QtGlobal>
#include <QString>
#include <QVector>
#include <QMap>

#include "PciDeviceManager.H"

// ============================================================================
// PciIntxLine  (INTA# .. INTD#)
// ============================================================================
enum class PciIntxLine : quint8
{
	INTA = 0,
	INTB = 1,
	INTC = 2,
	INTD = 3
};

// ============================================================================
// PciInterruptRoute
// ============================================================================
// Maps a PCI INTx signal to a system interrupt vector (e.g., IPL or irqLine).
//
// Example:
//     INTA -> irq 24
//     INTB -> irq 25
//     INTC -> irq 26
//     INTD -> irq 27
//
// ============================================================================
struct PciInterruptRoute
{
	quint8  irqLine;     // target IRQ line in your system IRQ controller
	bool    levelTrigger;
	bool    activeLow;

	PciInterruptRoute() noexcept
		: irqLine(0)
		, levelTrigger(true)
		, activeLow(true)
	{
	}
};

// ============================================================================
// PciBus
// ============================================================================
//
// The PciBus object holds:
//   • Bus number
//   • IRQ routing overrides
//   • Device registration (references to PciRegisteredDevice objects)
//
// Device storage rule:
//   - The bus does NOT own the devices or BARs.
//   - Devices are owned by EmulatorManager and registered via PciDeviceManager.
//
// Integration path:
//   1. Emulator loads configuration
//   2. PciScsiConfigBinder creates controllers
//   3. EmulatorManager:
//         bus.registerDevice(ptrToRegisteredDevice);
//   4. EmulatorManager:
//         mmioManager->mapBarsFrom(pciDeviceManager.devices());
//
// ============================================================================
class PciBus
{
public:
	explicit PciBus(quint8 busNumber = 0) noexcept
		: m_busNumber(busNumber)
	{
		// Default INTx routing (can be overridden by config)
		for (int i = 0; i < 4; ++i)
			m_intRouting[i] = defaultRouteFor(static_cast<PciIntxLine>(i));
	}

	~PciBus() noexcept = default;

	// ------------------------------------------------------------------------
	// Bus number
	// ------------------------------------------------------------------------
	inline quint8 busNumber() const noexcept
	{
		return m_busNumber;
	}

	// ------------------------------------------------------------------------
	// Register a device (does NOT allocate BARs)
	// ------------------------------------------------------------------------
	//
	// Caller supplies a pointer to a PciRegisteredDevice created by
	// PciDeviceManager. The bus stores that pointer for routing and lookup.
	//
	inline void registerDevice(PciRegisteredDevice* dev) noexcept
	{
		if (!dev)
			return;

		m_devices.append(dev);
	}

	// ------------------------------------------------------------------------
	// Lookup device by (slot, function)
	// ------------------------------------------------------------------------
	inline PciRegisteredDevice* lookup(quint8 slot,
		quint8 logFunction) noexcept
	{
		for (auto* dev : m_devices)
		{
			if (dev->location.slot == slot &&
				dev->location.logFunction == logFunction)
			{
				return dev;
			}
		}
		return nullptr;
	}

	inline const PciRegisteredDevice* lookup(quint8 slot,
		quint8 logFunction) const noexcept
	{
		for (auto* dev : m_devices)
		{
			if (dev->location.slot == slot &&
				dev->location.logFunction == logFunction)
			{
				return dev;
			}
		}
		return nullptr;
	}

	// ------------------------------------------------------------------------
	// Listing devices
	// ------------------------------------------------------------------------
	inline const QVector<PciRegisteredDevice*>& devices() const noexcept
	{
		return m_devices;
	}

	// ------------------------------------------------------------------------
	// INTx routing (PCI -> system IRQ controller)
	// ------------------------------------------------------------------------
	inline void setInterruptRoute(PciIntxLine intx,
		quint8 irqLine,
		bool level = true,
		bool activeLow = true) noexcept
	{
		m_intRouting[static_cast<int>(intx)].irqLine = irqLine;
		m_intRouting[static_cast<int>(intx)].levelTrigger = level;
		m_intRouting[static_cast<int>(intx)].activeLow = activeLow;
	}

	inline const PciInterruptRoute& route(PciIntxLine intx) const noexcept
	{
		return m_intRouting[static_cast<int>(intx)];
	}

private:
	quint8 m_busNumber;
	QVector<PciRegisteredDevice*> m_devices;
	PciInterruptRoute m_intRouting[4];

	static inline PciInterruptRoute defaultRouteFor(PciIntxLine intx) noexcept
	{
		PciInterruptRoute r;
		r.irqLine = 20 + static_cast<quint8>(intx);  // Example default
		r.levelTrigger = true;
		r.activeLow = true;
		return r;
	}
};

#endif // PCI_BUS_H

