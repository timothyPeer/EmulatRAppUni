// ============================================================================
// EmulatorSettingsInline.h - COMPLETE REWRITE (Flattened Structure)
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

#ifndef EmulatorSettingsInline_h__
#define EmulatorSettingsInline_h__

// ============================================================================
//  EmulatorSettingsInline.h - COMPLETE REWRITE (Flattened Structure)
//
//  Purpose: Load EmulatorSettings from flattened ASAEmulatr.ini
//
//  Aligned with:
//  - settings.h (corrected structs)
//  - ASAEmulatr.ini (flattened, no nested sections)
// ============================================================================

#include "settings.h"
#include <QSettings>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <QString>

#include "coreLib/Axp_Attributes_core.h"
#include "emulatrLib/EmulatorPaths.h"

// Forward declare logging (avoid circular dependency)
void WARN_LOG_Fallback(const QString& msg);
void INFO_LOG_Fallback(const QString& msg);
void CRITICAL_LOG_Fallback(const QString& msg) ;

// ============================================================================
// EmulatorSettingsInline - Loads EmulatorSettings from INI
// ============================================================================

struct EmulatorSettingsInline
{
    EmulatorSettings podData;
    bool bAlreadyInitialized{ false };

    // ========================================================================
    // MAIN LOADER
    // ========================================================================

    inline bool loadFromIni(const QString& iniFile)
    {
        if (!QFileInfo::exists(iniFile)) {
            CRITICAL_LOG_Fallback(QString("Configuration file not found: %1").arg(iniFile));
            return false;
        }

        if (bAlreadyInitialized) {
            WARN_LOG_Fallback(QString("Configuration already loaded from: %1 (restart required for changes)").arg(iniFile));
            return false;
        }

        INFO_LOG_Fallback(QString("Loading configuration from: %1").arg(iniFile));

        QSettings ini(iniFile, QSettings::IniFormat);

        // Load all sections in order
        loadSystem(ini);
        loadLogging(ini);
        loadExecTrace(ini);
        loadTLBShootdown(ini);
        loadInterrupts(ini);
        loadFloatingPoint(ini);
        loadRom(ini);
        loadCaches(ini);
        loadDevicesAndControllers(ini);
        loadOPAConsoles(ini);
        loadMemoryMap(ini);

        bAlreadyInitialized = true;

        INFO_LOG_Fallback(QString("Configuration loaded: %1 controllers, %2 devices, %3 consoles")
            .arg(podData.controllers.size())
            .arg(podData.devices.size())
            .arg(podData.opaConsoles.size()));

        return true;
    }

    // ========================================================================
    // [System] Section
    // ========================================================================

    inline void loadSystem(QSettings& ini)
    {
        ini.beginGroup("System");

        podData.system.memorySizeGB = ini.value("MemorySizeGB", 32).toInt();
        podData.system.hwModel = ini.value("hw-Model", "ES40").toString();
        podData.system.hwSerialNumber = ini.value("hw-Serial-Number", "").toString();
        podData.system.coherencyCache = ini.value("Coherency-Cache", 2048).toInt();
        podData.system.platformEv = ini.value("Platform-Ev", 6).toInt();
        podData.system.ptePageSize = ini.value("PTE-PageSize", 8192).toInt();
        podData.system.threadCount = ini.value("ThreadCount", 4).toInt();
        QString tmpVal = ini.value("CpuCount", 4).toString();
        podData.system.processorCount = tmpVal.toInt();
        
        // Parse system type
        int sysTypeInt = ini.value("system_type_q8", 2).toInt();
        podData.system.sysType = static_cast<SystemType_EmulatR>(sysTypeInt);
        
        podData.system.cpuFrequencyHz = ini.value("CPU_FREQUENCY_HZ", 500000000).toULongLong();

        ini.endGroup();

        INFO_LOG_Fallback(QString("System: %1, %2 CPUs, %3 GB RAM")
            .arg(podData.system.hwModel)
            .arg(podData.system.processorCount)
            .arg(podData.system.memorySizeGB));
    }

    // ========================================================================
    // [Logging] Section
    // ========================================================================

