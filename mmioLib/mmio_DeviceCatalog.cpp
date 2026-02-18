#include "mmio_DeviceCatalog.h"
#include "coreLib/LoggingMacros.h"         
#include "coreLib/Axp_Attributes_core.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

DeviceCatalog::DeviceCatalog()
{
	// Empty catalog, call initializeBuiltins() to populate
}

// ============================================================================
// CATALOG MANAGEMENT
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::initializeBuiltins()
{
	addBuiltinScsiTemplates();
	addBuiltinNicTemplates();
	addBuiltinUartTemplates();
	addBuiltinIdeTemplates();
	addBuiltinBridgeTemplates();
	addBuiltinClassDefaults();
}

AXP_HOT AXP_FLATTEN void DeviceCatalog::addTemplate(const QString& key, const DeviceTemplate& tmpl)
{
	m_templates[key] = tmpl;
}

AXP_HOT AXP_FLATTEN bool DeviceCatalog::hasTemplate(const QString& key) const
{
	return m_templates.contains(key);
}

AXP_HOT AXP_FLATTEN const DeviceTemplate* DeviceCatalog::getTemplate(const QString& key) const
{
	auto it = m_templates.find(key);
	return (it != m_templates.end()) ? &it.value() : nullptr;
}

AXP_HOT AXP_FLATTEN void DeviceCatalog::removeTemplate(const QString& key)
{
	m_templates.remove(key);
}

AXP_HOT AXP_FLATTEN void DeviceCatalog::clear()
{
	m_templates.clear();
}

// ============================================================================
// LOOKUP HELPERS
// ============================================================================

AXP_HOT AXP_FLATTEN const DeviceTemplate* DeviceCatalog::lookupByPciId(quint16 vendorId, quint16 deviceId) const
{
	QString key = makePciKey(vendorId, deviceId);
	return getTemplate(key);
}

AXP_HOT AXP_FLATTEN const DeviceTemplate* DeviceCatalog::lookupByPciSubsystemId(
	quint16 vendorId, quint16 deviceId,
	quint16 subsysVendorId, quint16 subsysDeviceId) const
{
	QString key = makePciSubsysKey(vendorId, deviceId, subsysVendorId, subsysDeviceId);
	return getTemplate(key);
}

const DeviceTemplate* DeviceCatalog::lookupByClass(mmio_DeviceClass cls) const
{
	QString key = makeClassKey(cls);
	return getTemplate(key);
}

// ============================================================================
// INTROSPECTION
// ============================================================================

AXP_HOT AXP_FLATTEN QVector<QString> DeviceCatalog::getAllKeys() const
{
	return m_templates.keys().toVector();
}

AXP_HOT AXP_FLATTEN QVector<QString> DeviceCatalog::getTemplatesByClass(mmio_DeviceClass cls) const
{
	QVector<QString> result;

	for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
		if (it.value().deviceClass == cls) {
			result.append(it.key());
		}
	}

	return result;
}

AXP_HOT AXP_FLATTEN DeviceCatalog::Statistics DeviceCatalog::getStatistics() const
{
	Statistics stats;
	stats.totalTemplates = m_templates.size();
	stats.pciDeviceTemplates = 0;
	stats.classDefaultTemplates = 0;
	stats.customTemplates = 0;

	for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
		const QString& key = it.key();

		if (key.startsWith("pci_")) {
			stats.pciDeviceTemplates++;
		}
		else if (key.startsWith("generic_")) {
			stats.classDefaultTemplates++;
		}
		else {
			stats.customTemplates++;
		}
	}

	return stats;
}

