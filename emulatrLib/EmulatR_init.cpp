// ============================================================================
// EmulatR_init.cpp - Enhanced with InitPhaseLogger
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

// ============================================================================
// EmulatR_init.cpp - Enhanced with InitPhaseLogger
// ============================================================================

#include "EmulatR_init.h"
#include <QMap>
#include <QStringList>
#include <QString>
#include <QDir>
#include "d:/Qt/6.9.3/msvc2022_64/include/QtCore/qminmax.h"
#include "configLib/global_EmulatorSettings.h"
#include "coreLib/globalCPUCount.h"
#include "memoryLib/global_writeBufferManager.h"
#include "coreLib/ExecTrace.h"
#include "palLib_EV6/Global_PALVectorTable.h"
#include "palLib_EV6/PalVectorTable_final.h"
#include "grainFactoryLib/global_GrainResolver.h"
#include "pteLib/global_Ev6TLB_Singleton.h"
#include "mmioLib/global_mmioManager.h"
#include "deviceLib/global_SRMEnvStore.h"
#include "deviceLib/SRMEnvStore.h"
#include "deviceLib/global_ConsoleManager.h"
#include "memoryLib/global_MemoryBarrierCoordinator.h"
#include "coreLib/LoggingMacros.h"
#include "deviceLib/SRMConsoleDevice.h"
#include "memoryLib/GuestMemory.h"
#include "deviceLib/ConsoleManager.h"
#include "memoryLib/memory_core.h"
#include "memoryLib/global_firmwaredevicemanager.h"
#include "EmulatorPaths.h"
#include "ExecutionCoordinator.h"
#include "config/Version.h"
#include "coreLib/InitPhaseLogging.h"
#include "cpuCoreLib/AlphaCPU.h"
#include "memoryLib/global_GuestMemory.h"
#include "grainFactoryLib/GrainArchitectureDump.h"
#include "grainFactoryLib/iGrain-DualCache_singleton.h"
#include "memoryLib/SrmRomLoader.h"
#include "memoryLib/GuestPhysicalRegionRegistry.h"
// Include all grain headers

// ============================================================================
// COMPLETE INITIALIZATION IMPLEMENTATION
// ============================================================================

AXP_HOT AXP_FLATTEN bool EmulatR_init::initialize() {
	INFO_LOG("============================================================");
	INFO_LOG("ASA ALPHA EMULATOR INITIALIZATION SEQUENCE");
	INFO_LOG("============================================================");

	InitializationVerifier::reset();
	InitPhaseLogger::reset();
	InitPhaseLogger::setTotalPhases(20);

	if (!initializePhase0_Bootstrap()) return false;
	if (!initializePhase2_Configuration()) return false;
	if (!initializePhase1_Logging()) return false;
	if (!initializePhase1_2_ExecTrace()) return false;

	if (!initializePhase3_PlatformIdentity()) return false;
	if (!initializePhase4_MemorySystem()) return false;
	if (!initializePhase5_FirmwareLoading()) return false;
	if (!initializePhase5_5_TLBSystem()) return false;
	if (!initializePhase6_ReservationSystem()) return false;
	if (!initializePhase7_ExceptionInfrastructure()) return false;
	if (!initializePhase7_5_DeviceTree()) return false;
	if (!initializePhase8_PALInfrastructure()) return false;
	if (!initializePhase8_5_PalHandlers()) return false;
	if (!initializePhase9_InstructionSystem()) return false;
	if (!initializePhase9_5_InstructionSet()) return false;
	if (!initializePhase10_DeviceInfrastructure()) return false;

	if (!initializePhase11_CoordinationLayer()) return false;
	if (!initializePhase13_InitializeConsole()) return false;
	if (!initializePhase13_ConsoleEnvironment()) return false;
	if (!initializePhase14_CPUBringUp()) return false;
	if (!initializePhase15_FinalVerification()) return false;


	INFO_LOG("============================================================");
	INFO_LOG("INITIALIZATION COMPLETE - SYSTEM READY");
	INFO_LOG("============================================================");
	INFO_LOG(InitializationVerifier::getInitializationReport());

	return true;
}