	inline void loadLogging(QSettings& ini)
	{
		ini.beginGroup("Logging");

		podData.logging.enableDiskLogging = ini.value("EnableDiskLogging", true).toBool();
		podData.logging.enableConsole = ini.value("EnableConsole", true).toBool();
		podData.logging.logLevel = ini.value("logLevel", 0).toInt();
		podData.logging.logFileName = ini.value("logFileName", "logs/es40_instance.log").toString();
		podData.logging.maxLogFileSizeBytes = ini.value("MaxLogFileSizeBytes", 104857600).toULongLong();
		podData.logging.maxLogFileCount = ini.value("MaxLogFileCount", 10).toUInt();
		podData.logging.appendToExisting = (ini.value("AppendLog", "append").toString().toLower() == "append");
		podData.logging.enableTimestamps = ini.value("EnableTimestamps", true).toBool();
		podData.logging.useHighPerfTimestamps = ini.value("UseHighPerfTimestamps", true).toBool();
		podData.logging.logRegisterState = ini.value("logRegisterState", true).toBool();
		podData.logging.regEnableDiskLogging = ini.value("regEnableDiskLogging", true).toBool();
		podData.logging.regEnableConsole = ini.value("regEnableConsole", true).toBool();
        podData.logging.flushInterval = ini.value("FlushInterval", 10).toUInt();

		ini.endGroup();

		// Just convert to native separators, don't resolve path here
		podData.logging.logFileName = QDir::toNativeSeparators(podData.logging.logFileName);
	}

    // ========================================================================
    // [ExecTrace] Section
    // ========================================================================

    inline void loadExecTrace(QSettings& ini)
    {
        ini.beginGroup("ExecTrace");

        podData.execTrace.execTraceEnabled = ini.value("ExecTraceEnabled", false).toBool();
        podData.execTrace.immediateFlush = ini.value("ImmediateFlush", false).toBool();
        podData.execTrace.execTraceMode = ini.value("ExecTraceMode", "triggered").toString();
        podData.execTrace.traceFormat = ini.value("TraceFormat", "csv").toString();
        podData.execTrace.perCpuTraceFiles = ini.value("PerCpuTraceFiles", true).toBool();
        podData.execTrace.traceFilePattern = ini.value("TraceFilePattern", "traces/es40_instance.cpu{cpu}.trace").toString();
        podData.execTrace.maxTraceFileSizeBytes = ini.value("MaxTraceFileSizeBytes", 1073741824).toULongLong();
        podData.execTrace.maxTraceFileCount = ini.value("MaxTraceFileCount", 10).toUInt();
        podData.execTrace.traceRingRecordsPerCpu = ini.value("TraceRingRecordsPerCpu", 4096).toUInt();
        podData.execTrace.traceDumpPreRecords = ini.value("TraceDumpPreRecords", 32).toUInt();
        podData.execTrace.traceDumpPostRecords = ini.value("TraceDumpPostRecords", 32).toUInt();
        podData.execTrace.traceOutputDir = ini.value("TraceOutputDir", "./traces").toString();
        // Parse CPU mask (supports hex "0xF" or decimal "15")
        QString cpuMaskStr = ini.value("CpuMask", "0xF").toString();
        bool ok;
        podData.execTrace.cpuMask = cpuMaskStr.toUInt(&ok, 0xF);
        if (!ok) {
            WARN_LOG_Fallback(QString("Invalid CpuMask: %1, using 0xF").arg(cpuMaskStr));
            podData.execTrace.cpuMask = 0xF;
        }

        podData.execTrace.triggerOnException = ini.value("TriggerOnException", true).toBool();
        podData.execTrace.triggerOnIpi = ini.value("TriggerOnIpi", true).toBool();
        podData.execTrace.triggerOnPalEntry = ini.value("TriggerOnPalEntry", false).toBool();
        podData.execTrace.triggerOnPalExit = ini.value("TriggerOnPalExit", false).toBool();
        podData.execTrace.pcRangeEnabled = ini.value("PcRangeEnabled", false).toBool();
        podData.execTrace.pcRangeStart = ini.value("PcRangeStart", 0).toULongLong();
        podData.execTrace.pcRangeEnd = ini.value("PcRangeEnd", 0).toULongLong();
        podData.execTrace.includeIntRegWrites = ini.value("IncludeIntRegWrites", true).toBool();
        podData.execTrace.includeFpRegWrites = ini.value("IncludeFpRegWrites", false).toBool();
        podData.execTrace.includeIprWrites = ini.value("IncludeIprWrites", true).toBool();
        podData.execTrace.includeMemVA = ini.value("IncludeMemVA", true).toBool();
        podData.execTrace.includeMemPA = ini.value("IncludeMemPA", false).toBool();
        podData.execTrace.includeOpcodeWord = ini.value("IncludeOpcodeWord", true).toBool();
        podData.execTrace.flushIntervalMs = ini.value("FlushIntervalMs", 200).toUInt();
        podData.execTrace.fullTraceDebugBuildOnly = ini.value("FullTraceDebugBuildOnly", true).toBool();

        ini.endGroup();
    }

