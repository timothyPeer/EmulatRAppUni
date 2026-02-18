#ifndef SUBSYSTEMCOORDINATOR_H
#define SUBSYSTEMCOORDINATOR_H
// ============================================================================
// SubsystemCoordinator.h - Header-Only Infrastructure Manager
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Infrastructure subsystem coordinator - owns and manages all hardware
//   subsystems (memory, I/O, IRQ, devices). Header-only implementation.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================



#include <memory>
#include <QtGlobal>
#include <QString>

#include "IPIManager.h"
#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/LoggingMacros.h"
#include "memoryLib/GuestMemory.h"
#include "mmioLib/mmio_Manager.h"
#include "grainFactoryLib/GrainResolver.h"
#include "palLib_EV6/PalVectorTable_final.h"
#include "memoryLib/WriteBufferManager.h"
#include "memoryLib/SafeMemory.h"
#include "controllersLib/ScsiController.h"
#include "configLib/global_EmulatorSettings.h"
#include "deviceLib/ConsoleManager.h"

// ============================================================================
// SubsystemCoordinator - Infrastructure Ownership
// ============================================================================

class SubsystemCoordinator final {
public:
    // ====================================================================
    // Constructor (Header-Only)
    // ====================================================================
    AXP_HOT AXP_ALWAYS_INLINE SubsystemCoordinator() noexcept
    {
        DEBUG_LOG("SubsystemCoordinator: Constructing subsystems");

        // Get CPU count from settings
        auto& settings = global_EmulatorSettings();
        m_cpuCount = settings.podData.system.processorCount;
        if (m_cpuCount < 1 || m_cpuCount > MAX_CPUS) {
            WARN_LOG(QString("Invalid CPU count %1, using default 4").arg(m_cpuCount));
            m_cpuCount = 4;
        }

        // Construct subsystems in dependency order
        m_safeMemory = std::make_unique<SafeMemory>();
        m_guestMemory = std::make_unique<GuestMemory>();
        m_mmio = std::make_unique<MMIOManager>();
        m_grainResolver = std::make_unique<GrainResolver>();
        m_writeBufferMgr = std::make_unique<WriteBufferManager>(m_cpuCount);
        m_scsiCtrl = std::make_unique<ScsiController>();
        m_ipiManager = std::make_unique<IPIManager>();
        m_consoleManager = std::make_unique<ConsoleManager>();
        DEBUG_LOG("SubsystemCoordinator: IPIManager initialized");

        // SRM firmware region (PA-agnostic, will be loaded during initialization)


        createMemorySubsystems();
        initializeAllSubsystems();
    }

   Q_DISABLE_COPY(SubsystemCoordinator)

        // In SubsystemCoordinator.h

public:
    // Add after constructor
    AXP_HOT AXP_ALWAYS_INLINE void shutdown() const noexcept
    {
        INFO_LOG("SubsystemCoordinator: Beginning shutdown sequence");

        // Shutdown in REVERSE order of initialization

  // 1. Flush ALL pending writes FIRST
        if (m_writeBufferMgr && m_guestMemory) {
            INFO_LOG("Flushing all pending writes...");

            m_writeBufferMgr->flushAllBuffers(
                [this](CPUIdType cpuId, const WriteBufferEntry& entry) {
                    // Commit write to memory based on size
                    MEM_STATUS status;

                    switch (entry.bufferSize) {
                    case 1:
                        status = m_guestMemory->write8(entry.address, static_cast<quint8>(entry.bufferData));
                        break;
                    case 2:
                        status = m_guestMemory->write16(entry.address, static_cast<quint16>(entry.bufferData));
                        break;
                    case 4:
                        status = m_guestMemory->write32(entry.address, static_cast<quint32>(entry.bufferData));
                        break;
                    case 8:
                        status = m_guestMemory->write64(entry.address, entry.bufferData);
                        break;
                    default:
                        ERROR_LOG(QString("CPU%1: Invalid write buffer size %2").arg(cpuId).arg(entry.bufferSize));
                        return;
                    }

                    if (status != MEM_STATUS::Ok) {
                        WARN_LOG(QString("CPU%1: Failed to commit buffered write PA=0x%2 size=%3 status=%4")
                            .arg(cpuId)
                            .arg(entry.address, 16, 16, QChar('0'))
                            .arg(entry.bufferSize)
                            .arg(static_cast<int>(status)));
                    }
                }
            );
        }

        // 2. Close console connections
        if (m_consoleManager) {
            DEBUG_LOG("Closing console connections...");
            m_consoleManager->shutdown();
        }

        // 3. Stop IPI processing
        if (m_ipiManager) {
            DEBUG_LOG("Stopping IPI manager...");
            // m_ipiManager->stop();
        }

        // 4. Release SCSI resources
        if (m_scsiCtrl) {
            DEBUG_LOG("Releasing SCSI controller...");
            // m_scsiCtrl->shutdown();
        }

        // 5. Detach memory subsystems from GuestMemory
        if (m_guestMemory) {
            DEBUG_LOG("Detaching memory subsystems...");
            m_guestMemory->attachSubsystems( nullptr, nullptr);
        }

        // 6. unique_ptr destructor handles rest in reverse member order:
        //    - m_srmFirmware
        //    - m_ipiManager  
        //    - m_scsiCtrl
        //    - m_safeMemory
        //    - m_writeBufferMgr
        //    - m_grainResolver
        //    - m_irqController
        //    - m_mmio
        //    - m_guestMemory

        INFO_LOG("SubsystemCoordinator: Shutdown complete");
    }

