// ============================================================================
// main.cpp - ========================================================================
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

#include "emulatrLib/EmulatorPaths.h"
#include "coreLib/EventLog.h"
#include "emulatrLib/global_EmulatR_init.h"
#include "coreLib/LoggingMacros.h"
#include "configLib/global_EmulatorSettings.h"
#include "emulatrLib/EmulatR_init.h"      

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	// ========================================================================
   // PHASE 0: Critical Infrastructure (BEFORE anything else)
   // ========================================================================

	EmulatorPaths emuPaths;
   // Step 1: Initialize path management
	EmulatorPaths& paths = emuPaths;  // or local instance
	paths.createDirectories();  // Ensure all directories exist

	// Step 2: Initialize logging IMMEDIATELY (captures everything from here on)
	if (!EventLog::initialize()) {
		qCritical() << "FATAL: Failed to initialize EventLog";
		return 1;
	}

	qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& msg) {
		// Also send to EventLog
		switch (type) {
		case QtDebugMsg:   EventLog::write(LOG_DEBUG, msg); break;
		case QtInfoMsg:    EventLog::write(LOG_INFO, msg); break;
		case QtWarningMsg: EventLog::write(LOG_WARN, msg); break;
		case QtCriticalMsg:
		case QtFatalMsg:   EventLog::write(LOG_CRITICAL, msg); break;
		}

		// 2. ALSO write to console (stdout/stderr)
		QTextStream console(type >= QtCriticalMsg ? stderr : stdout);

		QString prefix;
		switch (type) {
		case QtDebugMsg:    prefix = "[DEBUG]"; break;
		case QtInfoMsg:     prefix = "[INFO ]"; break;
		case QtWarningMsg:  prefix = "[WARN ]"; break;
		case QtCriticalMsg: prefix = "[ERROR]"; break;
		case QtFatalMsg:    prefix = "[FATAL]"; break;
		}

		console << prefix << " " << msg << Qt::endl;

		});

	qInfo() << "=== ASA-EMulatR Starting ===";

	INFO_LOG("==========================================================");
	INFO_LOG("ASA EmulatR Starting...");
	INFO_LOG("==========================================================");
	INFO_LOG(QString("Executable: %1").arg(paths.getBinDir()));
	INFO_LOG(QString("Config Dir: %1").arg(paths.getConfigDir()));
	INFO_LOG(QString("Logs Dir:   %1").arg(paths.getLogsDir()));
	INFO_LOG(QString("Firmware:   %1").arg(paths.getFirmwareDir()));



	// Step 3: Load settings (now we can log if it fails)
	auto& settings = global_EmulatorSettings();
	if (!settings.load()) {
		ERROR_LOG("Failed to load settings, using defaults");
	}
	else {
		INFO_LOG("Settings loaded successfully");
	}

	// ========================================================================
	// STEP 3: Initialize Emulator Subsystems
	//         (this will load config THEN initialize EventLog in correct order)
	// ========================================================================

	EmulatR_init& init = global_EmulatR_init();

	if (!init.initializeSystem()) {
		qCritical() << "ASAEmulatR initialization failed";
		return 1;
	}

	// Now EventLog is initialized and we can use INFO_LOG, etc.
	INFO_LOG("============================================================");
	INFO_LOG("ASAEmulatR ready - entering event loop");
	INFO_LOG("============================================================");

	// ========================================================================
	// STEP 4: Run Application Event Loop
	// ========================================================================

	int result = app.exec();

	// ========================================================================
	// STEP 5: Shutdown
	// ========================================================================

	INFO_LOG("");
	INFO_LOG("============================================================");
	INFO_LOG("Shutting down...");
	INFO_LOG("============================================================");

	init.shutdown();
	EventLog::shutdown();

	return result;
}