    // ========================================================================
    // [TLBShootdown] Section
    // ========================================================================

    inline void loadTLBShootdown(QSettings& ini)
    {
        ini.beginGroup("TLBShootdown");

        podData.tlbShootdown.enableACKs = ini.value("EnableACKs", false).toBool();
        podData.tlbShootdown.enablePreciseInvalidation = ini.value("EnablePreciseInvalidation", false).toBool();
        podData.tlbShootdown.enableShootdownLogging = ini.value("EnableShootdownLogging", false).toBool();
        podData.tlbShootdown.maxShootdownSeq = ini.value("MaxShootdownSeq", 255).toUInt();

        ini.endGroup();
    }

    // ========================================================================
    // [Interrupts] Section
    // ========================================================================

    inline void loadInterrupts(QSettings& ini)
    {
        ini.beginGroup("Interrupts");

        podData.interrupts.enableInterruptMaskingLog = ini.value("EnableInterruptMaskingLog", false).toBool();
        podData.interrupts.criticalInterruptVectors = ini.value("CriticalInterruptVectors", "").toString();

        ini.endGroup();
    }

    // ========================================================================
    // [FloatingPoint] Section
    // ========================================================================

    inline void loadFloatingPoint(QSettings& ini)
    {
        ini.beginGroup("FloatingPoint");

        podData.floatingPoint.useSSEForF_Float = ini.value("UseSSEForF_Float", 0).toBool();
        podData.floatingPoint.useSSEForG_Float = ini.value("UseSSEForG_Float", 0).toBool();
        podData.floatingPoint.useSSEForD_Float = ini.value("UseSSEForD_Float", 0).toBool();
        podData.floatingPoint.useSSEForS_Float = ini.value("UseSSEForS_Float", 1).toBool();
        podData.floatingPoint.useSSEForT_Float = ini.value("UseSSEForT_Float", 1).toBool();

        ini.endGroup();
    }

    // ========================================================================
    // [ROM] Section
    // ========================================================================

    inline void loadRom(QSettings& ini)
    {
        ini.beginGroup("ROM");
        podData.rom.srmIncRomFile = ini.value("SrmRomVariant", "ES45").toString();
        podData.rom.srmRomFile = ini.value("SrmRomFile", "").toString();
        podData.rom.hostProcessorModuleFirmwareFile = ini.value("HostProcessorModuleFirmwareFile", "").toString();
        podData.rom.pciBusModuleFirmWare = ini.value("PCIBusModuleFirmWare", "").toString();
        podData.rom.systemModuleFirmwareFile = ini.value("SystemModuleFirmwareFile", "").toString();
        podData.rom.intelHexLoaderFile = ini.value("IntelHexLoaderFile", "").toString();
        ini.endGroup();
    }



    // ========================================================================
    // [MemoryMap] Section
    // ========================================================================