    ~SubsystemCoordinator() {
        shutdown();
    }
        // ====================================================================
        // Subsystem Accessors (Inline)
        // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE ConsoleManager* consoleManager() const noexcept {
        return m_consoleManager.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE GuestMemory* guestMemory() const noexcept
    {
        return m_guestMemory.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE MMIOManager* mmioManager() const noexcept
    {
        return m_mmio.get();
    }



    AXP_HOT AXP_ALWAYS_INLINE GrainResolver* grainResolver() const noexcept
    {
        return m_grainResolver.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE WriteBufferManager* writeBufferManager() const noexcept
    {
        return m_writeBufferMgr.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE SafeMemory* safeMemory() const noexcept
    {
        return m_safeMemory.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE ScsiController* scsiController() const noexcept
    {
        return m_scsiCtrl.get();
    }

    AXP_HOT AXP_ALWAYS_INLINE IPIManager* ipiManager() const noexcept
    {
        return m_ipiManager.get();
    }

    // ====================================================================
    // Initialization (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE bool initializeAllSubsystems() const noexcept
    {
        DEBUG_LOG("SubsystemCoordinator: Initializing all subsystems");

        {
            PalVectorTable::instance().initialize();
            DEBUG_LOG("SubsystemCoordinator: PAL vector table initialized");
        }

        // Additional subsystem initializations as needed

        if (areAllSubsystemsBound()) {
            INFO_LOG("SubsystemCoordinator: All critical subsystems initialized");
        }
        else {
            WARN_LOG("SubsystemCoordinator: Some subsystems failed to initialize");
            DEBUG_LOG(getSubsystemStatus());
        }
      

        return true;
    }



    // ====================================================================
    // Diagnostics (Inline)
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE bool areAllSubsystemsBound() const noexcept
    {
        return (m_guestMemory != nullptr) &&
            (m_mmio != nullptr) &&
            (m_grainResolver != nullptr) &&
            (m_writeBufferMgr != nullptr) &&
            (m_safeMemory != nullptr) &&
            (m_consoleManager != nullptr) 
            ;
    }

    AXP_HOT AXP_ALWAYS_INLINE QString getSubsystemStatus() const noexcept
    {
        QString status;
        status += "SubsystemCoordinator Status:\n";
        status += QString("  CPU Count: %1\n").arg(m_cpuCount);
        status += QString("  GuestMemory: %1\n").arg(m_guestMemory ? "OK" : "NULL");
        status += QString("  MMIOManager: %1\n").arg(m_mmio ? "OK" : "NULL");
        
        status += QString("  GrainResolver: %1\n").arg(m_grainResolver ? "OK" : "NULL");
        status += QString("  PALVectorTable: OK (singleton)");
        status += QString("  WriteBufferManager: %1\n").arg(m_writeBufferMgr ? "OK" : "NULL");
        status += QString("  SafeMemory: %1\n").arg(m_safeMemory ? "OK" : "NULL");
        status += QString("  MemoryBarrierCoordinator: OK (singleton)");
        status += QString("  ScsiController: %1\n").arg(m_scsiCtrl ? "OK" : "NULL");
        status += QString("  ConsoleManager: %1\n").arg(m_consoleManager ? "OK" : "NULL");
        return status;
    }

    AXP_HOT AXP_ALWAYS_INLINE int cpuCount() const noexcept
    {
        return m_cpuCount;
    }

private:
    // ====================================================================
    // Member Data (OWNED via std::unique_ptr)
    // ====================================================================

    int m_cpuCount{ 4 };  // Default, overwritten in constructor

    std::unique_ptr<GuestMemory> m_guestMemory;
    std::unique_ptr<MMIOManager> m_mmio;
    std::unique_ptr<GrainResolver> m_grainResolver;
 //   std::unique_ptr<PalVectorTable> m_palVectorTable;
    std::unique_ptr<WriteBufferManager> m_writeBufferMgr;
    std::unique_ptr<SafeMemory> m_safeMemory;
  //  std::unique_ptr<MemoryBarrierCoordinator> m_memoryBarrierCoord;
    std::unique_ptr<ScsiController> m_scsiCtrl;
    std::unique_ptr<IPIManager> m_ipiManager;
    std::unique_ptr<ConsoleManager> m_consoleManager;


    // Helper Private

    // SubsystemCoordinator constructor or initialization method

    bool createMemorySubsystems()
    {
        auto& config = global_EmulatorSettings();
        const auto& mm = config.podData.memoryMap;

        // Create SafeMemory
        m_safeMemory = std::make_unique<SafeMemory>();

        // Create MMIOManager
        m_mmio = std::make_unique<MMIOManager>();

        return true;
    }
};

#endif // SUBSYSTEMCOORDINATOR_H