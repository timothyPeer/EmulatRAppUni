// ============================================================================
// EmulatR_init.h - Phase 1: System initialization.
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

#ifndef EMULATR_INIT_H
#define EMULATR_INIT_H
// ============================================================================
//  ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
//  Non-Commercial Use Only.
// ============================================================================
//
//  EmulatR_init.h
//
//  Purpose: Hidden initialization subsystem for ASA Emulator.
//           Manages global subsystem lifecycle with QScopedPointer and
//           wires to Meyer's Singleton accessors.
//
//  Design Pattern:
//    1. QScopedPointer for ownership and automatic cleanup
//    2. Meyer's Singleton for global access via global_*() functions
//    3. Clean separation: main() only tracks, doesn't initialize
//
//  Usage in main():
//    auto& init = global_EmulatR_init();
//    init.initializeSystem();
//    init.initializeDeviceInterrupts();
//
// ============================================================================



#include <QtGlobal>
#include <QString>
#include <QDir>
#include <QFile>
#include <QScopedPointer>

#include "configLib/EmulatorConfig.h"

#include "configLib/EmulatorSettingsInline.h"
#include "EmulatorPaths.h"
#include "SubsystemCoordinator.h"
#include "coreLib/Axp_Attributes_core.h"

#include "memoryLib/global_GuestMemory.h"
#include "configLib/global_EmulatorSettings.h"
#include "memoryLib/GuestMemory.h"
#include "memoryLib/SrmRomLoader.h"

// Forward decarations
class SafeMemory;
struct PlatformAddressMap;
class IRQController;

class InitializationVerifier {
public:
	enum class InitStatus { NotInitialized, Initializing, Initialized, Failed };

	[[nodiscard]] explicit InitializationVerifier()
	{
	}

	static bool markInitializing(const QString& subsystem) noexcept {
		QMutexLocker lock(&s_mutex);
		if (s_status.contains(subsystem) && s_status[subsystem] != InitStatus::NotInitialized) {
			CRITICAL_LOG(QString("INITIALIZATION VIOLATION: %1 already initialized/initializing").arg(subsystem));
			return false;
		}
		s_status[subsystem] = InitStatus::Initializing;
		DEBUG_LOG(QString("Initializing: %1").arg(subsystem));
		return true;
	}

	static void markInitialized(const QString& subsystem) noexcept {
		QMutexLocker lock(&s_mutex);
		s_status[subsystem] = InitStatus::Initialized;
		s_initOrder.append(subsystem);
		INFO_LOG(QString("Initialized: %1 (order: %2)").arg(subsystem).arg(s_initOrder.size()));
	}

	static void markFailed(const QString& subsystem) noexcept {
		QMutexLocker lock(&s_mutex);
		s_status[subsystem] = InitStatus::Failed;
		ERROR_LOG(QString("FAILED: %1").arg(subsystem));
	}

	static bool isInitialized(const QString& subsystem) noexcept {
		QMutexLocker lock(&s_mutex);
		return s_status.value(subsystem) == InitStatus::Initialized;
	}

	static QString getInitializationReport() noexcept {
		QMutexLocker lock(&s_mutex);
		QString report = "=== INITIALIZATION REPORT ===\n";
		report += QString("Total subsystems: %1\n").arg(s_status.size());

		int initialized = 0;
		for (auto it = s_status.begin(); it != s_status.end(); ++it) {
			QString status;
			switch (it.value()) {
			case InitStatus::NotInitialized: status = "NOT_INIT"; break;
			case InitStatus::Initializing: status = "PENDING"; break;
			case InitStatus::Initialized: status = "OK"; initialized++; break;
			case InitStatus::Failed: status = "FAILED"; break;
			}
			report += QString("  %-25s: %1\n").arg(it.key()).arg(status);
		}

		report += QString("\nInitialization Order:\n");
		for (int i = 0; i < s_initOrder.size(); ++i) {
			report += QString("  %1. %2\n").arg(i + 1, 2).arg(s_initOrder[i]);
		}

		report += QString("\nSummary: %1/%2 subsystems initialized\n").arg(initialized).arg(s_status.size());
		return report;
	}

	static void reset() noexcept {
		QMutexLocker lock(&s_mutex);
		s_status.clear();
		s_initOrder.clear();
	}

private:
	inline static QMap<QString, InitStatus> s_status;
	inline static QStringList s_initOrder;
	inline static QMutex s_mutex;


    // private helpers. 
        

};
class ConsoleManager;
class ReservationManager;



// Static member definitions
// QMap<QString, InitializationVerifier::InitStatus> InitializationVerifier::s_status;
// QStringList InitializationVerifier::s_initOrder;
// QMutex InitializationVerifier::s_mutex;

// ============================================================================
// EMULATOR INITIALIZATION SUBSYSTEM
// ============================================================================

class EmulatR_init final {

    ConsoleManager* m_consoleManager;  // 
	SrmRomLoader             m_srmRomLoader;

	/*	friend EmulatR_init& global_EmulatR_init() noexcept;*/
public:

	~EmulatR_init();
    EmulatR_init(SubsystemCoordinator* coordinator);