    inline void loadMemoryMap(QSettings& ini)
    {
        ini.beginGroup("MemoryMap");

        podData.memoryMap.hwrpbBase = ini.value("HwrpbBase", "0x2000").toString().toULongLong(nullptr, 16);
        podData.memoryMap.hwrpbSize = ini.value("HwrpbSize", "0x4000").toString().toULongLong(nullptr, 16);
        podData.memoryMap.ramBase = ini.value("RamBase", "0x80000000").toString().toULongLong(nullptr, 16);

        podData.memoryMap.mmioBase = ini.value("MmioBase", "0xF0000000").toString().toULongLong(nullptr, 16);
        podData.memoryMap.mmioSize = ini.value("MmioSize", "0x10000000").toString().toULongLong(nullptr, 16);


        // *** PATTERN FOR "auto" SUPPORT ***
        QString pciMemBaseStr = ini.value("PciMemBase", "auto").toString().toLower();
        if (pciMemBaseStr == "auto") {
            podData.memoryMap.pciMemBase = 0;  // 0 = sentinel for auto-calculate
            INFO_LOG_Fallback("PciMemBase: auto (will calculate above RAM)");
        }
        else {
            podData.memoryMap.pciMemBase = pciMemBaseStr.toULongLong(nullptr, 16);
            INFO_LOG_Fallback(QString("PciMemBase: 0x%1 (manual)")
                .arg(podData.memoryMap.pciMemBase, 16, 16, QChar('0')));
        }

        podData.memoryMap.pciMemBase = ini.value("PciMemBase", "0x200000000").toString().toULongLong(nullptr, 16);
        podData.memoryMap.pciMemSize = ini.value("PciMemSize", "0x100000000").toString().toULongLong(nullptr, 16);

        ini.endGroup();

        
    }
    // ========================================================================
    // [CACHE/*] Sections - Load as Structs
    // ========================================================================

    inline void loadCaches(QSettings& ini)
    {
        QStringList cacheGroups = { "CACHE/l1", "CACHE/L2", "CACHE/L3" };

        for (const QString& cacheGroup : cacheGroups) {
            if (!ini.childGroups().contains(cacheGroup)) {
                continue;
            }

            ini.beginGroup(cacheGroup);

            CacheConfig cache;
            cache.numSets = ini.value("NumSets", 256).toInt();
            cache.associativity = ini.value("Associativity", 2).toInt();
            cache.lineSize = ini.value("LineSize", 64).toInt();
            cache.totalSize = ini.value("TotalSize", 0).toInt();
            cache.enablePrefetch = ini.value("EnablePrefetch", true).toBool();
            cache.enableStatistics = ini.value("EnableStatistics", true).toBool();
            cache.enableCoherency = ini.value("EnableCoherency", true).toBool();
            cache.coherencyProtocol = ini.value("CoherencyProtocol", "MESI").toString();
            cache.statusUpdateInterval = ini.value("StatusUpdateInterval", 1000).toInt();
            cache.replacementPolicy = ini.value("ReplacementPolicy", "MRU").toString();
            cache.evictionThreshold = ini.value("EvictionThreshold", 1000).toInt();
            cache.cacheSize = ini.value("CacheSize", 48).toInt();

            ini.endGroup();

            podData.caches[cacheGroup] = cache;
        }

        INFO_LOG_Fallback(QString("Loaded %1 cache configurations").arg(podData.caches.size()));
    }

    // ========================================================================
    // [Device.*] Sections - FLATTENED STRUCTURE
    // ========================================================================

