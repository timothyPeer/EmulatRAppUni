// ============================================================================
// PciScsiConfigBinder.H  -  Bind SettingsLoader -> PCI SCSI Controllers
// ============================================================================
// Purpose:
//   This header connects your existing settings / SettingsLoader layer:
//
//       EmulatorSettings
//         +- controllers (PKB0, PKC0, etc.)
//         +- devices     (DKA0, DKB1, MKA600, etc.)
//
//   to the new PCI SCSI controller stack:
//
//       - PciScsiDeviceFactory
//       - ISP1020_Controller / KZPBA_Controller
//       - VirtualScsiController (ScsiBus + ScsiController)
//       - PciScsiDeviceShell (PCI+MMIO+SCSI host adapter)
//
//   It does NOT depend on AlphaCPU, SafeMemory, or MMIOManager.
//   It ONLY wires configuration objects to SCSI/PCI controller objects.
//
// Design constraints:
//   - Header-only, no CPP.
//   - QtCore + settings.h + PciScsiLib.H only.
//   - Pure ASCII, UTF-8 (no BOM).
//
// TODO (explicit):
//   - Implement per-device binding (SCSI_DISK, SCSI_TAPE, SCSI_CDROM, etc.)
//     once VirtualScsiDisk / VirtualTapeDevice / VirtualIsoDevice (or
//     equivalents) are finalized.
// ============================================================================

#ifndef PCI_SCSI_CONFIG_BINDER_H
#define PCI_SCSI_CONFIG_BINDER_H

#include <QtGlobal>
#include <QString>
#include <QMap>
#include <QVector>

#include "settings.h"
#include "PciScsiLib.H"

// Forward declaration to avoid forcing a specific device header here.
// Concrete virtual SCSI devices will live in scsiCoreLib.
class VirtualScsiDevice;

// ============================================================================
// BoundScsiController
// ============================================================================
//
// Represents one configured SCSI HBA from EmulatorSettings.controllers,
// bound to:
//
//   - A VirtualScsiController (owns ScsiBus + ScsiController)
//   - A PCI SCSI device (PciScsiDeviceShell, e.g. KZPBA_Controller)
//
struct BoundScsiController
{
	QString             name;          // Controller name: "PKB0", "PKC0"
	ControllerConfig    cfg;           // Original config block

	VirtualScsiController* vscsi = nullptr;       // Owns ScsiBus + ScsiController
	PciScsiDeviceShell* pciController = nullptr; // ISP1020/KZPBA/etc.

	// Convenience accessors
	inline ScsiBus* scsiBus() const noexcept
	{
		return vscsi ? &vscsi->bus() : nullptr;
	}
};

// ============================================================================
// BoundScsiDevice
// ============================================================================
//
// Represents one configured SCSI target device from EmulatorSettings.devices,
// bound to a VirtualScsiDevice (disk, tape, cdrom, etc.) and attached to a
// parent controller by (parent, scsi_id, unit/lun).
//
struct BoundScsiDevice
{
	QString        name;      // Device name: "DKA0", "MKA600", etc.
	DeviceConfig   cfg;       // Original device config block

	VirtualScsiDevice* device = nullptr; // Concrete virtual device (disk/tape/iso)
};

// ============================================================================
// PciScsiConfigBinder
// ============================================================================
//
// Usage:
//
//   SettingsLoader loader;
//   loader.load("ASAEmulatr.ini");
//   const EmulatorSettings& cfg = loader.getSettings();
//
//   PciScsiConfigBinder binder;
//   binder.bind(cfg);
//
//   auto controllers = binder.controllers();
//   auto devices     = binder.devices();
//
// Memory ownership:
//   - PciScsiConfigBinder allocates all VirtualScsiController and
//     PciScsiDeviceShell instances it creates and deletes them in
//     its destructor.
//   - VirtualScsiDevice instances created in bindDevices() are also
//     owned and deleted by this binder.
//   - Caller should treat returned pointers as non-owning.
//
// ============================================================================

class PciScsiConfigBinder
{
public:
	explicit PciScsiConfigBinder(bool threadSafe = false) noexcept
		: m_threadSafe(threadSafe)
	{
	}

	~PciScsiConfigBinder() noexcept
	{
		// Clean up controllers and their vscsi/pciController objects.
		for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it)
		{
			BoundScsiController& bc = it.value();
			delete bc.pciController;
			delete bc.vscsi;
		}

		// Clean up devices.
		for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
		{
			BoundScsiDevice& bd = it.value();
			delete bd.device;
		}
	}

	// ------------------------------------------------------------------------
	// Top-level bind entry point
	// ------------------------------------------------------------------------
	//
	// Returns true if all SCSI HBAs were successfully created.
	// Device binding is currently a TODO; failure there is logged/loggable,
	// but not treated as a hard failure yet.
	//
	bool bind(const EmulatorSettings& settings) noexcept
	{
		m_controllers.clear();
		m_devices.clear();

		if (!bindControllers(settings))
		{
			return false;
		}

		// NOTE:
		//   Device binding is partially stubbed and marked as TODO for now.
		bindDevices(settings);

		return true;
	}

	// ------------------------------------------------------------------------
	// Accessors
	// ------------------------------------------------------------------------

	inline const QMap<QString, BoundScsiController>& controllers() const noexcept
	{
		return m_controllers;
	}

	inline const QMap<QString, BoundScsiDevice>& devices() const noexcept
	{
		return m_devices;
	}