AXP_HOT AXP_FLATTEN QString DeviceCatalog::dump() const
{
	QStringList lines;
	lines << "=== Device Catalog ===";
	lines << QString("Total templates: %1").arg(m_templates.size());
	lines << "";

	// Group by type
	QStringList pciTemplates;
	QStringList classTemplates;
	QStringList customTemplates;

	for (auto it = m_templates.begin(); it != m_templates.end(); ++it) {
		const QString& key = it.key();
		const DeviceTemplate& tmpl = it.value();

		QString line = QString("%1: %2 (BARs=%3, IRQs=%4)")
			.arg(key)
			.arg(tmpl.displayName)
			.arg(tmpl.bars.size())
			.arg(tmpl.irqs.size());

		if (key.startsWith("pci_")) {
			pciTemplates << line;
		}
		else if (key.startsWith("generic_")) {
			classTemplates << line;
		}
		else {
			customTemplates << line;
		}
	}

	if (!pciTemplates.isEmpty()) {
		lines << "PCI Device Templates:";
		lines << pciTemplates;
		lines << "";
	}

	if (!classTemplates.isEmpty()) {
		lines << "Class Default Templates:";
		lines << classTemplates;
		lines << "";
	}

	if (!customTemplates.isEmpty()) {
		lines << "Custom Templates:";
		lines << customTemplates;
		lines << "";
	}

	return lines.join('\n');
}

// ============================================================================
// KEY GENERATION HELPERS
// ============================================================================

QString DeviceCatalog::makePciKey(quint16 vendorId, quint16 deviceId)
{
	return QString("pci_%1_%2")
		.arg(vendorId, 4, 16, QChar('0'))
		.arg(deviceId, 4, 16, QChar('0'));
}

AXP_HOT AXP_FLATTEN QString DeviceCatalog::makePciSubsysKey(quint16 vendorId, quint16 deviceId,
	quint16 subsysVendorId, quint16 subsysDeviceId)
{
	return QString("pci_%1_%2_%3_%4")
		.arg(vendorId, 4, 16, QChar('0'))
		.arg(deviceId, 4, 16, QChar('0'))
		.arg(subsysVendorId, 4, 16, QChar('0'))
		.arg(subsysDeviceId, 4, 16, QChar('0'));
}

AXP_HOT AXP_FLATTEN QString DeviceCatalog::makeClassKey(mmio_DeviceClass cls)
{
	switch (cls) {
	case mmio_DeviceClass::SCSI_CONTROLLER: return "generic_scsi_hba";
	case mmio_DeviceClass::SCSI_DISK:       return "generic_scsi_disk";
	case mmio_DeviceClass::SCSI_TAPE:       return "generic_scsi_tape";
	case mmio_DeviceClass::SCSI_CDROM:      return "generic_scsi_cdrom";
	case mmio_DeviceClass::IDE_CONTROLLER:  return "generic_ide_controller";
	case mmio_DeviceClass::IDE_DISK:        return "generic_ide_disk";
	case mmio_DeviceClass::IDE_CDROM:       return "generic_ide_cdrom";
	case mmio_DeviceClass::NIC:             return "generic_nic";
	case mmio_DeviceClass::UART:            return "generic_uart";
	case mmio_DeviceClass::BRIDGE:          return "generic_bridge";
	default:                           return QString();
	}
}

// ============================================================================
// BUILT-IN SCSI TEMPLATES
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinScsiTemplates()
{
	// QLogic ISP1020 SCSI HBA
	DeviceTemplate qlogic1020;
	qlogic1020.displayName = "QLogic ISP1020";
	qlogic1020.vendorName = "QLogic";
	qlogic1020.deviceClass = mmio_DeviceClass::SCSI_CONTROLLER;

	// Create BAR template
	BarTemplate bar0;
	bar0.barIndex = 0;
	bar0.name = "registers";
	bar0.size = 256;
	bar0.minAlignment = 256;
	bar0.is64Bit = false;
	bar0.prefetchable = false;
	bar0.allowedWidths = 0x0C;  // 4/8-byte only
	bar0.stronglyOrdered = true;
	bar0.sideEffectOnRead = true;
	bar0.sideEffectOnWrite = true;
	bar0.regEndian = mmio_Endianness::LITTLE;

	qlogic1020.bars.append(bar0);

	// Create IRQ template
	IrqTemplate irq0;
	irq0.trigger = IrqTriggerMode::Level;
	irq0.ipl = IrqIPL::DEVICE_20;

	qlogic1020.irqs.append(irq0);

	// DMA capabilities
	qlogic1020.dmaCaps.supported = true;
	qlogic1020.dmaCaps.addressingBits = 32;
	qlogic1020.dmaCaps.dmaMask = 0xFFFFFFFF;
	qlogic1020.dmaCaps.coherent = false;
	qlogic1020.dmaCaps.needsDoorbellFence = false;

	qlogic1020.exposeWhenDegraded = false;

	addTemplate(makePciKey(0x1077, 0x1020), qlogic1020);
}