    inline void loadDevicesAndControllers(QSettings& ini)
    {
        QStringList allGroups = ini.childGroups();

        for (const QString& group : allGroups) {
            if (!group.startsWith("Device.")) {
                continue;
            }

            QString deviceName = group.mid(7);  // Remove "Device." prefix

            // Skip OPA consoles (handled separately)
            if (deviceName.startsWith("OPA")) {
                continue;
            }

            ini.beginGroup(group);

            QString classType = ini.value("classType", "").toString();
            
            if (classType.isEmpty()) {
                WARN_LOG_Fallback(QString("Device %1 has no classType, skipping").arg(deviceName));
                ini.endGroup();
                continue;
            }

            QString parent = ini.value("parent", "").toString();

            // Read all keys into fields map
            QMap<QString, QVariant> fields;
            QStringList keys = ini.childKeys();
            for (const QString& key : keys) {
                fields[key] = ini.value(key);
            }

            ini.endGroup();

            // Categorize: Controller (no parent) vs Device (has parent)
            if (parent.isEmpty()) {
                // This is a controller
                ControllerConfig ctrl;
                ctrl.name = deviceName;
                ctrl.classType = classType;
                ctrl.fields = fields;

                podData.controllers[deviceName] = ctrl;

                INFO_LOG_Fallback(QString("Loaded controller: %1 (%2)")
                    .arg(deviceName).arg(classType));

            } else {
                // This is a device
                DeviceConfig device;
                device.name = deviceName;
                device.classType = classType;
                device.parent = parent;
                device.fields = fields;

                podData.devices[deviceName] = device;

                INFO_LOG_Fallback(QString("Loaded device: %1 (%2) parent=%3")
                    .arg(deviceName).arg(classType).arg(parent));
            }
        }
    }



    // ========================================================================
    // [Device.OPA*] Sections - Console Devices
    // ========================================================================

    inline void loadOPAConsoles(QSettings& ini)
    {
        QStringList allGroups = ini.childGroups();
        QRegularExpression rxOPA(R"(^Device\.(OPA\d+)$)");

        for (const QString& group : allGroups) {
            auto match = rxOPA.match(group);
            if (!match.hasMatch()) {
                continue;
            }

            QString consoleName = match.captured(1);

            ini.beginGroup(group);

            OPAConsoleConfig console;
            console.name = ini.value("name", consoleName).toString();
            console.classType = ini.value("classType", "UART").toString();
            console.location = ini.value("location", "cab0/drw0").toString();
            console.iface = ini.value("iface", "Net").toString();
            console.ifacePort = ini.value("iface_port", 5555).toUInt();
            console.application = ini.value("application", "").toString();
            console.rxBufferSize = ini.value("rx_buffer_size", 256).toUInt();
            console.txBufferSize = ini.value("tx_buffer_size", 1024).toUInt();
            console.dropOnOverflow = ini.value("drop_on_overflow", true).toBool();
            console.autoReconnect = ini.value("auto_reconnect", true).toBool();

            ini.endGroup();

            podData.opaConsoles[consoleName] = console;

            INFO_LOG_Fallback(QString("Loaded console: %1 (port %2)")
                .arg(consoleName).arg(console.ifacePort));
        }
    }

    // ========================================================================
    // HELPER ACCESSORS
    // ========================================================================

    inline const CacheConfig* cache(const QString& level) const
    {
        auto it = podData.caches.find(level);
        return (it == podData.caches.end() ? nullptr : &(*it));
    }

    inline const CacheConfig& requireCache(const QString& level) const
    {
        auto it = podData.caches.find(level);
        if (it == podData.caches.end()) {
            CRITICAL_LOG_Fallback(QString("Missing required cache level: %1").arg(level));
        }
        return *it;
    }

    inline const ControllerConfig* controller(const QString& name) const
    {
        auto it = podData.controllers.find(name);
        return (it == podData.controllers.end() ? nullptr : &(*it));
    }

    inline const DeviceConfig* device(const QString& name) const
    {
        auto it = podData.devices.find(name);
        return (it == podData.devices.end() ? nullptr : &(*it));
    }

    inline const OPAConsoleConfig* console(const QString& name) const
    {
        auto it = podData.opaConsoles.find(name);
        return (it == podData.opaConsoles.end() ? nullptr : &(*it));
    }

