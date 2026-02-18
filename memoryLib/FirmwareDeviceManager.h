// ============================================================================
// FirmwareDeviceManager.h (UPDATED - Uses Global Singleton Pattern)
// ============================================================================
// Purpose: SRM Device Tree Manager (as per DEVICE_TREE_INITIALIZATION_SPEC)
//
// This implements the 5-phase device tree initialization:
// - Phase 0: Firmware Context Initialization
// - Phase 1: Platform Root Creation
// - Phase 2: Bus Discovery and Attachment
// - Phase 3: Device Enumeration and Registration
// - Phase 4: Device Finalization and Console Exposure
//
// Integration with ASAEmulatR.ini:
// - Reads device configurations from EmulatorSettings
// - Creates SRM-style device tree (OPA0, DKA0, PKB0, EWA0, etc.)
// - Exposes devices to console (SHOW DEVICE command)
//
// Access Pattern:
// - Use global_FirmwareDeviceManager() to access singleton
// - Use initializeGlobalFirmwareDeviceManager() to initialize
// ============================================================================

#ifndef FIRMWAREDEVICEMANAGER_H
#define FIRMWAREDEVICEMANAGER_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QSharedPointer>
#include "../configLib/settings.h"
#include "../coreLib/LoggingMacros.h"

// ============================================================================
// Device Tree Node Types
// ============================================================================

enum class DeviceNodeType : quint8
{
    PlatformRoot,       // Top-level platform node
    SystemBus,          // System bus
    PCIBus,             // PCI root bus
    ConsoleBus,         // Console I/O bus
    VirtualBus,         // Virtual/firmware bus
    SCSIController,     // SCSI HBA (PKB0, PKC0)
    SCSIDisk,           // SCSI disk (DKA0, DKA1, DKB1)
    SCSITape,           // SCSI tape (MKA600)
    IDEController,      // IDE controller (PQA0)
    IDEDisk,            // IDE disk (DQA0)
    NetworkInterface,   // Ethernet (EWA0)
    ConsoleTerminal,    // UART console (OPA0, OPA1)
    Unknown
};

// ============================================================================
// Device Tree Node
// ============================================================================

struct DeviceNode
{
    // Core properties
    QString name;                       // "OPA0", "PKB0", "DKA0"
    DeviceNodeType deviceNodeType;
    QString busName;                    // Parent bus
    quint32 unit;                       // Unit number
    bool enabled{ true };
    
    // Hardware resources
    quint64 mmioBase{ 0 };
    quint64 mmioSize{ 0 };
    QString irqStr;                     // "auto" or "0x300"
    quint32 irqIpl{ 20 };
    
    // SRM-specific
    QString location;                   // "cab0/drw0/io0/hose0/bus3/slot1"
    QString classType;                  // "SCSI_HBA", "SCSI_DISK", "NIC"
    
    // Configuration (from ASAEmulatR.ini)
    QMap<QString, QVariant> properties;
    
    // Relationships
    QVector<QString> children;          // Child device names
    QString parent;                     // Parent device name
};

// ============================================================================
// Firmware Device Manager
// ============================================================================
// Access via global_FirmwareDeviceManager() function (defined in global_*.h)
// ============================================================================

class FirmwareDeviceManager
{
public:
    // ========================================================================
    // 5-Phase Initialization (SRM Device Tree Spec)
    // ========================================================================
    
    /**
     * @brief Phase 0: Firmware Context Initialization
     * Initialize Firmware Device Manager, SystemOwner=SRM, memory maps
     */
    bool initializePhase0_FirmwareContext(const EmulatorSettings& config)
    {
        INFO_LOG("Device Tree Phase 0: Firmware Context Initialization");
        
        m_config = config;
        m_initialized = false;
        m_nodes.clear();
        
        INFO_LOG("Firmware context initialized");
        return true;
    }
    