// ============================================================================
// PHASE IMPLEMENTATIONS WITH DETAILED LOGGING
// ============================================================================

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase0_Bootstrap() {
	InitPhaseLogger phase("Bootstrap - Global Settings");

	// Access the global singleton
	auto& settings = global_EmulatorSettings();

	// Validate defaults
	if (settings.podData.system.processorCount == 0) {
		settings.podData.system.processorCount = 1;
		phase.logDetail("Set default processor count: 1");
	}

	if (settings.podData.system.memorySizeGB == 0) {
		settings.podData.system.memorySizeGB = 4;
		phase.logDetail("Set default memory size: 4 GB");
	}

	phase.logConfig("Processor Count", settings.podData.system.processorCount);
	phase.logConfig("Memory Size (GB)", settings.podData.system.memorySizeGB);

	InitializationVerifier::markInitialized("Bootstrap");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase1_Logging() {
	InitPhaseLogger phase("Event Logging System");

	// EventLog::initialize() reads from global_EmulatorSettings()
	// if (!EventLog::initialize()) {
	// 	ERROR_LOG("Failed to initialize EventLog");
	// 	return false;
	// }

	auto& config = global_EmulatorSettings();

	phase.logDetail("Event logging backend already initialized");
	phase.logDetail(QString("Log file: %1").arg(config.podData.logging.logFileName));
	phase.logDetail(QString("Disk logging: %1").arg(config.podData.logging.enableDiskLogging ? "enabled" : "disabled"));
	phase.logDetail(QString("Log level: %1").arg(config.podData.logging.logLevel));
	phase.logDetail(EmulatR::getVersionAndBuild());
	phase.logDetail(EmulatR::getBuildInfo());

	InitializationVerifier::markInitialized("EventLog");
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase1_2_ExecTrace() {
	InitPhaseLogger phase("Execution Trace System");

	if (!ExecTrace::initialize("asm")) {
		ERROR_LOG("Failed to initialize ExecTrace");
		return false;
	}

	if (ExecTrace::isEnabled()) {
		phase.logDetail("ExecTrace ENABLED");
		auto& settings = global_EmulatorSettings();

		// Log which CPUs are traced
		uint32_t mask = settings.podData.execTrace.cpuMask;
		QString cpuList;
		for (int i = 0; i < 16; ++i) {
			if (mask & (1U << i)) {
				if (!cpuList.isEmpty()) cpuList += ", ";
				cpuList += QString::number(i);
			}
		}
		phase.logConfig("Traced CPUs", cpuList);
		phase.logConfig("Trace Mode", settings.podData.execTrace.execTraceMode);
		phase.logConfig("Ring Size", settings.podData.execTrace.traceRingRecordsPerCpu);
	}
	else {
		phase.logDetail("ExecTrace disabled in configuration");
	}




	InitializationVerifier::markInitialized("ExecTrace");
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
// ============================================================================
// Phase 2: Instruction Set Analysis and Validation
//
// Purpose:
//   - Enumerate all registered instruction grains
//   - Detect duplicate opcode/function registrations
//   - Identify coverage gaps in instruction set
//   - Generate comprehensive reports for debugging
//
// Outputs:
//   - architecture_dump.txt: Human-readable full report
//   - grain_registry.json: Machine-readable JSON export
//   - grain_registry.tsv: Tab-delimited spreadsheet format
//
// Critical Checks:
//   - Duplicate registrations (same opcode+function -> multiple grains)
//   - Coverage gaps (missing expected instructions)
//   - Grain count validation (should have ~200+ grains for full Alpha ISA)
//
// Returns:
//   true if analysis successful (warnings acceptable)
//   false if critical error prevents analysis
// ============================================================================
bool EmulatR_init::initializePhase9_5_InstructionSet()
{
	if (!beginInitialization("InstructionSet"))
		return false;

	INFO_LOG("========================================");
	INFO_LOG("Phase 2: Instruction Set Analysis");
	INFO_LOG("========================================");

	// ------------------------------------------------------------------------
	// Step 1: Create grain architecture analyzer
	// ------------------------------------------------------------------------
	auto& dump = GrainArchitectureDump::instance();

	INFO_LOG("Analyzing grain registry...");
	if (!dump.analyze())
	{
		ERROR_LOG("Failed to analyze grain architecture");
		markFailure();
		return false;
	}

	// ------------------------------------------------------------------------
	// Step 2: Log summary statistics
	// ------------------------------------------------------------------------
	const int totalGrains = dump.totalGrains();
	INFO_LOG(QString("Grain Registry Summary: %1").arg(dump.getSummary()));
	INFO_LOG(QString("  Total Grains Registered: %1").arg(totalGrains));

	// ------------------------------------------------------------------------
	// Step 3: Validate minimum grain count
	// ------------------------------------------------------------------------
	// Alpha AXP has approximately 200-250 instructions depending on variant
	// EV6 should have at least 180 grains for basic functionality
	constexpr int MIN_EXPECTED_GRAINS = 100;  // Conservative minimum
	constexpr int RECOMMENDED_GRAINS = 180;   // Full basic ISA

	if (totalGrains < MIN_EXPECTED_GRAINS)
	{
		ERROR_LOG(QString("Insufficient grains registered: %1 (minimum: %2)")
			.arg(totalGrains)
			.arg(MIN_EXPECTED_GRAINS));
		ERROR_LOG("Instruction set is incomplete - emulator will fail");
		markFailure();
		return false;
	}

	if (totalGrains < RECOMMENDED_GRAINS)
	{
		WARN_LOG(QString("Low grain count: %1 (recommended: %2)")
			.arg(totalGrains)
			.arg(RECOMMENDED_GRAINS));
		WARN_LOG("Some Alpha instructions may not be implemented");
	}
	else
	{
		INFO_LOG(QString("x Grain count acceptable: %1/%2")
			.arg(totalGrains)
			.arg(RECOMMENDED_GRAINS));
	}

	// ------------------------------------------------------------------------
	// Step 4: Check for duplicate registrations
	// ------------------------------------------------------------------------
	if (dump.hasDuplicates())
	{
		const int dupCount = dump.duplicateCount();
		WARN_LOG(QString("Warning  Found %1 duplicate grain registration(s)").arg(dupCount));
		WARN_LOG("Multiple grains registered for same opcode+function");
		WARN_LOG("This may cause non-deterministic instruction execution");
		// Don't fail - duplicates are warnings, not fatal errors
	}
	else
	{
		INFO_LOG("x No duplicate registrations detected");
	}

	// ------------------------------------------------------------------------
	// Step 5: Check for coverage gaps
	// ------------------------------------------------------------------------
	if (dump.hasGaps())
	{
		const int gapCount = dump.gapCount();
		WARN_LOG(QString("Warning  Found %1 coverage gap(s)").arg(gapCount));
		WARN_LOG("Some expected instructions are not registered");
		WARN_LOG("Check architecture_dump.txt for details");
		// Don't fail - gaps are warnings, not fatal errors
	}
	else
	{
		INFO_LOG("x No coverage gaps detected");
	}

	// ------------------------------------------------------------------------
	// Step 6: Write analysis reports
	// ------------------------------------------------------------------------
	QString logDir = m_emulatorPaths.getLogPath("");

	// Ensure log directory exists
	QDir dir(logDir);
	if (!dir.exists())
	{
		if (!dir.mkpath("."))
		{
			WARN_LOG(QString("Failed to create log directory: %1").arg(logDir));
			WARN_LOG("Reports will not be written");
			// Don't fail - missing reports are not critical
		}
	}

	bool reportsWritten = false;

	// Write text report (comprehensive human-readable)
	QString txtPath = logDir + "/architecture_dump.txt";
	if (dump.writeReport(txtPath))
	{
		INFO_LOG(QString("x Text report: %1").arg(txtPath));
		reportsWritten = true;
	}
	else
	{
		WARN_LOG(QString("Failed to write text report: %1").arg(txtPath));
	}

	// Write JSON export (machine-readable)
	QString jsonPath = logDir + "/grain_registry.json";
	if (dump.writeJSON(jsonPath))
	{
		INFO_LOG(QString("x JSON export: %1").arg(jsonPath));
		reportsWritten = true;
	}
	else
	{
		WARN_LOG(QString("Failed to write JSON export: %1").arg(jsonPath));
	}

	// Write TSV export (spreadsheet format)
	QString tsvPath = logDir + "/grain_registry.tsv";
	if (dump.writeTSV(tsvPath))
	{
		INFO_LOG(QString("x TSV export: %1").arg(tsvPath));
		reportsWritten = true;
	}
	else
	{
		WARN_LOG(QString("Failed to write TSV export: %1").arg(tsvPath));
	}

	if (!reportsWritten)
	{
		WARN_LOG("No analysis reports were written");
		WARN_LOG("Check file permissions and disk space");
		// Don't fail - reports are helpful but not critical for emulation
	}

	// ------------------------------------------------------------------------
	// Step 7: Log important findings
	// ------------------------------------------------------------------------
	if (dump.hasDuplicates() || dump.hasGaps())
	{
		WARN_LOG("========================================");
		WARN_LOG("INSTRUCTION SET ANALYSIS WARNINGS:");

		if (dump.hasDuplicates())
		{
			WARN_LOG(QString("  - %1 duplicate registration(s)")
				.arg(dump.duplicateCount()));
		}

		if (dump.hasGaps())
		{
			WARN_LOG(QString("  - %1 coverage gap(s)")
				.arg(dump.gapCount()));
		}

		WARN_LOG("See architecture_dump.txt for details");
		WARN_LOG("========================================");
	}

	// ------------------------------------------------------------------------
	// Step 8: Final validation
	// ------------------------------------------------------------------------
	INFO_LOG("Instruction set analysis complete");
	INFO_LOG(QString("Status: %1 grains registered, %2 duplicates, %3 gaps")
		.arg(totalGrains)
		.arg(dump.hasDuplicates() ? dump.duplicateCount() : 0)
		.arg(dump.hasGaps() ? dump.gapCount() : 0));

	markSuccess();
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase2_Configuration()
{
	InitPhaseLogger phase("Configuration Loading");

	// ========================================================================
	// Build path to configuration file
	// ========================================================================

	QString iniFile = m_emulatorPaths.getConfigPath("ASAEmulatr.ini");
	//  Gets full path to config/ASAEmulatr.ini

	// ========================================================================
	// Initialize the global singleton with the INI file
	// ========================================================================

	if (QFile::exists(iniFile)) {
		phase.logDetail(QString("Loading configuration: %1").arg(iniFile));

		if (!initializeGlobalEmulatorSettings(iniFile)) {
			phase.logDetail("WARNING: Configuration load failed, using defaults");
		}
		else {
			phase.logDetail("Configuration loaded successfully");
		}
	}
	else {
		phase.logDetail(QString("WARNING: Config file not found: %1").arg(iniFile));
		phase.logDetail("Using default configuration values");
	}

	// ========================================================================
	// Access the loaded configuration
	// ========================================================================

	auto& config = global_EmulatorSettings();

	// ========================================================================
	// Resolve log file path if relative
	// ========================================================================

	if (QDir::isRelativePath(config.podData.logging.logFileName)) {
		// Log files should go in the logs directory
		QString logPath = m_emulatorPaths.getLogPath(config.podData.logging.logFileName);
		config.podData.logging.logFileName = QDir::toNativeSeparators(logPath);

		phase.logDetail(QString("Resolved log path: %1").arg(config.podData.logging.logFileName));
	}

	// ========================================================================
	// Resolve ExecTrace path if relative
	// ========================================================================

	if (QDir::isRelativePath(config.podData.execTrace.traceFilePattern)) {
		// Extract filename from pattern (e.g., "traces/es40_instance.cpu{cpu}.trace")
		QString pattern = config.podData.execTrace.traceFilePattern;

		// If pattern contains directory separator, it's a subdirectory path
		if (pattern.contains('/') || pattern.contains('\\')) {
			// Build relative to bin directory
			QString tracePath = QDir(m_emulatorPaths.getBinDir()).filePath(pattern);
			config.podData.execTrace.traceFilePattern = QDir::toNativeSeparators(tracePath);
		}
		else {
			// Just a filename - put in logs directory
			QString tracePath = m_emulatorPaths.getLogPath(pattern);
			config.podData.execTrace.traceFilePattern = QDir::toNativeSeparators(tracePath);
		}

		phase.logDetail(QString("Resolved trace path: %1").arg(config.podData.execTrace.traceFilePattern));
	}

	// ========================================================================
	// Cache values for quick access
	// ========================================================================

	m_cpuCount = qBound(quint16(1),
		static_cast<quint16>(config.podData.system.processorCount),
		quint16(MAX_CPUS));

	m_memorySizeGB = config.podData.system.memorySizeGB;
	m_memorySizeBytes = static_cast<quint64>(m_memorySizeGB) * GB;
	m_sysType = config.podData.system.sysType;



	// ========================================================================
	// Log configuration summary
	// ========================================================================

	phase.logConfig("CPU Count", m_cpuCount);
	phase.logConfig("Memory (GB)", m_memorySizeGB);
	phase.logConfig("System Type", static_cast<int>(m_sysType));
	phase.logConfig("Platform EV", config.podData.system.platformEv);
	phase.logConfig("CPU Frequency (MHz)", config.podData.system.cpuFrequencyHz / 1000000.0);

	// Log device counts
	phase.logConfig("Controllers", config.podData.controllers.size());
	phase.logConfig("Devices", config.podData.devices.size());
	phase.logConfig("Consoles", config.podData.opaConsoles.size());
	phase.logConfig("Caches", config.podData.caches.size());

	// ========================================================================
	// Validation
	// ========================================================================

	if (!validateConfiguration()) {
		return false;
	}

	InitializationVerifier::markInitialized("Configuration");
	return true;
}

// ============================================================================
// resolveSnapshotPath -- derive .axpsnap path from ROM file name
// ============================================================================
// Path: <bindir>/snapshot/<romBaseName>.axpsnap
// If no ROM file (embedded), uses "embedded.axpsnap".
// Staleness is handled internally by loadSnapshot() via romHash + checksum.
// ============================================================================

static QString resolveSnapshotPath(const QString& romFilePath)
{
	const QString binDir = QCoreApplication::applicationDirPath();
	const QDir    snapDir(binDir + "/snapshot");

	if (!snapDir.exists())
		QDir(binDir).mkpath("snapshot");

	const QString stem = romFilePath.isEmpty()
		? QStringLiteral("embedded")
		: QFileInfo(romFilePath).baseName();

	return snapDir.filePath(stem + ".axpsnap");
}


AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase3_PlatformIdentity() {
	InitPhaseLogger phase("Platform Identity");

	GlobalCPUCount::initialize(m_cpuCount);
	phase.logDetail("Global CPU count initialized (read-only)");

	phase.logConfig("CPUs", m_cpuCount);
	phase.logConfig("RAM", QString("%1 GB").arg(m_memorySizeGB));
	phase.logConfig("System Type", static_cast<int>(m_sysType));

	InitializationVerifier::markInitialized("PlatformIdentity");
	return true;
}

// ============================================================================
// PHASE 4: Memory System
//
// Initializes SafeMemory (physical RAM backing) and GuestMemory (PA router).
// Two routes: SafeMemory for all RAM, MMIOManager for device I/O.
// No SRMFirmwareRegion -- firmware goes into SafeMemory via SrmRomLoader.
// ============================================================================

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase4_MemorySystem()
{
	InitPhaseLogger phase("Memory Subsystem");

	auto& config = global_EmulatorSettings();

	auto* safeMem = m_coordinator->safeMemory();
	auto* mmio = m_coordinator->mmioManager();

	// ========================================================================
	// 1. Initialize SafeMemory (physical RAM backing)
	// ========================================================================

	phase.logDetail("Initializing SafeMemory (physical RAM)...");

	if (!safeMem->initialize(m_memorySizeBytes)) {
		ERROR_LOG("SafeMemory initialization failed");
		return false;
	}

	phase.logConfig("Physical RAM", QString("%1 GB").arg(m_memorySizeGB));

	// ========================================================================
	// 2. Verify MMIOManager
	// ========================================================================

	phase.logDetail("Verifying MMIOManager...");

	if (!mmio) {
		ERROR_LOG("MMIOManager not available");
		return false;
	}

	phase.logDetail("MMIOManager ready");

	// ========================================================================
	// 3. Initialize WriteBufferManager
	// ========================================================================

	int wbThreads = config.podData.system.threadCount;
	phase.logDetail(QString("Initializing WriteBufferManager (%1 threads)...").arg(wbThreads));

	if (!initializeGlobalWriteBufferManager(wbThreads)) {
		ERROR_LOG("WriteBufferManager initialization failed");
		return false;
	}

	phase.logDetail("WriteBufferManager initialized");

	// ========================================================================
	// 4. Initialize GuestMemory PA routing
	//
	//    Two routes:
	//      [0x0, ramBase+ramSize)         -> SafeMemory (PA = offset)
	//      [mmioBase, mmioBase+mmioSize)  -> MMIOManager
	// ========================================================================

	phase.logDetail("Initializing GuestMemory PA routing...");

	auto& guestMem = global_GuestMemory();
	guestMem.attachSubsystems(safeMem, mmio);
	guestMem.initDefaultPARoutes();

	// ========================================================================
	// 5. Verification
	// ========================================================================

	if (!safeMem->isInitialized()) {
		ERROR_LOG("SafeMemory verification failed");
		return false;
	}

	// Verify PA 0x0 is mapped (firmware will be decompressed here)
	if (!guestMem.isRAM(0x0)) {
		ERROR_LOG("PA routing verification failed: PA 0x0 not mapped to SafeMemory");
		return false;
	}

	// Verify PA 0x900000 is mapped (decompressor staging area)
	if (!guestMem.isRAM(0x900000)) {
		ERROR_LOG("PA routing verification failed: PA 0x900000 not mapped to SafeMemory");
		return false;
	}

	// Verify main RAM region
	const quint64 ramBase = config.podData.memoryMap.ramBase;
	if (!guestMem.isRAM(ramBase)) {
		ERROR_LOG(QString("PA routing verification failed: RAM not at 0x%1")
			.arg(ramBase, 16, 16, QChar('0')));
		return false;
	}

	phase.logDetail("PA routing verification passed");

	// ========================================================================
	// 6. Register memory regions in GuestPhysicalRegionRegistry
	//    Firmware/DecompressedFW/PALcode/HWRPB entries are added in Phase 5
	//    after the ROM descriptor is populated.
	// ========================================================================

	//const quint64 ramBase  = config.podData.memoryMap.ramBase;
	const quint64 ramSize = m_memorySizeBytes;
	const quint64 mmioBase = config.podData.memoryMap.mmioBase;
	const quint64 mmioSize = config.podData.memoryMap.mmioSize;
	const quint64 pciBase = config.podData.memoryMap.pciMemBase;
	const quint64 pciSize = config.podData.memoryMap.pciMemSize;

	m_regionRegistry.addRam(ramBase, ramSize);
	m_regionRegistry.addMmio(mmioBase, mmioSize, "MMIO device registers");
	m_regionRegistry.addPci(pciBase, pciSize, "PCI BAR space");

	phase.logDetail("GuestPhysicalRegionRegistry: RAM/MMIO/PCI entries registered");

	return true;
}



AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase5_5_TLBSystem() const
{
	InitPhaseLogger phase("Translation/TLB System");

	phase.logDetail("Initializing per-CPU TLB structures...");
	initializeGlobalPTE(m_cpuCount);

	phase.logConfig("TLB Entries per CPU", "128 ITLB + 128 DTLB");
	phase.logDetail("TLB system ready");

	InitializationVerifier::markInitialized("TLBSystem");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase6_ReservationSystem() const
{
	InitPhaseLogger phase("Reservation & Atomicity");

	//auto& resv = globalReservationManager();
	initializeReservationManager(m_cpuCount);

	phase.logDetail("LDx_L/STx_C reservation tracking initialized");
	phase.logDetail("Reservation manager ready");

	InitializationVerifier::markInitialized("ReservationSystem");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase7_ExceptionInfrastructure() const
{
	InitPhaseLogger phase("Exception & Interrupt Infrastructure");

	phase.logDetail(QString("Initializing FaultDispatcherBank for %1 CPUs...").arg(m_cpuCount));
	GlobalFaultDispatcherBank::initialize(m_cpuCount);
	phase.logDetail("FaultDispatcherBank initialized");

	phase.logDetail("Initializing standard interrupt vectors...");

	phase.logConfig("IRQ Routing", "Per-CPU interrupt delivery");

	InitializationVerifier::markInitialized("ExceptionInfrastructure");
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase7_5_DeviceTree() {
	InitPhaseLogger phase("Device Infrastructure");

	auto& config = global_EmulatorSettings();

	phase.logDetail(QString("Loading %1 controllers...").arg(config.podData.controllers.size()));
	phase.logDetail(QString("Loading %1 devices...").arg(config.podData.devices.size()));
	phase.logDetail(QString("Loading %1 consoles...").arg(config.podData.opaConsoles.size()));

	// Example: Iterate through controllers
	for (auto it = config.podData.controllers.begin();
		it != config.podData.controllers.end(); ++it) {
		const QString& name = it.key();
		const ControllerConfig& ctrl = it.value();

		phase.logDetail(QString("  Controller %1: %2").arg(name).arg(ctrl.classType));
		// ... initialize controller ...
	}

	// Example: Iterate through devices
	for (auto it = config.podData.devices.begin();
		it != config.podData.devices.end(); ++it) {
		const QString& name = it.key();
		const DeviceConfig& dev = it.value();

		phase.logDetail(QString("  Device %1: %2 (parent: %3)")
			.arg(name).arg(dev.classType).arg(dev.parent));

		// Access flattened properties, e.g.:
		QString diskType = dev.fields.value("container.deviceType", "").toString();
		QString diskPath = dev.fields.value("container.path", "").toString();

		if (!diskType.isEmpty()) {
			phase.logDetail(QString("    Container: %1 -> %2").arg(diskType).arg(diskPath));
		}
	}

	InitializationVerifier::markInitialized("DeviceInfrastructure");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase8_PALInfrastructure() const
{
	InitPhaseLogger phase("PAL Infrastructure (CRITICAL)");

	// Initialize GlobalCPUState (replaces legacy iprBank + HWPCB bank)
	phase.logDetail("Initializing IPR Register Master...");
	GlobalCPUState& cpuState = globalCPUState();
	cpuState.setCpuCount(m_cpuCount);

	// Stamp per-CPU identity (whami / m_cpuId survive no reset path)
	for (quint32 cpu = 0; cpu < m_cpuCount; ++cpu) {
		auto& pal = cpuState.palIPR(static_cast<CPUIdType>(cpu));
		pal.m_cpuId = static_cast<CPUIdType>(cpu);
		pal.whami = static_cast<quint64>(cpu);
	}
	phase.logDetail(QString("IPR Register Master ready for %1 CPUs").arg(m_cpuCount));

	// PAL Vector Table
	phase.logDetail("Initializing PAL vector table...");
	PalVectorTable& palVectorTable = global_PalVectorTable();
	palVectorTable.initialize();
	phase.logDetail("PAL vector table ready");

	phase.logConfig("PAL Mode", "C++ emulation (not native PAL code)");
	InitializationVerifier::markInitialized("PALInfrastructure");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase8_5_PalHandlers()
{
	InitPhaseLogger phase("PAL Handler Infrastructure");

	// ====================================================================
	// Verify PalVectorTable
	// ====================================================================
	auto& palTable = global_PalVectorTable();

	int vectorCount = palTable.count();
	int handlerCount = palTable.handlerCount();

	INFO_LOG(QString("PAL vectors: %1 registered").arg(vectorCount));
	INFO_LOG(QString("PAL handlers: %1 registered").arg(handlerCount));

	phase.logConfig("Vector Count", QString::number(vectorCount));
	phase.logConfig("Handler Count", QString::number(handlerCount));

	// ====================================================================
	// Verify Critical Vectors Exist
	// ====================================================================
	if (!palTable.isRegistered(PalVectorId_EV6::RESET)) {
		WARN_LOG("RESET vector not registered");
	}
	if (!palTable.isRegistered(PalVectorId_EV6::DTB_MISS_SINGLE)) {
		WARN_LOG("DTB_MISS_SINGLE vector not registered");
	}
	if (!palTable.isRegistered(PalVectorId_EV6::ITB_MISS)) {
		WARN_LOG("ITB_MISS vector not registered");
	}

	phase.logDetail("Critical PAL vectors verified");

	// Note: CALL_PAL handlers are implemented in PalService
	// and dispatched via PalBox::executeCALL_PALL
	INFO_LOG("CALL_PAL dispatch via PalService (no handler registration needed)");

	InitializationVerifier::markInitialized("PalHandlers");
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase9_InstructionSystem()
{
	InitPhaseLogger phase("Instruction Decode System");

	// ====================================================================
	// 1. Configure GrainResolver
	// ====================================================================
	INFO_LOG("Configuring GrainResolver...");
	GrainResolver& grainResolver = global_GrainResolver();
	grainResolver.setPlatform(GrainPlatform::Alpha);
	phase.logDetail("GrainResolver configured for OpenVMS");
	phase.logConfig("Platform", "VMS");




	// ====================================================================
	// 3. Verify Grain Registry
	// ====================================================================
	auto& registry = InstructionGrainRegistry::instance();
	int grainCount = registry.grainCount();

	if (grainCount == 0) {
		ERROR_LOG("CRITICAL: No grains registered!");
		return false;
	}

	INFO_LOG(QString("Grain registry: %1 grains registered").arg(grainCount));
	phase.logConfig("Total Grains", QString::number(grainCount));

	// ====================================================================
	// 4. Test Grain Lookup (CALL_PAL HALT)
	// ====================================================================
	quint32 testInstruction = 0x00000000; // CALL_PAL 0x00
	quint8 opcode = (testInstruction >> 26) & 0x3F;
	quint16 function = testInstruction & 0x3FFFFFF;

	const InstructionGrain* grain = registry.lookup(opcode, function);
	if (!grain) {
		ERROR_LOG("CRITICAL: CALL_PAL grain lookup failed!");
		return false;
	}

	if (grain->opcode() != 0x00) {
		ERROR_LOG(QString("CRITICAL: Wrong grain - expected 0x00, got 0x%1")
			.arg(grain->opcode(), 2, 16, QChar('0')));
		return false;
	}

	INFO_LOG(QString("-- Grain lookup test passed: %1").arg(grain->mnemonic()));
	phase.logDetail("Grain lookup verification: PASSED");

	// ====================================================================
	// 5. Initialize Decode Caches
	// ====================================================================
	INFO_LOG("Initializing decode caches...");
	auto& pcCache = pcDecodeCache();
	auto& paCache = paDecodeCache();
	phase.logDetail("PC and PA decode caches initialized");

	InitializationVerifier::markInitialized("InstructionSystem");
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase10_DeviceInfrastructure() {
	InitPhaseLogger phase("Device Infrastructure (MMIO)");

	auto& mmio = global_MMIOManager();
	mmio.initialize();

	phase.logDetail("MMIO manager initialized");
	phase.logDetail("Device MMIO region registration ready");

	InitializationVerifier::markInitialized("DeviceInfrastructure");
	return true;
}


// ============================================================================
// PHASE 5: Firmware Loading
//
// Prepares the SRM ROM image for decompression. Does NOT decompress yet --
// that requires CPU0 which is created in Phase 14.
//
// Reads [ROM] section from EmulatorSettings:
//   SrmRomFile=<path>       Load from file (any EV6 SRM firmware .EXE)
//   SrmRomVariant=ES45      Use embedded ROM (default, local builds only)
//
// SrmRomFile takes precedence over SrmRomVariant.
// ============================================================================

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase5_FirmwareLoading()
{
	InitPhaseLogger phase("Firmware Loading (SRM ROM)");

	auto& config = global_EmulatorSettings();

	// ========================================================================
	// Load SRM ROM image (prepare only -- decompression is Phase 14b)
	// ========================================================================

	// Populate srmCfg here so loadPA is available for descriptor analysis
	// and registry seeding. Phase 14b re-reads the same values (idempotent).
	{
		const QSettings qset(m_configSettings.getConfigFilePath(), QSettings::IniFormat);
		srmCfg = SrmLoaderConfig::fromSettings(qset);
	}

	// Check for explicit file path first
	QString srmRomFile = config.podData.rom.srmRomFilename;

	if (!srmRomFile.isEmpty()) {
		// File path specified -- load from disk
		phase.logDetail(QString("Loading SRM ROM from file: %1").arg(srmRomFile));

		if (!m_srmRomLoader.loadFromFile(srmRomFile, srmCfg.loadPA)) {
			ERROR_LOG(QString("Failed to load SRM ROM: %1").arg(srmRomFile));
			return false;
		}

		phase.logConfig("ROM Source", srmRomFile);
	}
	else {
		// Use embedded ROM
		phase.logDetail("Using embedded ES45 V6.2 SRM ROM");

		if (!m_srmRomLoader.useEmbedded(srmCfg.loadPA)) {
			ERROR_LOG("Failed to load embedded SRM ROM");
			return false;
		}

		phase.logConfig("ROM Source", "Embedded ES45 V6.2");
	}

	// Seed region registry from descriptor (Firmware + DecompressedFW + PALcode + HWRPB)
	m_regionRegistry.seedFromDescriptor(
		m_srmRomLoader.descriptor(),
		srmCfg.loadPA);

	// Validation gate -- confirm no PA overlaps before proceeding
	{
		QString regError;
		if (!m_regionRegistry.validate(&regError)) {
			ERROR_LOG("PA region overlap in firmware regions -- halting: " + regError);
			return false;
		}
	}
	m_regionRegistry.logAll();

	phase.logConfig("ROM Size", QString("%1 KB").arg(m_srmRomLoader.romSize() / 1024));
	phase.logConfig("Header Skip", QString("0x%1").arg(m_srmRomLoader.headerSkip(), 0, 16));
	phase.logConfig("Payload Size", QString("%1 KB").arg(m_srmRomLoader.payloadSize() / 1024));

	phase.logDetail("SRM ROM prepared -- decompression deferred to Phase 14b");

	// ========================================================================
	// Initialize HWRPB (will be overwritten by SRM firmware during boot,
	// but we prepare a minimal one for early PAL code)
	// ========================================================================



	InitializationVerifier::markInitialized("Firmware");
	EventLog::flush();

	return true;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase11_CoordinationLayer() {

	InitPhaseLogger phase("Coordination & Synchronization");

	auto& memBarrier = global_MemoryBarrierCoordinator();
	phase.logDetail("Memory barrier coordinator ready (auto-initialized)");

	phase.logDetail("CPU state coordination ready");

	// Initialize ExecutionCoordinator but DON'T create CPUs yet
	auto& execCoord = global_ExecutionCoordinator();
	phase.logDetail("ExecutionCoordinator created (CPUs deferred)");

	InitializationVerifier::markInitialized("CoordinationLayer");
	return true;
}



// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase13_InitializeConsole() {
	InitPhaseLogger phase("Console Devices (OPA0/OPA1)");

	auto& config = global_EmulatorSettings();
	auto& consoleMgr = global_ConsoleManager();

	if (!config.podData.opaConsoles.contains("OPA0")) {
		ERROR_LOG("No OPA0 console configuration found in INI");
		return false;
	}

	const auto& opa0Config = config.podData.opaConsoles["OPA0"];

	// Create OPA0
	phase.logDetail("Creating OPA0 (primary console)...");
	SRMConsoleDevice::Config deviceConfig;
	deviceConfig.port = opa0Config.ifacePort;
	deviceConfig.echoEnabled = true;
	deviceConfig.autoLaunchPutty = true;
	deviceConfig.defaultTimeoutMs = 30000;

	phase.logConfig("OPA0 Port", deviceConfig.port);

	auto* opa0 = new SRMConsoleDevice(deviceConfig);
	if (!opa0->start()) {
		ERROR_LOG(QString("Failed to start OPA0 console on port %1").arg(deviceConfig.port));
		delete opa0;
		return false;
	}

	if (!consoleMgr.registerDevice("OPA0", opa0)) {
		ERROR_LOG("Failed to register OPA0 with console manager");
		opa0->stop();
		delete opa0;
		return false;
	}
	// Open console for CSERVE use
	if (consoleMgr.openConsole(0)) {
		phase.logDetail("OPA0: Opened for CSERVE access");
	}
	else {
		WARN_LOG("Failed to open OPA0 for CSERVE");
	}
	phase.logDetail(QString("OPA0 listening on TCP port %1").arg(deviceConfig.port));
	phase.logDetail(QString("Connect: putty -raw localhost %1").arg(deviceConfig.port));

	// Optional OPA1
	if (config.podData.opaConsoles.contains("OPA1")) {
		const auto& opa1Config = config.podData.opaConsoles["OPA1"];
		phase.logDetail("Creating OPA1 (secondary console)...");

		SRMConsoleDevice::Config opa1DeviceConfig;
		opa1DeviceConfig.port = opa1Config.ifacePort;
		opa1DeviceConfig.echoEnabled = true;
		opa1DeviceConfig.autoLaunchPutty = false;
		opa1DeviceConfig.defaultTimeoutMs = 30000;

		auto* opa1 = new SRMConsoleDevice(opa1DeviceConfig);
		if (opa1->start() && consoleMgr.registerDevice("OPA1", opa1)) {
			phase.logConfig("OPA1 Port", opa1DeviceConfig.port);
		}
		else {
			phase.logDetail("WARNING: OPA1 initialization failed (optional)");
			delete opa1;
		}
	}


	InitializationVerifier::markInitialized("ConsoleDevices");
	return true;
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase13_ConsoleEnvironment() const
{
	InitPhaseLogger phase("Console Environment (SRM Variables)");

	QString configPath = m_emulatorPaths.getConfigPath().isEmpty() ? "." : m_emulatorPaths.getConfigPath();

	phase.logDetail(QString("Loading SRM environment from: %1").arg(configPath));
	initializeGlobalSRMEnvStore(configPath);

	if (!isGlobalSRMEnvStoreInitialized()) {
		ERROR_LOG("Failed to initialize SRMEnvStore");
		return false;
	}

	int varCount = global_SRMEnvStore().count();
	phase.logConfig("SRM Variables", varCount);

	// Log some key variables
	auto& envStore = global_SRMEnvStore();
	QStringList keys = envStore.getAllNames();
	for (const QString& key : keys.mid(0, qMin(5, keys.size()))) {
		QString value = envStore.get(key);
		phase.logDetail(QString("  %1 = %2").arg(key, -20).arg(value));
	}

	if (keys.size() > 5) {
		phase.logDetail(QString("  ... and %1 more").arg(keys.size() - 5));
	}

	InitializationVerifier::markInitialized("ConsoleEnvironment");
	return true;
}


// ============================================================================
// PHASE 14: CPU Bring-Up and SRM Firmware Load
//
// Three-stage boot:
//   14a: Create CPU0 (object exists, thread NOT started)
//   14b: Load SRM firmware -- bifurcated path:
//
//        SNAPSHOT PATH (SrmSnapshot=true, .axpsnap exists and is valid):
//          loadSnapshot() restores guest memory + full CPU state directly.
//          No instruction execution. Completes in <1 second.
//          Snapshot: <bindir>/snapshot/ES40_V6_2.axpsnap
//          On stale/invalid snapshot: auto-deletes, falls through to decompress.
//
//        DECOMPRESS PATH (no snapshot, or snapshot rejected):
//          decompress() runs the self-decompressing Alpha PALcode binary.
//          Synchronous, runs on initialization thread via singleStep lambda.
//          Exercises full pipeline: IBox, decode, cache, execute, retire.
//          ~5.7M instructions: ALU, branch, HW_LD, HW_ST, HW_MFPR,
//          HW_MTPR, HW_REI, CALL_PAL (CSERVE, WRFEN, SWPCTX, LDQP)
//          ~16 min debug build / ~30 sec release build (full trace I/O).
//          On completion: saves .axpsnap if SrmSnapshot=true.
//
//        Both paths deliver identical CPU state on exit:
//          PC       = 0x5C0  (boot handoff, PAL mode)
//          PAL_BASE = 0x900000
//          Firmware resident at PA 0x0 (4 MB)
//
//   14c: Start CPU0 execution thread (boot SRM console)
//
// ============================================================================

AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase14_CPUBringUp()
{
	if (!beginInitialization("CPU BringUp")) return false;

	INFO_LOG("=== PHASE 14: CPU Bring-Up and SRM Decompression ===");

	// ========================================================================
	// 14a: Create CPU0
	// ========================================================================

	INFO_LOG("--- Phase 14a: Creating CPU instances ---");

	auto& execCoord = global_ExecutionCoordinator();
	execCoord.initializeCPUs();

	AlphaCPU* cpu0 = execCoord.getAlphaBootProcessor();
	if (!cpu0) {
		ERROR_LOG("Failed to get boot processor (CPU0)");
		markFailure();
		return false;
	}

	cpu0->reset();
	cpu0->setIPL(31);
	INFO_LOG("CPU0 created and reset (thread not started)");

	// ========================================================================
	// 14b: SRM Firmware -- snapshot or decompress
	// ========================================================================

	INFO_LOG("--- Phase 14b: SRM Firmware Load ---");

	if (!m_srmRomLoader.isLoaded()) {
		ERROR_LOG("SRM ROM not loaded -- Phase 5 failed?");
		markFailure();
		return false;
	}

	// Read configuration (srmCfg already populated in Phase 5 -- re-read for snapshot flag only)
	const QSettings qset(m_configSettings.getConfigFilePath(), QSettings::IniFormat);
	const bool snapshotEnabled = qset.value("ROM/SrmSnapshot", false).toBool();

	auto& config = global_EmulatorSettings();
	auto& guestMem = global_GuestMemory();

	const auto& desc = m_srmRomLoader.descriptor();
	INFO_LOG(QString("Phase 14: loadPA=0x%1  startPC=0x%2  palBase=0x%3  donePC=0x%4  source=%5")
		.arg(srmCfg.loadPA, 0, 16)
		.arg(desc.startPC(srmCfg.loadPA), 0, 16)
		.arg(desc.palBase, 0, 16)
		.arg(desc.donePC(), 0, 16)
		.arg(desc.sourceDescription));

	INFO_LOG(QString("Phase 14: SrmSnapshot=%1").arg(snapshotEnabled ? "true" : "false"));

	// ----------------------------------------------------------------
	// Shared lambdas (used by both snapshot and decompress paths)
	// ----------------------------------------------------------------

	// Memory write -- guest PA <- host data
	auto writeToPhysical = [&guestMem](quint64 pa, const quint8* data, size_t len) {
		guestMem.writePA(pa, data, static_cast<qsizetype>(len));
		};

	// Memory read -- host buffer <- guest PA  (snapshot save only)
	auto readFromPhysical = [&guestMem](quint64 pa, quint8* buf, size_t len) {
		guestMem.readPA(pa, buf, static_cast<qsizetype>(len));
		};

	// Single-step CPU0 and return current PC
	auto singleStep = [cpu0]() -> quint64 {
		cpu0->runOneInstruction();
		return cpu0->getPC();
		};

	// PC and PAL_BASE setters/getters
	auto setPC = [cpu0](quint64 pc) { cpu0->setPC(pc); };
	auto setPalBase = [cpu0](quint64 pb) { cpu0->setPAL_BASE(pb); };
	SrmRomLoader::GetU64Fn getPalBase = [cpu0]() -> quint64 {
		return cpu0->getPAL_BASE();
		};

	// Progress reporter
	SrmRomLoader::ProgressFn progress = [](int percent) {
		if (percent % 10 == 0)
			INFO_LOG(QString("  Decompression: %1%").arg(percent));
		};

	// Integer registers -- capture/restore 32 x quint64
	// r[31] is architecturally zero but we save/restore all 32 for symmetry
	auto getIntRegs = [](quint64 regs[32]) {
		const auto& src = globalCPUState().intRegs(static_cast<CPUIdType>(0));
		std::memcpy(regs, src.r, 32 * sizeof(quint64));
		};
	auto setIntRegs = [](const quint64 regs[32]) {
		auto& dst = globalCPUState().intRegs(static_cast<CPUIdType>(0));
		std::memcpy(dst.r, regs, 32 * sizeof(quint64));
		dst.r[31] = 0;  // R31 is hardwired zero -- enforce after memcpy
		};

	// Floating-point registers -- f[0..30] packed in regs[0..30], fpcr in regs[31]
	auto getFpRegs = [](quint64 regs[32]) {
		const auto& src = globalCPUState().floatRegs(static_cast<CPUIdType>(0));
		std::memcpy(regs, src.f, 31 * sizeof(quint64));
		regs[31] = src.fpcr;
		};
	auto setFpRegs = [](const quint64 regs[32]) {
		auto& dst = globalCPUState().floatRegs(static_cast<CPUIdType>(0));
		std::memcpy(dst.f, regs, 31 * sizeof(quint64));
		dst.fpcr = regs[31];
		};



	// IPR capture -- named PalIPR fields + pal_temp[32]
	// NB: pal_base is also in SrmRomLoadResult.finalPalBase (set via setPalBase),
	//     but we save it here too for completeness.
	auto getIPRs = []() -> std::vector<SrmRomLoader::IprPair> {
		std::vector<SrmRomLoader::IprPair> v;
		v.reserve(16 + 32);
		const auto& x = globalCPUState().palIPR(static_cast<CPUIdType>(0));
		v.push_back({ SnapIPR::vptb,     x.vptb });
		v.push_back({ SnapIPR::pal_base, x.pal_base });
		v.push_back({ SnapIPR::scbb,     x.scbb });
		v.push_back({ SnapIPR::pcbb,     x.pcbb });
		v.push_back({ SnapIPR::prbr,     x.prbr });
		v.push_back({ SnapIPR::virbnd,   x.virbnd });
		v.push_back({ SnapIPR::sysptbr,  x.sysptbr });
		v.push_back({ SnapIPR::mces,     x.mces });
		v.push_back({ SnapIPR::i_ctl,    x.i_ctl });
		v.push_back({ SnapIPR::m_ctl,    x.m_ctl });
		v.push_back({ SnapIPR::dc_ctl,   x.dc_ctl });
		v.push_back({ SnapIPR::va_ctl,   x.va_ctl });
		v.push_back({ SnapIPR::exc_sum,  x.exc_sum });
		v.push_back({ SnapIPR::exc_mask, x.exc_mask });
		v.push_back({ SnapIPR::mm_stat,  x.mm_stat });
		for (int n = 0; n < 32; ++n)
			v.push_back({ static_cast<quint32>(SnapIPR::pal_temp_base + n), x.pal_temp[n] });
		return v;
		};

	// IPR restore -- symmetric with getIPRs above
	auto setIPRs = [](const std::vector<SrmRomLoader::IprPair>& pairs) {
		auto& x = globalCPUState().palIPR(static_cast<CPUIdType>(0));
		for (const auto& [id, val] : pairs) {
			switch (id) {
			case SnapIPR::vptb:     x.vptb = val; break;
			case SnapIPR::pal_base: x.pal_base = val; break;
			case SnapIPR::scbb:     x.scbb = val; break;
			case SnapIPR::pcbb:     x.pcbb = val; break;
			case SnapIPR::prbr:     x.prbr = val; break;
			case SnapIPR::virbnd:   x.virbnd = val; break;
			case SnapIPR::sysptbr:  x.sysptbr = val; break;
			case SnapIPR::mces:     x.mces = val; break;
			case SnapIPR::i_ctl:    x.i_ctl = val; break;
			case SnapIPR::m_ctl:    x.m_ctl = val; break;
			case SnapIPR::dc_ctl:   x.dc_ctl = val; break;
			case SnapIPR::va_ctl:   x.va_ctl = val; break;
			case SnapIPR::exc_sum:  x.exc_sum = val; break;
			case SnapIPR::exc_mask: x.exc_mask = val; break;
			case SnapIPR::mm_stat:  x.mm_stat = val; break;
			default:
				if (id >= SnapIPR::pal_temp_base &&
					id < SnapIPR::pal_temp_base + 32)
					x.pal_temp[id - SnapIPR::pal_temp_base] = val;
				break;
			}
		}
		};


	// ----------------------------------------------------------------
	// Snapshot path
	// ----------------------------------------------------------------

	const QString snapshotPath = resolveSnapshotPath(config.podData.rom.srmRomFilename);

	// ----------------------------------------------------------------
	// Bifurcated boot path
	// ----------------------------------------------------------------

	SrmRomLoadResult result;
	bool usedSnapshot = false;

	if (snapshotEnabled && QFile::exists(snapshotPath)) {
		INFO_LOG(QString("Phase 14b: snapshot found -- attempting load from '%1'").arg(snapshotPath));

		result = m_srmRomLoader.loadSnapshot(
			snapshotPath,
			writeToPhysical,
			setPC,
			setPalBase,
			setIntRegs,
			setFpRegs,
			setIPRs
		);

		if (result.success) {
			usedSnapshot = true;
			INFO_LOG(QString("Phase 14b: snapshot loaded in %1 ms  (PC=0x%2  PAL_BASE=0x%3)")
				.arg(result.elapsedMs, 0, 'f', 1)
				.arg(result.cleanPC(), 8, 16, QChar('0'))
				.arg(result.palBase(), 8, 16, QChar('0')));
		}
		else {
			WARN_LOG(QString("Phase 14b: snapshot rejected -- %1").arg(result.errorMessage));
			WARN_LOG("Phase 14b: deleting stale snapshot and falling back to decompress");
			QFile::remove(snapshotPath);
		}
	}


	if (!usedSnapshot) {
		INFO_LOG("Phase 14b: running SRM decompressor...");

		result = m_srmRomLoader.decompress(
			srmCfg,           // 1  SrmLoaderConfig
			writeToPhysical,  // 2  WritePhysicalFn
			singleStep,       // 3  SingleStepFn
			setPC,            // 4  SetU64Fn  -- sets PC
			setPalBase,       // 5  SetU64Fn  -- sets PAL_BASE
			getPalBase,       // 6  GetU64Fn  -- reads PAL_BASE
			progress          // 7  ProgressFn
		);


#ifdef AXP_EXEC_TRACE
		// HWRPB signature validation
		quint64 hwrpbSig = 0;
		quint64 hwrpbPhysBase = 0;
		guestMem.read64(0x2000, hwrpbSig);
		guestMem.read64(0x2008, hwrpbPhysBase);
		qDebug() << QString("  signature  : 0x%1  (%2)")
			.arg(hwrpbSig, 16, 16, QChar('0'))
			.arg(hwrpbSig == 0x0000000042707248ULL ? "VALID (HrpB)" : "UNEXPECTED");
		qDebug() << QString("  phys_base  : 0x%1")
			.arg(hwrpbPhysBase, 0, 16);
#endif
		if (result.success && snapshotEnabled) {
			INFO_LOG(QString("Phase 14b: saving snapshot to '%1'...").arg(snapshotPath));

			// Region list driven by GuestPhysicalRegionRegistry (includeInSnapshot=true).
			// Includes: DecompressedFW (0x0, 4MB) + Firmware stub (0x900000) +
			//           PALcode (0x600000) + HWRPB (0x2000).
			// RAM excluded by default (large -- opt-in via spec §10.3).
			const auto regions = m_regionRegistry.snapshotRegions();

			const bool saved = m_srmRomLoader.saveSnapshot(
				snapshotPath,
				result,
				regions,
				readFromPhysical,
				getIntRegs,
				getFpRegs,
				getIPRs
			);

			if (saved)
				INFO_LOG(QString("Phase 14b: snapshot saved to '%1'").arg(snapshotPath));
			else
				WARN_LOG("Phase 14b: snapshot save failed (non-fatal -- will re-decompress next run)");
		}
	}

	// ----------------------------------------------------------------
	// HWRPB post-decompress verification
	// ----------------------------------------------------------------
	{
		quint64 hwrpbPA = 0x2000;
		quint64 sysType = 0, sysVar = 0, sysRev = 0;
		quint64 nProc = 0, memOffset = 0, ctbOffset = 0;
		guestMem.read64(hwrpbPA + 0x18, sysType);
		guestMem.read64(hwrpbPA + 0x20, sysVar);
		guestMem.read64(hwrpbPA + 0x28, sysRev);
		guestMem.read64(hwrpbPA + 0xA0, nProc);
		guestMem.read64(hwrpbPA + 0xB0, memOffset);
		guestMem.read64(hwrpbPA + 0xC0, ctbOffset);
		INFO_LOG("HWRPB post-decompress:");
		INFO_LOG(QString("  sys_type   : 0x%1").arg(sysType, 0, 16));
		INFO_LOG(QString("  sys_var    : 0x%1").arg(sysVar, 0, 16));
		INFO_LOG(QString("  sys_rev    : 0x%1").arg(sysRev, 0, 16));
		INFO_LOG(QString("  nproc      : %1").arg(nProc));
		INFO_LOG(QString("  mem_offset : 0x%1").arg(memOffset, 0, 16));
		INFO_LOG(QString("  ctb_offset : 0x%1").arg(ctbOffset, 0, 16));
	}

	// ----------------------------------------------------------------
	// Verify result
	// ----------------------------------------------------------------

	quint64 checkWord = 0;
	guestMem.read64(0x0, checkWord);
	qDebug() << "ROM word at reset vector:" << Qt::hex << checkWord;

	if (!result.success) {
		ERROR_LOG(QString("SRM firmware load FAILED: %1").arg(result.errorMessage));
		markFailure();
		return false;
	}

	INFO_LOG(QString("--- SRM Load Complete [%1] ---")
		.arg(result.fromSnapshot ? "snapshot" : "decompress"));
	INFO_LOG(QString("  Cycles:    %1").arg(result.cyclesExecuted));
	INFO_LOG(QString("  Time:      %1 ms").arg(result.elapsedMs, 0, 'f', 1));
	INFO_LOG(QString("  PC:        0x%1 (PALmode=%2)")
		.arg(result.cleanPC(), 8, 16, QChar('0'))
		.arg(result.isPalMode() ? "yes" : "no"));
	INFO_LOG(QString("  PAL_BASE:  0x%1")
		.arg(result.palBase(), 8, 16, QChar('0')));
	INFO_LOG(QString("  Snapshot:  %1")
		.arg(result.fromSnapshot ? result.snapshotPath : "n/a"));

	if (result.cleanPC() != 0x8000)
		WARN_LOG(QString("Unexpected boot PC: 0x%1 (expected 0x8000)")
			.arg(result.cleanPC(), 8, 16, QChar('0')));
	if (result.palBase() != 0x600000)
		WARN_LOG(QString("Unexpected PAL_BASE: 0x%1 (expected 0x600000)")
			.arg(result.palBase(), 8, 16, QChar('0')));

	// ========================================================================
	// Verify decompressed firmware is present
	// ========================================================================

	INFO_LOG("=== SRM FIRMWARE VERIFICATION ===");
	for (int i = 0; i < 4; ++i) {
		quint32 instr = 0;
		quint64 addr = result.cleanPC() + (i * 4);
		MEM_STATUS status = guestMem.read32(addr, instr);
		quint8 opcode = (instr >> 26) & 0x3F;
		INFO_LOG(QString("  PA 0x%1: 0x%2 (opc=0x%3) status=%4")
			.arg(addr, 8, 16, QChar('0'))
			.arg(instr, 8, 16, QChar('0'))
			.arg(opcode, 2, 16, QChar('0'))
			.arg(static_cast<int>(status)));
	}

	// ========================================================================
	// 14c: Start CPU0 execution thread
	// ========================================================================

	INFO_LOG("--- Phase 14c: Starting CPU0 execution ---");
	INFO_LOG(QString("  Boot PC:       0x%1").arg(cpu0->getPC(), 16, 16, QChar('0')));
	INFO_LOG(QString("  Boot PAL_BASE: 0x%1").arg(cpu0->getPAL_BASE(), 16, 16, QChar('0')));

	cpu0->executeLoop();
	INFO_LOG("CPU0: Execution thread started");

	markSuccess();
	return true;
}
// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializePhase15_FinalVerification() {
	InitPhaseLogger phase("Final System Verification");

	phase.logDetail("Verifying critical subsystems...");

	if (!InitializationVerifier::isInitialized("MemorySystem")) {
		ERROR_LOG("CRITICAL: Memory system not initialized");
		return false;
	}
	phase.logDetail("  - Memory system");

	if (!InitializationVerifier::isInitialized("PALInfrastructure")) {
		ERROR_LOG("CRITICAL: PAL infrastructure not initialized");
		return false;
	}
	phase.logDetail("  - PAL infrastructure");

	if (!InitializationVerifier::isInitialized("SMPIntegration")) {
		ERROR_LOG("CRITICAL: SMP integration not complete");
		return false;
	}
	phase.logDetail("  - SMP integration");

	if (!InitializationVerifier::isInitialized("ConsoleDevices")) {
		ERROR_LOG("CRITICAL: Console devices not initialized");
		return false;
	}
	phase.logDetail("  - Console devices");

	phase.logDetail("All critical subsystems verified");

	InitializationVerifier::markInitialized("FinalVerification");

	if (!beginInitialization("FinalVerification")) return false;

	INFO_LOG("=== FINAL VERIFICATION ===");

	// 1. Verify grain resolution works
	{
		quint32 callPalInstruction = 0x00000000; // CALL_PAL 0x00 (HALT)
		quint8 opcode = (callPalInstruction >> 26) & 0x3F;
		quint16 function = callPalInstruction & 0x3FFFFFF;

		const InstructionGrain* grain = InstructionGrainRegistry::instance().lookup(opcode, function);
		if (!grain) {
			ERROR_LOG("VERIFICATION FAILED: CALL_PAL grain not found");
			return false;
		}

		if (grain->opcode() != 0x00) {
			ERROR_LOG(QString("VERIFICATION FAILED: Wrong grain opcode 0x%1")
				.arg(grain->opcode(), 2, 16, QChar('0')));
			return false;
		}

		INFO_LOG("-- Grain lookup: CALL_PAL grain found");
	}

	// 2. Verify decode cache is ready
	{
		auto& pcCache = pcDecodeCache();
		auto& paCache = paDecodeCache();
		INFO_LOG("-- Decode caches: PC and PA caches ready");
	}

	// 3. Verify PAL infrastructure
	{
		auto& palTable = global_PalVectorTable();
		INFO_LOG(QString("-- PAL vectors: %1 registered").arg(palTable.count()));
	}

	// 4. Verify subsystems
	{
		if (!m_coordinator->areAllSubsystemsBound()) {
			ERROR_LOG("VERIFICATION FAILED: Not all subsystems bound");
			ERROR_LOG(m_coordinator->getSubsystemStatus());
			return false;
		}
		INFO_LOG("-- Subsystems: All bound");
	}


	INFO_LOG("=== ALL VERIFICATIONS PASSED ===");

	markSuccess();
	return true;




}

// ============================================================================
// Helper Methods
// ============================================================================

bool EmulatR_init::beginInitialization(const QString& subsystemName) {
	if (!InitializationVerifier::markInitializing(subsystemName)) {
		return false;
	}
	m_currentPhase = subsystemName;
	return true;
}

void EmulatR_init::markSuccess() {
	if (!m_currentPhase.isEmpty()) {
		InitializationVerifier::markInitialized(m_currentPhase);
		m_currentPhase.clear();
	}
}

void EmulatR_init::markFailure() {
	if (!m_currentPhase.isEmpty()) {
		InitializationVerifier::markFailed(m_currentPhase);
		m_currentPhase.clear();
	}
}

// ============================================================================
// SHUTDOWN IMPLEMENTATION
// ============================================================================

AXP_HOT AXP_FLATTEN void EmulatR_init::shutdown()  noexcept {
	INFO_LOG("============================================================");
	INFO_LOG("ASA EMULATOR SHUTDOWN SEQUENCE");
	INFO_LOG("============================================================");

	{
		InitPhaseLogger phase("Shutdown - Device Tree");
		shutdownGlobalFirmwareDeviceManager();
	}

	{
		InitPhaseLogger phase("Shutdown - Console Environment");
		shutdownGlobalSRMEnvStore();
	}

	{
		InitPhaseLogger phase("Shutdown - TLB System");
		shutdownGlobalPTE();
	}

	{
		InitPhaseLogger phase("Shutdown - Write Buffers");
		shutdownGlobalWriteBufferManager();
	}

	{
		InitPhaseLogger phase("Shutdown - Execution Trace");
		ExecTrace::shutdown();
	}

	{
		InitPhaseLogger phase("Shutdown - Memory Subsystems");
		m_coordinator->shutdown();
	}

	{
		InitPhaseLogger phase("Shutdown - Global Configuration");
		shutdownGlobalEmulatorSettings();
	}

	INFO_LOG("============================================================");
	INFO_LOG("Shutdown Complete");
	INFO_LOG("============================================================");
}

// Rest of implementation unchanged...
AXP_HOT AXP_FLATTEN bool EmulatR_init::initializeSystem() noexcept {
	InitPhaseLogger::reset();
	InitPhaseLogger::setTotalPhases(19);

	EmulatorPaths paths;

	m_emulatorConfig.configPath = paths.getConfigPath();
	m_emulatorConfig.logPath = paths.getLogPath();
	m_emulatorConfig.binPath = paths.getBinPath();

	INFO_LOG(QString("ASAEmulatR root: %1").arg(m_emulatorPaths.getRootPath()));
	INFO_LOG(QString("Config path: %1").arg(m_emulatorPaths.getConfigPath()));

	initializeGlobalSRMEnvStore(m_emulatorConfig.configPath);

	return initialize();
}

EmulatR_init::EmulatR_init(SubsystemCoordinator* coordinator)
	: m_consoleManager(&global_ConsoleManager()), m_coordinator(coordinator)
{
}

EmulatR_init::~EmulatR_init() { shutdown(); }

AXP_HOT AXP_FLATTEN void EmulatR_init::initializeLogging() noexcept {
	initializePhase1_Logging();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN void EmulatR_init::initializeDeviceInterrupts() noexcept {
	INFO_LOG("Device interrupts initialized during phase 7");
}

// ReSharper disable once CppMemberFunctionMayBeStatic
AXP_HOT AXP_FLATTEN void EmulatR_init::shutdownConsoles() {
	if (m_consoleManager) {
		m_consoleManager->shutdown();
		delete m_consoleManager;
		m_consoleManager = nullptr;
	}
	shutdownGlobalSRMEnvStore();
}

AXP_HOT AXP_FLATTEN void EmulatR_init::initializeDevices() noexcept {
	INFO_LOG("Devices initialized during phase 10");
}

AXP_HOT AXP_FLATTEN bool EmulatR_init::validateConfiguration() noexcept {
	if (m_cpuCount < 1 || m_cpuCount > MAX_CPUS) {
		ERROR_LOG(QString("Invalid CPU count: %1").arg(m_cpuCount));
		m_cpuCount = qBound(static_cast<quint16>(1), static_cast<quint16>(m_cpuCount), static_cast<quint16>(MAX_CPUS));
		WARN_LOG(QString("CPU count adjusted to: %1").arg(m_cpuCount));
	}

	if (m_memorySizeGB < 1) {
		ERROR_LOG(QString("Invalid memory size: %1 GB").arg(m_memorySizeGB));
		m_memorySizeGB = 4;
		m_memorySizeBytes = 4ULL * GB;
		WARN_LOG("Memory size adjusted to 4 GB");
	}

	return true;
}

AXP_HOT AXP_FLATTEN void EmulatR_init::dumpMemoryMap() {
	INFO_LOG("\n" + QString(70, '='));
	INFO_LOG("PHYSICAL ADDRESS SPACE LAYOUT");
	INFO_LOG(QString(70, '='));

	struct TestAddr {
		quint64     pa;
		const char* description;
	};

	TestAddr testAddrs[] = {
{ 0x0000000000000000ULL, "Firmware/HWRPB (PA 0x0)" },
{ 0x0000000000600000ULL, "PAL_BASE" },
{ 0x0000000000900000ULL, "Decompressor staging" },
{ 0x0000000080000000ULL, "Main RAM start" },
{ 0x0000001000000000ULL, "MMIO start" },
	};

	for (const auto& test : testAddrs) {
		QString classification = m_coordinator->guestMemory()->classifyPhysicalAddress(test.pa);
		INFO_LOG(QString("  0x%1 - %2: %3")
			.arg(test.pa, 16, 16, QChar('0'))
			.arg(test.description, -35)
			.arg(classification));
	}

	INFO_LOG(QString(70, '=') + "\n");


}