   static quint64 readMemorySize(const QString& key, quint64 defaultValue) noexcept
    {
        QSettings settings("eNVy", "ASA_EmulatR");

        // Try to read as string first (supports units like "32GB", "2MB")
        QString sizeStr = settings.value(key).toString();

        if (sizeStr.isEmpty()) {
            // Fall back to direct numeric value
            bool ok = false;
            quint64 value = settings.value(key, defaultValue).toULongLong(&ok);
            return ok ? value : defaultValue;
        }

        // Parse size string (e.g., "32GB", "2MB", "0x800000000")
        sizeStr = sizeStr.trimmed().toUpper();

        // Check for hex format (0x...)
        if (sizeStr.startsWith("0X")) {
            bool ok = false;
            quint64 value = sizeStr.mid(2).toULongLong(&ok, 16);
            if (ok) return value;
        }

        // Extract numeric part and unit
        quint64 multiplier = 1;
        QString numPart;

        if (sizeStr.endsWith("GB")) {
            multiplier = 1024ULL * 1024ULL * 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 2);
        }
        else if (sizeStr.endsWith("MB")) {
            multiplier = 1024ULL * 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 2);
        }
        else if (sizeStr.endsWith("KB")) {
            multiplier = 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 2);
        }
        else if (sizeStr.endsWith("G")) {
            multiplier = 1024ULL * 1024ULL * 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 1);
        }
        else if (sizeStr.endsWith("M")) {
            multiplier = 1024ULL * 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 1);
        }
        else if (sizeStr.endsWith("K")) {
            multiplier = 1024ULL;
            numPart = sizeStr.left(sizeStr.length() - 1);
        }
        else {
            // No unit, assume bytes
            numPart = sizeStr;
        }

        // Parse numeric part
        bool ok = false;
        quint64 value = numPart.toULongLong(&ok);

        if (!ok) {
            WARN_LOG(QString("Failed to parse memory size '%1', using default %2")
                .arg(sizeStr)
                .arg(defaultValue));
            return defaultValue;
        }

        return (value * multiplier);
    }

    // ========================================================================
    // Configuration File Management
    // ========================================================================

    /**
     * @brief Load settings from configuration file
     * Uses EmulatorPaths to locate config/emulator.ini
     * Creates default config if it doesn't exist
     * @return true if loaded successfully (or defaults used)
     */
   AXP_ALWAYS_INLINE bool load() noexcept
   {
       // Get config file path from EmulatorPaths
       EmulatorPaths paths;
       QString configPath = paths.getConfigPath("ASAEmulatr.ini");

       QFileInfo configInfo(configPath);

       if (!configInfo.exists()) {
           // Config doesn't exist - use defaults and save
           qDebug() << "EmulatorSettings: Config file not found, creating defaults at" << configPath;

           // Defaults are already set in POD initialization
           // Just save them to file
           return saveToFile(configPath);
       }

       // Load existing config
       return loadFromFile(configPath);
   }

   /**
    * @brief Save current settings to configuration file
    * @return true if saved successfully
    */
   AXP_ALWAYS_INLINE bool save() noexcept
   {
       EmulatorPaths paths;
       QString configPath = paths.getConfigPath("ASAEmulatr.ini");
       return saveToFile(configPath);
   }

   /**
    * @brief Get the configuration file path
    */
   AXP_ALWAYS_INLINE QString getConfigFilePath() const noexcept
   {
       EmulatorPaths paths;
       return paths.getConfigPath("ASAEmulatr.ini");
   }

