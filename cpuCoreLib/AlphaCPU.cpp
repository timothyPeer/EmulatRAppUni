// ============================================================================
// AlphaCPU.cpp - Minimal Implementation (MOC Requirements Only)
// ============================================================================
// This file contains ONLY the constructor, destructor, and executeLoop()
// which are required by Qt's Meta-Object Compiler (MOC).
//
// All other methods are implemented inline in AlphaCPU.h.
// ============================================================================

#include "AlphaCPU.h"
#include "CBoxLib/CBoxBase.h"
#include "PalBoxLib/PalBoxBase.h"
#include "EBoxLib/EBoxBase.h"
#include "FBoxLib/FBoxBase.h"
#include "MBoxLib_EV6/MBoxBase.h"
#include "IBoxLib/iBoxBase.h"
#include "AlphaPipeline.h"
#include "coreLib/BoxRequest.h"
#include "faultLib/GlobalFaultDispatcherBank.h"
#include "faultLib/FaultDispatcher.h"
#include "emulatrLib/IPIManager.h"
#include "emulatrLib/global_IPIManager.h"

#define COMPONENT_NAME "AlphaCPU"

// Static console (CPU 0 only)
std::unique_ptr<SRMConsole> AlphaCPU::s_srmConsole;

// ============================================================================
// Constructor - Box Creation and Injection
// ============================================================================

AlphaCPU::AlphaCPU(CPUIdType cpuId, CBox* cbox,  QObject* parent) noexcept
    : QObject(parent)
      , m_cpuId(cpuId)
      , m_faultSink(&globalFaultDispatcher(cpuId))
      , m_cBox(cbox)
, m_reservationManager(&globalReservationManager())
, m_ipiManager(&global_IPIManager())
, m_memoryBarrierCoordinator(&global_MemoryBarrierCoordinator())
, m_tlb(&globalEv6SPAM())
, m_faultDispatcher{&globalFaultDispatcher(m_cpuId) }
, m_ev6Translate{cpuId}
, m_iprGlobalMaster(getCPUStateView(cpuId))
{

    m_pending.reset(new IRQPendingState());
    m_router.reset(new InterruptRouter());
    m_pending->reset();
    m_router->registerCPU(m_cpuId, m_pending.data());
    m_router->registerPlatformSources();
   

    // ========================================================================
    // Create all boxes (AlphaCPU owns everything)
    // ========================================================================
    m_pBox.reset(new PalBox(cpuId, m_pending.data(),m_router.data()));
    m_eBox.reset(new EBox(cpuId));
    m_fBox.reset(new FBox(cpuId));
    m_mBox.reset(new MBox(cpuId));
    m_iBox.reset(new IBox(cpuId, &global_ExecutionCoordinator(),m_faultDispatcher, &global_GuestMemory()));



    // ========================================================================
    // Create pipeline with box references
    // ========================================================================
    m_alphaPipeline.reset(new AlphaPipeline(
        cpuId,
        m_cBox,
        m_mBox.data(),
        m_eBox.data(),
        m_fBox.data(),
        m_pBox.data()
    ));

    // ========================================================================
    // Inject boxes into pipeline (once, at construction)
    // ========================================================================
    m_alphaPipeline->injectOtherBoxes(
        m_eBox.data(),
        m_fBox.data(),
        m_mBox.data(),
        m_pBox.data(),
        m_cBox        
    );
    INFO_LOG(QString("AlphaCPU %1: Initialized with all boxes").arg(cpuId));
}

// ============================================================================
// Destructor
// ============================================================================

AlphaCPU::~AlphaCPU()
{
    // QScopedPointer handles cleanup automatically
    DEBUG_LOG(QString("AlphaCPU %1: Destroyed").arg(m_cpuId));
}

// ============================================================================
// Main Execution Loop (Slot - must be in .cpp for MOC)
// ============================================================================



