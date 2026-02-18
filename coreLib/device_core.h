#pragma once
#include <QtGlobal>
#include <QByteArray>
#include <QString>
#include "mmio_core.h"
#include "IRQ_core.h"
#include "DMA_core.h"
#include <QVector>
#include <QHash>
#include <QDebug>
#include <variant>
#include <optional>
#include <QSet>

// ============================================================================
// BOOT CONFIGURATION (determines critical devices)
// ============================================================================

/**
 * @brief Boot configuration loaded from INI or firmware.
 *
 * Populated during Phase 0 (INI parse).
 */
 // ============================================================================
 // BOOT CONFIGURATION (determines critical devices)
 // ============================================================================


struct BootConfiguration {
	// Boot device (disk that contains OS)
	QString bootDeviceName;        // e.g., "DKA0"
	QString bootControllerName;    // e.g., "PKA0"

	// Console device (primary user interface)
	QString consoleDeviceName;     // e.g., "OPA0"

	// Explicit criticality overrides (by device name)
	QSet<QString> criticalDevices;   // Must succeed or abort boot
	QSet<QString> importantDevices;  // Warn if fails, continue
	QSet<QString> optionalDevices;   // Silent if fails

	// Network boot settings (optional)
	bool networkBootEnabled = false;
	QString networkBootDevice;     // e.g., "EWA0"
	QString primaryNICName;        // e.g., "EWA0"

	// Diagnostic settings
	bool verboseAllocation = false;
	bool stopOnFirstFailure = false;
};

// ============================================================================
// DEVICE-SPECIFIC PAYLOADS (typed, not QVariantMap)
// ============================================================================

// SCSI child payload
struct SCSIChildPayload {
	QString parentControllerName; // "PKA0"
	quint8 targetId;              // 0-7 (narrow) or 0-15 (wide)
	quint8 lun;                   // 0-7 typical
	QString imageFile;            // Backing store
	QString serial;               // Device serial number
	quint64 imageSize = 0;        // Bytes (validated during late phase)
	bool readOnly;				  // Payload is readonly
};

// NIC payload
struct NICPayload {
	QByteArray macAddress;        // 6 bytes, canonical form
	QString linkMode;             // "auto", "100/full", etc.
	quint16 mtu = 1500;           // Default Ethernet MTU
};

// HBA payload
struct HBAPayload {
	quint8 hostAdapterId = 7;     // SCSI ID of controller itself
	quint16 queueDepth = 32;      // Command queue depth
	quint8 maxTarget = 7;         // 7 (narrow) or 15 (wide)
	quint8 maxLun = 7;            // Typical; some devices support 255
	bool wideMode = false;        // 8-bit (narrow) vs 16-bit (wide) SCSI
	bool terminationEnabled = true; // Logical termination
};

// keep a single alias :
using DevicePayload = std::variant<std::monostate, SCSIChildPayload, NICPayload, HBAPayload>;
// Tagged union for device-specific data
// struct DevicePayload {
// 	enum class Type : quint8 { NONE, SCSI_CHILD, NIC, HBA } type = Type::NONE;
// 
// 	union {
// 		SCSIChildPayload scsiChild;
// 		NICPayload nic;
// 		HBAPayload hba;
// 	};
// 
// 	DevicePayload() : type(Type::NONE) {}
// 	~DevicePayload() {}
// };

	// Helper to check payload type
template<typename T>
bool hasPayload(const DevicePayload& payload) {
	return std::holds_alternative<T>(payload);
}
// Helper to check payload type

// Example usage:
// if (hasPayload<HBAPayload>(node.payload)) {
//     auto& hba = std::get<HBAPayload>(node.payload);
//     quint8 maxTarget = hba.maxTarget;
// }

// ============================================================================
// PCI IDENTITY (for PCI_FUNCTION nodes)
// ============================================================================

struct PCIIdentity {
	quint16 vendorId;
	quint32 deviceId;
	quint8 revision;
	quint32 classCode;  // Base class, subclass, prog IF
};



// ============================================================================
// NODE KIND (now a bitmask for overlapping roles)
// ============================================================================

enum NodeKindFlags : quint16 {
	NODE_NONE = 0x0000,
	NODE_HOST_BRIDGE = 0x0001,  // Hose/IO Hub
	NODE_PCI_FUNCTION = 0x0002,  // Physical PCI BDF
	NODE_CONTROLLER = 0x0004,  // Functional controller (HBA, NIC)
	NODE_CHILD_DEVICE = 0x0008,  // Behind controller (disk, tape)
};

// Allow bitwise operations
inline NodeKindFlags operator|(NodeKindFlags a, NodeKindFlags b) {
	return static_cast<NodeKindFlags>(static_cast<quint16>(a) | static_cast<quint16>(b));
}

inline NodeKindFlags operator&(NodeKindFlags a, NodeKindFlags b) {
	return static_cast<NodeKindFlags>(static_cast<quint16>(a) & static_cast<quint16>(b));
}