private:
	bool m_threadSafe;

	QMap<QString, BoundScsiController> m_controllers;
	QMap<QString, BoundScsiDevice>     m_devices;

	// ------------------------------------------------------------------------
	// Controller binding
	// ------------------------------------------------------------------------
	//
	// For each ControllerConfig in settings.controllers:
	//   - If classType == "SCSI_HBA":
	//       * Create VirtualScsiController (owns ScsiBus)
	//       * Create PciScsiDeviceShell (KZPBA, ISP1020, etc.)
	//       * Store in m_controllers[name]
	//
	bool bindControllers(const EmulatorSettings& settings) noexcept
	{
		int index = 0;

		auto it = settings.controllers.constBegin();
		for (; it != settings.controllers.constEnd(); ++it)
		{
			const QString& key = it.key();
			const ControllerConfig& cfg = it.value();

			if (!cfg.classType.compare("SCSI_HBA", Qt::CaseInsensitive) == 1
				&& cfg.classType != "SCSI_HBA")
			{
				// Non-SCSI controllers are ignored by this binder.
				continue;
			}

			BoundScsiController bc;
			bc.name = cfg.name.isEmpty() ? key : cfg.name;
			bc.cfg = cfg;

			// Create a VirtualScsiController (ScsiBus + ScsiController).
			bc.vscsi = new VirtualScsiController(m_threadSafe);

			ScsiBus* scsiBus = bc.scsiBus();
			if (!scsiBus)
			{
				delete bc.vscsi;
				bc.vscsi = nullptr;
				return false;
			}

			// Synthesize a WWN from controller name + index.
			const quint64 wwn = synthesizeWwn(bc.name, index++);

			// Use PciScsiDeviceFactory to construct the PCI SCSI controller.
			PciScsiDeviceShell* pciCtrl = PciScsiDeviceFactory::createByType(
				cfg.controllerType, scsiBus, bc.name, wwn, m_threadSafe);

			if (!pciCtrl)
			{
				delete bc.vscsi;
				bc.vscsi = nullptr;
				return false;
			}

			bc.pciController = pciCtrl;

			// Optional: set IRQ line / BAR index from cfg.fields later.
			// TODO: Map cfg.fields["pci_bus"], ["pci_slot"], ["pci_function"],
			//       and DeviceIrq irq into your PCI bus + IRQ manager.

			m_controllers.insert(bc.name, bc);
		}

		return true;
	}

	// ------------------------------------------------------------------------
	// Device binding (SCSI targets)
	// ------------------------------------------------------------------------
	//
	// For each DeviceConfig in settings.devices:
	//   - Find parent controller by cfg.parent
	//   - Decode scsi_id and unit (LUN)
	//   - Create appropriate VirtualScsiDevice (disk, tape, iso) from
	//     DeviceConfig.fields + subBlocks (container, geometry, identity)
	//   - Attach to the parent's VirtualScsiController via attachDevice()
	//
	// NOTE:
	//   This is intentionally left as a TODO placeholder until your
	//   VirtualScsiDisk/VirtualTapeDevice/VirtualIsoDevice interfaces
	//   are finalized. For now it only parses basic fields.
	//
	void bindDevices(const EmulatorSettings& settings) noexcept
	{
		auto it = settings.devices.constBegin();
		for (; it != settings.devices.constEnd(); ++it)
		{
			const QString& devName = it.key();
			const DeviceConfig& cfg = it.value();

			BoundScsiDevice bd;
			bd.name = cfg.name.isEmpty() ? devName : cfg.name;
			bd.cfg = cfg;
			bd.device = nullptr; // Created later by a VirtualScsiDeviceFactory.

			// Find parent controller.
			const QString parentName = cfg.parent;
			auto ctrlIt = m_controllers.find(parentName);
			if (ctrlIt == m_controllers.end())
			{
				// No matching SCSI_HBA; this device will be ignored for now.
				m_devices.insert(bd.name, bd);
				continue;
			}

			BoundScsiController& bc = ctrlIt.value();
			if (!bc.vscsi)
			{
				m_devices.insert(bd.name, bd);
				continue;
			}

			// Extract SCSI target ID and LUN from fields.
			const int targetId = cfg.fields.value("scsi_id").toInt();
			const int lunInt = cfg.fields.value("unit").toInt();
			const ScsiLun lun(static_cast<quint16>(lunInt));

			// TODO:
			//   Instantiate the correct VirtualScsiDevice subclass here.
			//   Suggested design:
			//
			//     VirtualScsiDevice* dev =
			//         VirtualScsiDeviceFactory::createFromDeviceConfig(cfg);
			//
			//   Then:
			//
			//     if (dev) {
			//         bc.vscsi->attachDevice(static_cast<quint8>(targetId),
			//                                lun,
			//                                dev);
			//         bd.device = dev;
			//     }
			//
			//   For now, we only record the parsed target/lun relationship as
			//   part of the config; no device is created.

			// Store bound device state (even if bd.device == nullptr for now).
			m_devices.insert(bd.name, bd);
		}
	}

	// ------------------------------------------------------------------------
	// WWN synthesizer
	// ------------------------------------------------------------------------
	//
	// Creates a synthetic, stable WWN from controller name + index.
	// This is sufficient for identification within the emulator; you can
	// replace it later with a real WWN scheme if desired.
	//
	quint64 synthesizeWwn(const QString& controllerName, int index) const noexcept
	{
		// Simple scheme:
		//   high 32 bits  = fixed magic
		//   low  32 bits  = qHash(name) ^ index
		const quint32 magic = 0x50000000u;
		const quint32 hash = static_cast<quint32>(qHash(controllerName)) ^ static_cast<quint32>(index);

		return (static_cast<quint64>(magic) << 32)
			| static_cast<quint64>(hash);
	}
};

#endif // PCI_SCSI_CONFIG_BINDER_H