void AlphaCPU::handleIPIInterrupt() const noexcept
{

    auto cpuState = m_router->cpuPendingState(m_cpuId);
   

    // Drain IPI mailbox
    quint64 ipiData = m_ipiManager->fetchIPI(m_cpuId);

    if (ipiData == 0) {
        return;  // Spurious IPI
    }

    // Decode command
    IPICommand command = static_cast<IPICommand>(ipiData & 0xFF);
    quint8     asn     = static_cast<quint8>((ipiData >> 8) & 0xFF);


    switch (command) {
    case IPICommand::TLB_INVALIDATE_ASN: {
        ASNType asn_local = decodeIPIParam8(ipiData);
        m_tlb->invalidateTLBsByASN(m_cpuId, asn_local);
        break;
    }

    case IPICommand::TLB_INVALIDATE_VA_BOTH: {
        VAType va = decodeIPIParam56(ipiData);
        m_tlb->invalidateTLBEntry(m_cpuId, Realm::Both, va, asn);
        break;
    }

    case IPICommand::TLB_INVALIDATE_ALL:
        m_tlb->invalidateAllTLBs(m_cpuId);
        break;
    case IPICommand::MEMORY_BARRIER_FULL:
        // Drain write buffer
        m_cBox->drainWriteBuffers();

        // Acknowledge barrier
        m_memoryBarrierCoordinator->acknowledgeMemoryBarrier(m_cpuId);
        break;
    default:;

    }
}

void AlphaCPU::initializeFirmware()  noexcept
{
    setPC(0x900001);
    setPAL_BASE(0x900000);
    while ((m_iprGlobalMaster->h->pc & ~1ULL) >= 0x200000) {
        runOneInstruction();
    }
}


void AlphaCPU::executeLoop() 
{
    m_running.store(true, std::memory_order_release);

    m_alphaPipeline->injectOtherBoxes(m_eBox.get(), m_fBox.get(), m_mBox.get(), m_pBox.get(), m_cBox);

    INFO_LOG(QString("CPU %1: Execution loop started on thread 0x%2")
        .arg(m_cpuId)
        .arg(reinterpret_cast<quint64>(QThread::currentThreadId()), 0, 16));

    try {
        // Main execution loop
        while (!m_stopRequested.load(std::memory_order_acquire)) {

            // ============================================================
            // Handle pause state - yield while paused
            // ============================================================
            if (m_paused.load(std::memory_order_acquire)) {
                QThread::msleep(1);  // Sleep 1ms while paused
                continue;
            }

            // ============================================================
            // Handle halt state - CPU halted by HW_REI
            // ============================================================
            if (m_halted.load(std::memory_order_acquire)) {
                QThread::msleep(10);  // Longer sleep when halted

                // Check for wake-up conditions
                if (checkForWakeup()) {
                    m_halted.store(false, std::memory_order_release);
                    INFO_LOG(QString("CPU %1: Waking from halt").arg(m_cpuId));
                }
                continue;
            }

            // ============================================================
            // HOT PATH: Execute one instruction
            // ============================================================
            runOneInstruction();

            // ============================================================
            // Update counters (batched every 4K instructions)
            // ============================================================
            ++m_localInstrCount;

            // TODO: Publish to CPUStatusBlock when implemented
            // if ((m_localInstrCount & 0xFFF) == 0) {
            //     m_statusBlock->instrRetired.store(m_localInstrCount);
            // }
        }

        INFO_LOG(QString("CPU %1: Execution loop stopping normally").arg(m_cpuId));

    }
    catch (const std::exception& e) {
        // Fatal error - log and notify coordinator
        ERROR_LOG(QString("CPU %1: Fatal exception: %2")
            .arg(m_cpuId).arg(e.what()));
        emit fatalError(m_cpuId, QString::fromUtf8(e.what()));
    }
    catch (...) {
        ERROR_LOG(QString("CPU %1: Unknown fatal exception").arg(m_cpuId));
        emit fatalError(m_cpuId, "Unknown exception");
    }

    // Clean shutdown
    shutdownGracefully();

    m_running.store(false, std::memory_order_release);

    INFO_LOG(QString("CPU %1: Execution loop ended").arg(m_cpuId));

    // Signal thread to quit
    QThread::currentThread()->quit();
}