    /**
     * @brief Phase 1: Platform Root Creation
     * Create immutable platform root node
     */
    bool initializePhase1_PlatformRoot()
    {
        INFO_LOG("Device Tree Phase 1: Platform Root Creation");
        
        DeviceNode root;
        root.name = "platform";
        root.deviceNodeType = DeviceNodeType::PlatformRoot;
        root.enabled = true;
        
        // Platform properties from config
        root.properties["platform.name"] = "AlphaServer";
        root.properties["platform.model"] = m_config.system.hwModel;
        root.properties["platform.cpu.count"] = m_config.system.processorCount;
        root.properties["platform.memory.size"] = m_config.system.memorySizeGB;
        root.properties["platform.firmware.version"] = "1.0.0";
        
        m_nodes["platform"] = root;
        
        INFO_LOG(QString("Platform root created: %1, %2 CPUs, %3 GB RAM")
            .arg(m_config.system.hwModel)
            .arg(m_config.system.processorCount)
            .arg(m_config.system.memorySizeGB));
        
        return true;
    }
    
    /**
     * @brief Phase 2: Bus Discovery and Attachment
     * Create bus nodes (SystemBus, PCI, Console, Virtual)
     */
    bool initializePhase2_BusDiscovery()
    {
        INFO_LOG("Device Tree Phase 2: Bus Discovery");
        
        // System Bus
        createBusNode("systembus", DeviceNodeType::SystemBus, "platform");
        
        // PCI Root Bus
        createBusNode("pci0", DeviceNodeType::PCIBus, "systembus");
        
        // Console Bus
        createBusNode("consolebus", DeviceNodeType::ConsoleBus, "systembus");
        
        // Virtual Bus (for firmware services)
        createBusNode("virtualbus", DeviceNodeType::VirtualBus, "systembus");
        
        INFO_LOG("Bus discovery complete (4 buses)");
        return true;
    }
    
    /**
     * @brief Phase 3: Device Enumeration and Registration
     * Read ASAEmulatR.ini and create device nodes
     */
    bool initializePhase3_DeviceEnumeration()
    {
        INFO_LOG("Device Tree Phase 3: Device Enumeration");
        
        int deviceCount = 0;
        
        // Enumerate OPA consoles
        for (auto it = m_config.opaConsoles.begin(); 
             it != m_config.opaConsoles.end(); ++it)
        {
            registerConsoleDevice(it.key(), it.value());
            deviceCount++;
        }
        
        // Enumerate controllers (PKB0, PKC0, PQA0, EWA0)
        for (auto it = m_config.controllers.begin();
             it != m_config.controllers.end(); ++it)
        {
            registerController(it.key(), it.value());
            deviceCount++;
        }
        
        // Enumerate devices (DKA0, DKA1, DKB1, DQA0, MKA600)
        for (auto it = m_config.devices.begin();
             it != m_config.devices.end(); ++it)
        {
            registerDevice(it.key(), it.value());
            deviceCount++;
        }
        
        INFO_LOG(QString("Device enumeration complete: %1 devices").arg(deviceCount));
        return true;
    }
    
    /**
     * @brief Phase 4: Device Finalization and Console Exposure
     * Validate, bind console services, mark firmware-ready
     */
    bool initializePhase4_Finalization()
    {
        INFO_LOG("Device Tree Phase 4: Finalization");
        
        // Validate address space (no overlaps)
        if (!validateAddressSpace()) {
            ERROR_LOG("Address space validation failed");
            return false;
        }
        
        // Bind console services to OPA devices
        bindConsoleServices();
        
        // Mark all devices firmware-ready
        for (auto& node : m_nodes) {
            node.properties["firmware.ready"] = true;
        }
        
        m_initialized = true;
        
        INFO_LOG("Device tree finalized and exposed to console");
        dumpDeviceTree();
        
        return true;
    }

    // ========================================================================
    // Query Interface (for SHOW DEVICE, SHOW CONFIG commands)
    // ========================================================================
    
    /**
     * @brief Get device node by name
     */
    const DeviceNode* getDevice(const QString& name) const
    {
        auto it = m_nodes.find(name);
        return (it != m_nodes.end()) ? &it.value() : nullptr;
    }
    