inline NodeKindFlags& operator^=(NodeKindFlags& a, NodeKindFlags b) {
	a = static_cast<NodeKindFlags>(
		static_cast<quint16>(a) ^ static_cast<quint16>(b));
	return a;
}
inline NodeKindFlags operator~(NodeKindFlags a) {
	return static_cast<NodeKindFlags>(~static_cast<quint16>(a));
}

// Helpers
// inline bool hasKind(NodeKindFlags flags, NodeKindFlags test) {
// 	return (flags & test) != NODE_NONE;
// }

inline bool hasKind(NodeKindFlags flags, NodeKindFlags test) {
	return (static_cast<quint16>(flags) & static_cast<quint16>(test)) != 0;
}

// Example usage:
// DeviceNode pka0;
// pka0.nodeKind = NODE_PCI_FUNCTION | NODE_CONTROLLER; // Both roles
// 
// if (hasKind(pka0.nodeKind, NODE_CONTROLLER)) {
//     // Handle controller logic
// }



struct DeviceNode {
	// Identity
	quint32      uid = 0;              // DeviceUIDAllocator (1+, 0 invalid)
	QString      name;                 // "PKA0", "EWA0" (used in logs/reports)
	QString      location;             // "hose H @ bus:slot.func" (filled Phase 1)
	mmio_DeviceClass  deviceClass = mmio_DeviceClass::INVALID;  // Controller/child class
	QString      templateId;           // Optional explicit override (pre-Phase 2)
	QString      resolvedTemplateId;   // Set by allocator on successful lookup
	bool		 m_bIsRoot;		       // device emulator for root devices (controllers)
	bool		 isRoot() { return m_bIsRoot; }
	/*DeviceClass deviceClass() { return m_deviceClass;  }*/


	// Topology
	quint16      hoseId = 0;           // PCI hose/domain
	struct PciAddr { quint8 bus = 0, slot = 0, func = 0; } pci;  // For location formatting

	// Optional PCI identity (used for template lookup)
	struct PciId {
		quint16 vendorId = 0, deviceId = 0;
		quint16 subsysVendorId = 0, subsysDeviceId = 0;
		quint8  revisionId = 0;
	};
	std::optional<PciId> pciId;        // nullopt for non-PCI devices  

	// Hierarchy
	DeviceNode* parent = nullptr;
	QVector<DeviceNode*>    children;

	// Phase 2 results (allocation)
	QVector<MMIOWindow>     mmioWindows;   // Allocated BARs (preserve barIndex)  
	QHash<QString, int>      barAliasToIndex; // Optional: "registers"->0, "buffers"->1
	QVector<IRQDescriptor>  irqs;          // Allocated vectors (hose-scoped)      

	// Device capabilities/config
	DMACapabilities dmaCaps;             // Addressing mask, coherency, doorbell   
	/*	using Payload = std::variant<std::monostate, SCSIChildPayload, NICPayload, HBAPayload>;*/
	DevicePayload devicePayload;                     // Backend config (image path, MAC, etc.)  

	// Lifecycle (shared type)
	LifecycleState lifecycle;            // enabled/probed/started                 

};


// ============================================================================
// DEVICE CRITICALITY (for resource allocation failure handling)
// ============================================================================

enum class DeviceCriticality : quint8 {
	CRITICAL,       // Boot fails if device cannot be initialized
	NON_CRITICAL,   // Boot continues with degraded functionality
	UNKNOWN,        // Not yet classified (treat as NON_CRITICAL)
	OPTIONAL_,
	IMPORTANT
};

/**
 * @brief Determine device criticality based on role and boot path.
 * @param node Device node
 * @param bootConfig Boot configuration (console device, boot device path, network boot flag)
 * @return Criticality level
 */
inline DeviceCriticality classifyDeviceCriticality(
	const DeviceNode* node,
	const BootConfiguration& bootConfig)
{
	Q_ASSERT(node);

	// ========================================================================
	// ALWAYS CRITICAL
	// ========================================================================

	// 1. Console UART (OPA0) - required for firmware/boot diagnostics
	if (node->deviceClass == mmio_DeviceClass::UART_CONSOLE) {
		return DeviceCriticality::CRITICAL;
	}

	// 2. Boot device controller (e.g., PKA0 for SCSI boot)
	if (node->name == bootConfig.bootControllerName) {
		return DeviceCriticality::CRITICAL;
	}

	// 3. Boot device itself (e.g., DKA0 - disk behind PKA0)
	if (node->name == bootConfig.bootDeviceName) {
		return DeviceCriticality::CRITICAL;
	}

	// 4. Primary NIC (if network boot requested)
	if (bootConfig.networkBootEnabled &&
		node->name == bootConfig.primaryNICName) {
		return DeviceCriticality::CRITICAL;
	}

	// ========================================================================
	// NON-CRITICAL (can be disabled with degraded functionality)
	// ========================================================================

	// All other devices:
	// - Secondary HBAs/NICs (PKB0, EWA1, etc.)
	// - Tape drives (MKA0)
	// - CD-ROM drives (not boot path)
	// - IDE controllers (unless boot device)
	// - Optional peripherals

	return DeviceCriticality::NON_CRITICAL;
}