// ============================================================================
// BUILT-IN NIC TEMPLATES
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinNicTemplates()
{
	// DEC 21143 Tulip
	DeviceTemplate tulip;
	tulip.displayName = "DEC 21143 Tulip";
	tulip.vendorName = "Digital Equipment Corporation";
	tulip.deviceClass = mmio_DeviceClass::NIC;

	// BAR 0 - I/O registers
	BarTemplate bar0;
	bar0.barIndex = 0;
	bar0.name = "io_regs";
	bar0.size = 128;
	bar0.minAlignment = 128;
	bar0.is64Bit = false;
	bar0.prefetchable = false;
	bar0.allowedWidths = 0x0F;  // All widths
	bar0.stronglyOrdered = true;
	bar0.sideEffectOnRead = false;
	bar0.sideEffectOnWrite = true;
	bar0.regEndian = mmio_Endianness::LITTLE;
	tulip.bars.append(bar0);

	// BAR 1 - Memory-mapped registers
	BarTemplate bar1;
	bar1.barIndex = 1;
	bar1.name = "mem_regs";
	bar1.size = 4096;
	bar1.minAlignment = 4096;
	bar1.is64Bit = false;
	bar1.prefetchable = false;
	bar1.allowedWidths = 0x0C;  // 4/8-byte
	bar1.stronglyOrdered = false;
	bar1.sideEffectOnRead = false;
	bar1.sideEffectOnWrite = false;
	bar1.regEndian = mmio_Endianness::LITTLE;
	tulip.bars.append(bar1);

	// IRQ
	IrqTemplate irq0;
	irq0.trigger = IrqTriggerMode::Level;
	irq0.ipl = IrqIPL::DEVICE_20;
	tulip.irqs.append(irq0);

	// DMA
	tulip.dmaCaps.supported = true;
	tulip.dmaCaps.addressingBits = 32;
	tulip.dmaCaps.dmaMask = 0xFFFFFFFF;
	tulip.dmaCaps.coherent = false;
	tulip.dmaCaps.needsDoorbellFence = true;

	tulip.exposeWhenDegraded = false;

	addTemplate(makePciKey(0x1011, 0x0019), tulip);
}

// ============================================================================
// BUILT-IN UART TEMPLATES (stub - implement as needed)
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinUartTemplates()
{
	// TODO: Add specific UART device templates if needed
	// Most UARTs use the generic class default
}

// ============================================================================
// BUILT-IN IDE TEMPLATES (stub - implement as needed)
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinIdeTemplates()
{
	// TODO: Add specific IDE controller templates if needed
	// Most IDE controllers use the generic class default
}

// ============================================================================
// BUILT-IN BRIDGE TEMPLATES (stub - implement as needed)
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinBridgeTemplates()
{
	// TODO: Add specific bridge templates if needed
}

// ============================================================================
// BUILT-IN CLASS DEFAULTS
// ============================================================================