    /**
     * @brief Get all devices of a specific type
     */
    QVector<QString> getDevicesByType(DeviceNodeType type) const
    {
        QVector<QString> result;
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            if (it.value().deviceNodeType == type) {
                result.append(it.key());
            }
        }
        return result;
    }
    
    /**
     * @brief Get all device names (for SHOW DEVICE)
     */
    QVector<QString> getAllDeviceNames() const
    {
        QVector<QString> result;
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            // Skip buses and platform root
            if (it.value().deviceNodeType != DeviceNodeType::PlatformRoot &&
                it.value().deviceNodeType != DeviceNodeType::SystemBus &&
                it.value().deviceNodeType != DeviceNodeType::PCIBus &&
                it.value().deviceNodeType != DeviceNodeType::ConsoleBus &&
                it.value().deviceNodeType != DeviceNodeType::VirtualBus)
            {
                result.append(it.key());
            }
        }
        return result;
    }
    
    /**
     * @brief Check if device tree is initialized
     */
    bool isInitialized() const noexcept
    {
        return m_initialized;
    }
    
    /**
     * @brief Dump device tree to log (for debugging)
     */
	void dumpDeviceTree() const
	{
		try {
			INFO_LOG("\n" + QString(70, '='));
			INFO_LOG("SRM DEVICE TREE");
			INFO_LOG(QString(70, '='));

			int deviceCount = 0;
			for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
				const auto& node = it.value();

				// Skip buses and platform
				if (node.deviceNodeType == DeviceNodeType::PlatformRoot ||
					node.deviceNodeType == DeviceNodeType::SystemBus ||
					node.deviceNodeType == DeviceNodeType::PCIBus ||
					node.deviceNodeType == DeviceNodeType::ConsoleBus ||
					node.deviceNodeType == DeviceNodeType::VirtualBus) {
					continue;
				}

				QString typeStr = node.classType.isEmpty()
					? deviceTypeToString(node.deviceNodeType)
					: node.classType;

				INFO_LOG(QString("  %1: %2 (%3)")
					.arg(node.name, -12)
					.arg(typeStr, -20)
					.arg(node.enabled ? "Online" : "Offline"));

				deviceCount++;

				// Safety limit
				if (deviceCount > 100) {
					ERROR_LOG("Device tree dump exceeded 100 devices - stopping");
					break;
				}
			}

			INFO_LOG(QString(70, '=') + "\n");
			INFO_LOG(QString("Total devices displayed: %1").arg(deviceCount));

		}
		catch (const std::exception& e) {
			ERROR_LOG(QString("Exception in dumpDeviceTree: %1").arg(e.what()));
		}
		catch (...) {
			ERROR_LOG("Unknown exception in dumpDeviceTree");
		}
	}

    // ========================================================================
    // Allow global accessor to construct singleton
    // ========================================================================
    friend FirmwareDeviceManager& global_FirmwareDeviceManager() noexcept;