private:

    /**
     * @brief Load settings from specific file
     */
    AXP_ALWAYS_INLINE bool loadFromFile(const QString& filePath) noexcept
    {
        QSettings settings(filePath, QSettings::IniFormat);

        if (settings.status() != QSettings::NoError) {
            qWarning() << "EmulatorSettings: Failed to read" << filePath << "- using defaults";
            return false;
        }

        settings.beginGroup("System");
        QString tmpVal = settings.value("CpuCount", "4").toString();
        // System settings
    
        podData.system.processorCount = tmpVal.toInt();
        podData.system.sysType =static_cast<SystemType_EmulatR>( settings.value("systemType", 2).toUInt());
        podData.system.memorySizeGB = settings.value("ramSizeGB", 32).toUInt();
        settings.endGroup();

        // Memory map settings
        settings.beginGroup("MemoryMap");
        podData.memoryMap.ramBase = parseHexValue(settings, "ramBase", "0x80000000");
        podData.memoryMap.mmioBase = parseHexValue(settings, "mmioBase", "0x1000000000");
        settings.endGroup();

        // Logging settings
        settings.beginGroup("Logging");
        podData.logging.enableDiskLogging = settings.value("enableDiskLogging", true).toBool();
        podData.logging.enableConsole = settings.value("enableConsole", true).toBool();
        podData.logging.logFileName = settings.value("logFileName", "es40_instance.log").toString();
        podData.logging.logLevel = settings.value("logLevel", LOG_INFO).toInt();
        podData.logging.maxLogFileSizeBytes = settings.value("maxLogFileSizeBytes", 104857600).toULongLong();
        podData.logging.maxLogFileCount = settings.value("maxLogFileCount", 10).toUInt();
        podData.logging.appendToExisting = settings.value("appendToExisting", false).toBool();
        podData.logging.enableTimestamps = settings.value("enableTimestamps", true).toBool();
        podData.logging.useHighPerfTimestamps = settings.value("useHighPerfTimestamps", true).toBool();
        settings.endGroup();

        qDebug() << "EmulatorSettings: Loaded from" << filePath;
        return true;
    }

    /**
     * @brief Save settings to specific file
     */
    AXP_ALWAYS_INLINE bool saveToFile(const QString& filePath) noexcept
    {
        QSettings settings(filePath, QSettings::IniFormat);

        QString tmpVal = QString::number(podData.system.processorCount);
        // System settings
        settings.beginGroup("System");
        settings.setValue("CpuCount", tmpVal);
        settings.setValue("systemType", static_cast<quint16>(podData.system.sysType));
        settings.setValue("ramSizeGB", podData.system.memorySizeGB);
        settings.endGroup();

        // Memory map settings
        settings.beginGroup("MemoryMap");
        settings.setValue("ramBase", QString("0x%1").arg(podData.memoryMap.ramBase, 0, 16));
        settings.setValue("mmioBase", QString("0x%1").arg(podData.memoryMap.mmioBase, 0, 16));
        settings.endGroup();

        // Logging settings
        settings.beginGroup("Logging");
        settings.setValue("enableDiskLogging", podData.logging.enableDiskLogging);
        settings.setValue("enableConsole", podData.logging.enableConsole);
        settings.setValue("logFileName", podData.logging.logFileName);
        settings.setValue("logLevel", podData.logging.logLevel);
        settings.setValue("maxLogFileSizeBytes", podData.logging.maxLogFileSizeBytes);
        settings.setValue("maxLogFileCount", podData.logging.maxLogFileCount);
        settings.setValue("appendToExisting", podData.logging.appendToExisting);
        settings.setValue("enableTimestamps", podData.logging.enableTimestamps);
        settings.setValue("useHighPerfTimestamps", podData.logging.useHighPerfTimestamps);
        settings.endGroup();

        settings.sync();

        if (settings.status() != QSettings::NoError) {
            qWarning() << "EmulatorSettings: Failed to save to" << filePath;
            return false;
        }

        qDebug() << "EmulatorSettings: Saved to" << filePath;
        return true;
    }

    /**
     * @brief Parse hex value from QSettings
     */
    AXP_ALWAYS_INLINE quint64 parseHexValue(QSettings& settings, const QString& key, const QString& defaultValue) const noexcept
    {
        QString value = settings.value(key, defaultValue).toString();
        bool ok = false;

        // Remove 0x prefix if present
        if (value.startsWith("0x", Qt::CaseInsensitive)) {
            value = value.mid(2);
        }

        quint64 result = value.toULongLong(&ok, 16);

        if (!ok) {
            qWarning() << "EmulatorSettings: Failed to parse hex value for" << key << "- using default";
            quint64 defaultResult = defaultValue.mid(2).toULongLong(nullptr, 16);
            return defaultResult;
        }

        return result;
    }


};

// ============================================================================
// Fallback Logging (used before EventLog is initialized)
// ============================================================================

inline void INFO_LOG_Fallback(const QString& msg)
{
    qInfo().noquote() << "[INFO ]" << msg;
}

inline void WARN_LOG_Fallback(const QString& msg)
{
    qWarning().noquote() << "[WARN ]" << msg;
}

inline void CRITICAL_LOG_Fallback(const QString& msg) 
{
    qCritical().noquote() << "[CRIT ]" << msg;
    qFatal("%s", qPrintable(msg));
}

#endif // EmulatorSettingsInline_h__
