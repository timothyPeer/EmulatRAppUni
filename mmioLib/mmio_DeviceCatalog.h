#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include "mmio_deviceTemplate.h"



// ============================================================================
// DEVICE CATALOG (TEMPLATE DATABASE)
// ============================================================================

class DeviceCatalog {
public:
	// Constructor
	DeviceCatalog();

	// Destructor
	~DeviceCatalog() = default;

	// ========================================================================
	// CATALOG MANAGEMENT
	// ========================================================================

	// Initialize with built-in templates
	void initializeBuiltins();

	// Add custom template
	void addTemplate(const QString& key, const DeviceTemplate& tmpl);

	// Check if template exists
	bool hasTemplate(const QString& key) const;

	// Get template by key (returns nullptr if not found)
	const DeviceTemplate* getTemplate(const QString& key) const;

	// Remove template
	void removeTemplate(const QString& key);

	// Clear all templates
	void clear();

	// ========================================================================
	// LOOKUP HELPERS
	// ========================================================================

	// Lookup by PCI ID (vendor:device)
	const DeviceTemplate* lookupByPciId(quint16 vendorId, quint16 deviceId) const;

	// Lookup by PCI subsystem ID (vendor:device:subsys_vendor:subsys_device)
	const DeviceTemplate* lookupByPciSubsystemId(quint16 vendorId,
		quint16 deviceId,
		quint16 subsysVendorId,
		quint16 subsysDeviceId) const;

	// Lookup by device class (fallback)
	const DeviceTemplate* lookupByClass(mmio_DeviceClass cls) const;

	// ========================================================================
	// INTROSPECTION
	// ========================================================================

	// Get all template keys
	QVector<QString> getAllKeys() const;

	// Get templates by device class
	QVector<QString> getTemplatesByClass(mmio_DeviceClass cls) const;

	// Get catalog statistics
	struct Statistics {
		int totalTemplates;
		int pciDeviceTemplates;
		int classDefaultTemplates;
		int customTemplates;
	};

	Statistics getStatistics() const;

	// Dump catalog to string (for debugging)
	QString dump() const;
	int size() {
		return m_templates.size();
	}

	// ========================================================================
	// KEY GENERATION HELPERS
	// ========================================================================

	static QString makePciKey(quint16 vendorId, quint16 deviceId);
	static QString makePciSubsysKey(quint16 vendorId, quint16 deviceId,
		quint16 subsysVendorId, quint16 subsysDeviceId);
	static QString makeClassKey(mmio_DeviceClass cls);

private:
	// ========================================================================
	// BUILT-IN TEMPLATE CREATION
	// ========================================================================

	void addBuiltinScsiTemplates();
	void addBuiltinNicTemplates();
	void addBuiltinUartTemplates();
	void addBuiltinIdeTemplates();
	void addBuiltinBridgeTemplates();
	void addBuiltinClassDefaults();



	// ========================================================================
	// PRIVATE MEMBERS
	// ========================================================================

	// Template database (key -> template)
	QHash<QString, DeviceTemplate> m_templates;
};