AXP_HOT AXP_FLATTEN void DeviceCatalog::addBuiltinClassDefaults()
{
	// Generic SCSI HBA
	{
		DeviceTemplate generic;
		generic.displayName = "Generic SCSI HBA";
		generic.deviceClass = mmio_DeviceClass::SCSI_CONTROLLER;

		BarTemplate bar0;
		bar0.barIndex = 0;
		bar0.name = "registers";
		bar0.size = 4096;
		bar0.minAlignment = 4096;
		bar0.is64Bit = false;
		bar0.prefetchable = false;
		bar0.allowedWidths = 0x0C;
		bar0.stronglyOrdered = true;
		bar0.sideEffectOnRead = false;
		bar0.sideEffectOnWrite = true;
		bar0.regEndian = mmio_Endianness::LITTLE;
		generic.bars.append(bar0);

		IrqTemplate irq0;
		irq0.trigger = IrqTriggerMode::Level;
		irq0.ipl = IrqIPL::DEVICE_20;
		generic.irqs.append(irq0);

		generic.dmaCaps.supported = true;
		generic.dmaCaps.addressingBits = 32;
		generic.dmaCaps.dmaMask = 0xFFFFFFFF;
		generic.dmaCaps.coherent = false;
		generic.dmaCaps.needsDoorbellFence = false;
		generic.exposeWhenDegraded = false;

		addTemplate("generic_scsi_hba", generic);
	}

	// Generic NIC
	{
		DeviceTemplate generic;
		generic.displayName = "Generic NIC";
		generic.deviceClass = mmio_DeviceClass::NIC;

		BarTemplate bar0;
		bar0.barIndex = 0;
		bar0.name = "registers";
		bar0.size = 4096;
		bar0.minAlignment = 4096;
		bar0.is64Bit = false;
		bar0.prefetchable = false;
		bar0.allowedWidths = 0x0F;
		bar0.stronglyOrdered = false;
		bar0.sideEffectOnRead = false;
		bar0.sideEffectOnWrite = true;
		bar0.regEndian = mmio_Endianness::LITTLE;
		generic.bars.append(bar0);

		IrqTemplate irq0;
		// irq0.purpose = "primary";
		// irq0.defaultIRQIpl = 21;
		// irq0.trigger = IRQTrigger::LEVEL;
		// irq0.shareable = false;
		irq0.trigger = IrqTriggerMode::Level;
		irq0.ipl = IrqIPL::DEVICE_20;
		generic.irqs.append(irq0);

		generic.dmaCaps.supported = true;
		generic.dmaCaps.addressingBits = 32;
		generic.dmaCaps.dmaMask = 0xFFFFFFFF;
		generic.dmaCaps.coherent = false;
		generic.dmaCaps.needsDoorbellFence = true;
		generic.exposeWhenDegraded = false;

		addTemplate("generic_nic", generic);
	}

	// Generic UART
	{
		DeviceTemplate generic;
		generic.displayName = "Generic UART";
		generic.deviceClass = mmio_DeviceClass::UART;

		BarTemplate bar0;
		bar0.barIndex = 0;
		bar0.name = "registers";
		bar0.size = 8;  // 16550-style
		bar0.minAlignment = 8;
		bar0.is64Bit = false;
		bar0.prefetchable = false;
		bar0.allowedWidths = 0x01;  // Byte-only
		bar0.stronglyOrdered = true;
		bar0.sideEffectOnRead = true;
		bar0.sideEffectOnWrite = true;
		bar0.regEndian = mmio_Endianness::LITTLE;
		generic.bars.append(bar0);

		IrqTemplate irq0;
		irq0.trigger = IrqTriggerMode::Level;
		irq0.ipl = IrqIPL::DEVICE_20;
		generic.irqs.append(irq0);

		generic.dmaCaps.supported = false;
		generic.dmaCaps.addressingBits = 0;
		generic.dmaCaps.dmaMask = 0;
		generic.dmaCaps.coherent = false;
		generic.dmaCaps.needsDoorbellFence = false;
		generic.exposeWhenDegraded = true;  // Console should be visible

		addTemplate("generic_uart", generic);
	}
}

/*
 *
 *for (auto& irq : device.irqs) {
    irq.assignedSourceId = m_router->registerDevice(
        irq.ipl, irq.vector, irq.trigger, irq.routing, irq.affinityCpu);
}
 */