private:
    // Private constructor for Meyer's singleton
    FirmwareDeviceManager() = default;
    ~FirmwareDeviceManager() = default;
    
    FirmwareDeviceManager(const FirmwareDeviceManager&) = delete;
    FirmwareDeviceManager& operator=(const FirmwareDeviceManager&) = delete;

    // ========================================================================
    // Device Registration Helpers
    // ========================================================================
    
    void createBusNode(const QString& name, DeviceNodeType type, const QString& parent)
    {
        DeviceNode bus;
        bus.name = name;
        bus.deviceNodeType = type;
        bus.parent = parent;
        bus.enabled = true;
        
        m_nodes[name] = bus;
        
        // Add to parent's children
        if (m_nodes.contains(parent)) {
            m_nodes[parent].children.append(name);
        }
    }
    
	void registerConsoleDevice(const QString& name, const OPAConsoleConfig& config)
	{
		DeviceNode node;
		node.name = name;
		node.deviceNodeType = DeviceNodeType::ConsoleTerminal;
		node.busName = "consolebus";
		node.unit = name.mid(3).toUInt();  // "OPA0" -> 0
		node.enabled = true;
		node.location = config.location;

		// Set classType explicitly
		node.classType = "UART";

		// Copy config properties
		node.properties["iface"] = config.iface;
		node.properties["iface_port"] = config.ifacePort;
		node.properties["application"] = config.application;

		m_nodes[name] = node;
	}
    
	void registerController(const QString& name, const ControllerConfig& config)
	{
		DeviceNode node;
		node.name = name;
		node.deviceNodeType = classTypeToDeviceType(config.classType);
		node.busName = "pci0";
		node.enabled = true;

		// Set classType from config
		node.classType = config.classType;  // "SCSI_HBA", "NIC", etc.

		// Copy all fields
		for (auto it = config.fields.begin(); it != config.fields.end(); ++it) {
			node.properties[it.key()] = it.value();
		}

		m_nodes[name] = node;
	}
    
	void registerDevice(const QString& name, const DeviceConfig& config)
	{
		DeviceNode node;
		node.name = name;
		node.deviceNodeType = deviceTypeFromString(config.classType);
		node.parent = config.parent;
		node.unit = config.fields.value("unit", 0).toUInt();
		node.enabled = true;

		// Set classType from config (this is what shows in SHOW DEVICE)
		node.classType = config.classType;  // "SCSI_DISK", "SCSI_TAPE", etc.

		// Copy all fields
		for (auto it = config.fields.begin(); it != config.fields.end(); ++it) {
			node.properties[it.key()] = it.value();
		}

		m_nodes[name] = node;
	}
    
    // ========================================================================
    // Validation and Binding
    // ========================================================================
    
    bool validateAddressSpace()
    {
        // Check for MMIO overlaps
        QMap<quint64, QString> mmioMap;
        
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
            const auto& node = it.value();
            if (node.mmioBase != 0 && node.mmioSize != 0) {
                // Check overlap
                for (quint64 addr = node.mmioBase; 
                     addr < node.mmioBase + node.mmioSize; 
                     addr += 4096)  // Page granularity
                {
                    if (mmioMap.contains(addr)) {
                        ERROR_LOG(QString("MMIO overlap: %1 and %2 at 0x%3")
                            .arg(node.name)
                            .arg(mmioMap[addr])
                            .arg(addr, 16, 16, QChar('0')));
                        return false;
                    }
                    mmioMap[addr] = node.name;
                }
            }
        }
        
        return true;
    }
    
    void bindConsoleServices()
    {
        // Bind OPA devices to console manager
        // This is done in EmulatR_init Phase 13
        INFO_LOG("Console services binding deferred to Phase 13");
    }
    
    // ========================================================================
    // Type Conversion Helpers
    // ========================================================================
    
    static DeviceNodeType classTypeToDeviceType(const QString& classType)
    {
        if (classType == "SCSI_HBA") return DeviceNodeType::SCSIController;
        if (classType == "IDE_CONTROLLER") return DeviceNodeType::IDEController;
        if (classType == "NIC") return DeviceNodeType::NetworkInterface;
        return DeviceNodeType::Unknown;
    }
    
    static DeviceNodeType deviceTypeFromString(const QString& typeStr)
    {
        if (typeStr == "SCSI_DISK") return DeviceNodeType::SCSIDisk;
        if (typeStr == "SCSI_TAPE") return DeviceNodeType::SCSITape;
        if (typeStr == "IDE_DISK") return DeviceNodeType::IDEDisk;
        if (typeStr == "UART") return DeviceNodeType::ConsoleTerminal;
        return DeviceNodeType::Unknown;
    }
    
    static QString deviceTypeToString(DeviceNodeType type)
    {
        switch (type) {
        case DeviceNodeType::PlatformRoot: return "Platform";
        case DeviceNodeType::SystemBus: return "SystemBus";
        case DeviceNodeType::PCIBus: return "PCIBus";
        case DeviceNodeType::ConsoleBus: return "ConsoleBus";
        case DeviceNodeType::VirtualBus: return "VirtualBus";
        case DeviceNodeType::SCSIController: return "SCSI_HBA";
        case DeviceNodeType::SCSIDisk: return "SCSI_DISK";
        case DeviceNodeType::SCSITape: return "SCSI_TAPE";
        case DeviceNodeType::IDEController: return "IDE_CONTROLLER";
        case DeviceNodeType::IDEDisk: return "IDE_DISK";
        case DeviceNodeType::NetworkInterface: return "NIC";
        case DeviceNodeType::ConsoleTerminal: return "UART";
        default: return "Unknown";
        }
    }

    // ========================================================================
    // Member Variables
    // ========================================================================
    
    EmulatorSettings m_config;
    QMap<QString, DeviceNode> m_nodes;
    bool m_initialized{ false };
};

#endif // FIRMWAREDEVICEMANAGER_H