    // ========================================================================
    // MEYER'S SINGLETON ACCESSOR
    // ========================================================================
    static EmulatR_init& instance(SubsystemCoordinator()) noexcept;


    // ========================================================================
    // INITIALIZATION API
    // ========================================================================
    AXP_HOT AXP_FLATTEN auto initializePhase8_5_PalHandlers() -> bool;
	bool                     initialize();
    /**
     * @brief Phase 1: System initialization.
     *
     * Loads configuration, creates subsystems, wires to global accessors.
     *
     * @param configPath Path to configuration directory
     * @return true if successful, false on error
     */
 
    AXP_HOT AXP_FLATTEN bool initializeSystem() noexcept;

	/**
     * @brief Clean shutdown of all subsystems.
     */
	AXP_HOT AXP_FLATTEN void shutdown()  noexcept;
	AXP_HOT AXP_FLATTEN void shutdownConsoles();
    


	// ========================================================================
    // ACCESSORS (for wiring to global_*() functions)
    // ========================================================================


    SafeMemory* safeMemory() const noexcept { return m_coordinator->safeMemory(); }
    GuestMemory* guestMemory() const noexcept { return m_coordinator->guestMemory(); }

    quint16 cpuCount() const noexcept { return m_cpuCount; }
    quint64 memorySizeGB() const noexcept { return m_memorySizeGB; }
    quint64 memorySizeBytes() const noexcept { return m_memorySizeBytes; }

private:

    /**
   * @brief Phase 2: Device interrupt registration.
   *
   * Registers interrupt vectors for all emulated devices.
   */
    AXP_HOT AXP_FLATTEN   void initializeDeviceInterrupts() noexcept;

    /**
     * @brief Phase 3: Initialize device emulators (future expansion).
     */
    AXP_HOT AXP_FLATTEN void initializeDevices() noexcept;


    AXP_HOT AXP_FLATTEN void initializeLogging() noexcept;

    // ============================================================================
    // Initialization Phases (20 phases)
    // ============================================================================


    AXP_HOT AXP_FLATTEN bool initializePhase0_Bootstrap();
    AXP_HOT AXP_FLATTEN bool initializePhase1_Logging();
    AXP_HOT AXP_FLATTEN bool initializePhase1_2_ExecTrace();
	bool                     initializePhase9_5_InstructionSet();
    /*   AXP_HOT AXP_FLATTEN bool validateConfiguration() noexcept;*/
    AXP_HOT AXP_FLATTEN bool initializePhase2_Configuration();
    AXP_HOT AXP_FLATTEN bool initializePhase3_PlatformIdentity();
    AXP_HOT AXP_FLATTEN bool initializePhase4_MemorySystem() ;           // SRM Support 20260113
    AXP_HOT AXP_FLATTEN bool initializePhase5_5_TLBSystem() const;
    AXP_HOT AXP_FLATTEN bool initializePhase6_ReservationSystem() const;
    AXP_HOT AXP_FLATTEN bool initializePhase7_ExceptionInfrastructure() const;
    AXP_HOT AXP_FLATTEN bool initializePhase7_5_DeviceTree();
    AXP_HOT AXP_FLATTEN bool initializePhase8_PALInfrastructure() const;
	bool                     initializePhase8_5_CallPalHandlers();
	AXP_HOT AXP_FLATTEN bool initializePhase9_InstructionSystem();
    AXP_HOT AXP_FLATTEN bool initializePhase10_DeviceInfrastructure();
    AXP_HOT AXP_FLATTEN bool initializePhase5_FirmwareLoading();
    AXP_HOT AXP_FLATTEN bool initializePhase11_CoordinationLayer();
	AXP_HOT AXP_FLATTEN bool initializePhase13_InitializeConsole();
	AXP_HOT AXP_FLATTEN bool initializePhase13_ConsoleEnvironment() const;
    AXP_HOT AXP_FLATTEN bool initializePhase14_CPUBringUp();
	AXP_HOT AXP_FLATTEN bool initializePhase15_FinalVerification();

    AXP_HOT AXP_FLATTEN bool validateConfiguration() noexcept;
    AXP_HOT AXP_FLATTEN void dumpMemoryMap();
private:
    // ========================================================================
    // SINGLETON ENFORCEMENT
    // ========================================================================
	
    Q_DISABLE_COPY(EmulatR_init)


    // ========================================================================
    // VALIDATION
    // ========================================================================

    // ========================================================================
    // STATE
    // ========================================================================

    EmulatorPaths m_emulatorPaths;                           // 20260113
    EmulatorConfig m_emulatorConfig;
    quint16  m_cpuCount{ 1 };
    quint64  m_memorySizeGB{ 4 };
    quint64  m_memorySizeBytes{ 4ULL * GB };
    SystemType_EmulatR m_sysType{ SystemType_EmulatR::ES40 };       // 20260113
    QString m_currentPhase;

private:
	// Helper method that MUST be called inside function scope
	bool beginInitialization(const QString& subsystemName);

	void markSuccess();

	void markFailure();

    EmulatorSettingsInline m_configSettings; 
    SubsystemCoordinator* m_coordinator;



	

};


#endif // EMULATR_INIT_H
