// ============================================================================
// AlphaCPU.h - CPU Orchestrator (Mostly Header-Only)
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

#ifndef AlphaCPU_h__
#define AlphaCPU_h__

// ============================================================================
// AlphaCPU.h - CPU Orchestrator (Mostly Header-Only)
// ============================================================================
// Orchestrates instruction execution through AlphaPipeline.
// Responsibilities:
//   - Run loop: fetch -> pipeline -> retire -> check faults
//   - Box ownership and injection (single owner)
//   - Context save/restore (SSOT)
//   - Interrupt/trap handling
//   - PAL mode transitions
// ============================================================================

#include <atomic>
#include <memory>
#include <chrono>

#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/enum_header.h"
#include "coreLib/types_core.h"
#include "coreLib/LoggingMacros.h"
#include "coreLib/BoxRequest.h"
#include "machineLib/PipeLineSlot.h"
#include "exceptionLib/PendingEventKind.h"
#include "exceptionLib/ExceptionClass_EV6.h"
#include "faultLib/PendingEvent_Refined.h"
#include "faultLib/FaultDispatcher.h"
#include "palLib_EV6/PAL_core.h"
#include "palLib_EV6/Pal_core_inl.h"
#include "palLib_EV6/PalVectorId_refined.h"
#include "CBoxLib/CBoxBase.h"
#include "EBoxLib/EBoxBase.h"
#include "FBoxLib/FBoxBase.h"
#include "MBoxLib_EV6/MBoxBase.h"
#include "PalBoxLib/PalBoxBase.h"
#include "IBoxLib/iBoxBase.h"
#include "deviceLib/SRMConsole.h"
#include "configLib/global_EmulatorSettings.h"
#include "deviceLib/global_SRMEnvStore.h"
#include "AlphaPipeline.h"
#include "machineLib/PipeLineSlot.h"
#include "memoryLib/global_MemoryBarrierCoordinator.h"

// Forward declarations
struct IFaultSink;
struct FetchResult;

#define COMPONENT_NAME "AlphaCPU"

// ============================================================================
// AlphaCPU - CPU Orchestrator
// ============================================================================
class alignas(8) AlphaCPU final : public QObject
{
    Q_OBJECT

    // ========================================================================
    // Pending Event (Local to CPU)
    // ========================================================================
    enum class PendingEventType {
        None,
        CodeModification,
        CacheInvalidation,
    };

    struct PendingEvent_cpuLocal {
        PendingEventType type{ PendingEventType::None };
        quint64 startPC{ 0 };
        quint64 endPC{ 0 };
        quint64 eventData{ 0 };

        void clear() noexcept { type = PendingEventType::None; }
        bool isPending() const noexcept { return type != PendingEventType::None; }
    };

    // ========================================================================
    // Member Data
    // ========================================================================

    // IRQ Controllers
    QScopedPointer<IRQPendingState> m_pending;
    QScopedPointer<InterruptRouter> m_router;
    //


    // CPU Configuration
    CPUIdType m_cpuId{ 0 };
    CPUFamily m_family{CPUFamily::EV6};
    FaultDispatcher* m_faultSink{ nullptr };

    // Thread Control State
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_paused{ false };
    std::atomic<bool> m_stopRequested{ false };
    std::atomic<bool> m_halted{ false };
    std::atomic<bool> m_rescheduleRequested{ false };

    // Performance Counters
    quint64 m_localInstrCount{ 0 };
    quint64 m_localCycleCount{ 0 };

    // Pending Events
    PendingEvent_cpuLocal m_pendingEvent;

 
    // Subsystem References (Injected)
    // HWPCBBank* m_hwpcbBank{ nullptr };
    // HWPCB* m_hwpcb{ nullptr };
    ReservationManager* m_reservationManager;
    IPIManager* m_ipiManager;
    MemoryBarrierCoordinator* m_memoryBarrierCoordinator;
    Ev6SPAMShardManager* m_tlb;
    FaultDispatcher* m_faultDispatcher;
    CPUStateView* m_iprGlobalMaster{ nullptr };
    // Box Ownership (AlphaCPU owns all boxes)
    CBox* m_cBox;
    QScopedPointer<PalBox> m_pBox;
    QScopedPointer<EBox> m_eBox;
    QScopedPointer<FBox> m_fBox;
    QScopedPointer<MBox> m_mBox;
    QScopedPointer<IBox> m_iBox;
    QScopedPointer<AlphaPipeline> m_alphaPipeline;

    Ev6Translator m_ev6Translate;

    // Static Console (CPU 0 only)
    static std::unique_ptr<SRMConsole> s_srmConsole;

    // Error state
    quint32 m_errorCount{ 0 };
    QString m_lastError;

public:
    // ========================================================================
    // Constructor & Destructor (in .cpp for MOC requirement)
    // ========================================================================
    explicit AlphaCPU(CPUIdType cpuId, CBox* cBox,  QObject* parent = nullptr) noexcept;
    ~AlphaCPU() override;


    // Error severity levels
    enum class ErrorSeverity {
        WARNING,          // Log only
        RECOVERABLE,      // Queue fault, continue
        FATAL,           // Halt CPU
        MACHINE_CHECK    // Alpha machine check exception
    };


#pragma region Cycle Count Functions

    // ============================================================================
    // Cycle Counter Control Bits
    // bit 0: CC_CTL_ENABLE     -> cycle counter increments when set
    // bit 1: CC_CTL_FREEZE_PAL -> when set, cycle counter freezes in PAL mode
    // ============================================================================
    static constexpr quint64 CC_CTL_ENABLE = 0x1;
    static constexpr quint64 CC_CTL_FREEZE_PAL = 0x2;

    // ============================================================================
    // incrementCycleCount
    // Called once per retired instruction (or per cycle-model increment).
    // cnt8 = optional cycle increment amount (ignored unless you choose to use it)
    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE void  incrementCycleCount(quint8 cnt8 = 1) const noexcept
    {
        const quint64 ctl = m_iprGlobalMaster->r->cc_ctl;
        const bool enabled = (ctl & CC_CTL_ENABLE) != 0;
        const bool frozen = (ctl & CC_CTL_FREEZE_PAL) && m_iprGlobalMaster->isInPalMode();

        m_iprGlobalMaster->r->cc += (enabled & !frozen) * cnt8;
    }

    // ============================================================================
    // Read the cycle counter
    // ============================================================================
    inline quint64 getCycleCount() const noexcept
    {
        return m_iprGlobalMaster->r->cc;
    }

    // ============================================================================
    // Reset the cycle counter and re-enable CC
    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE void  cycleCounterReset() const noexcept
    {

       m_iprGlobalMaster->r->cc = 0;
       m_iprGlobalMaster->r->cc_ctl = CC_CTL_ENABLE;  // reset -> enable CC by default
    }

    // ============================================================================
    // Enable cycle counter increments
    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE void  cycleCounterEnable() const noexcept
    {
        m_iprGlobalMaster->r->cc_ctl |= CC_CTL_ENABLE;
    }

    // ============================================================================
    // Disable cycle counter increments
    // ============================================================================
    AXP_HOT AXP_ALWAYS_INLINE void  cycleCounterDisable() const noexcept
    {
        m_iprGlobalMaster->r->cc_ctl &= ~CC_CTL_ENABLE;
    }


#pragma endregion Cycle Count Functions




    AXP_HOT AXP_ALWAYS_INLINE void setPAL_BASE(quint64 palBase) const noexcept
    {
        m_iprGlobalMaster->x->pal_base = palBase;
    }
    AXP_HOT AXP_ALWAYS_INLINE quint64 getPAL_BASE() const noexcept
    {
        return  m_iprGlobalMaster->x->pal_base;
    }

    AXP_HOT AXP_ALWAYS_INLINE quint64 getPC() const noexcept
    {
        return  m_iprGlobalMaster->h->pc;
    }
    // ========================================================================
    // Thread Control (Inline)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  void pause() noexcept {
        m_paused.store(true, std::memory_order_release);
        DEBUG_LOG(QString("CPU %1: Pause requested").arg(m_cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE  void resume() noexcept {
        m_paused.store(false, std::memory_order_release);
        DEBUG_LOG(QString("CPU %1: Resume requested").arg(m_cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE  void stop() noexcept {
        m_stopRequested.store(true, std::memory_order_release);
        DEBUG_LOG(QString("CPU %1: Stop requested").arg(m_cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE  void start() const noexcept {
        WARN_LOG(QString("CPU %1: start() is legacy - use executeLoop() slot").arg(m_cpuId));
    }

    // Query methods
    AXP_HOT AXP_ALWAYS_INLINE  bool isRunning() const noexcept {
        return m_running.load(std::memory_order_acquire);
    }

    AXP_HOT AXP_ALWAYS_INLINE  bool isPaused() const noexcept {
        return m_paused.load(std::memory_order_acquire);
    }

    AXP_HOT AXP_ALWAYS_INLINE  bool isCPUHalted() const noexcept {
        return m_halted.load(std::memory_order_acquire);
    }

    AXP_HOT AXP_ALWAYS_INLINE  bool isInPalMode() const noexcept {
        return m_pBox.data()->isInPalMode();
    }

    // ========================================================================
    // Accessors
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  CPUIdType cpuId() const noexcept { return m_cpuId; }
    AXP_HOT AXP_ALWAYS_INLINE  CPUFamily family() const noexcept { return m_family; }
    PalService* palService() const noexcept { return m_pBox->palService(); }

    const PendingEvent_cpuLocal& getPendingEvent() const noexcept { return m_pendingEvent; }
    void clearPendingEvent() noexcept { m_pendingEvent.clear(); }

    // ========================================================================
    // Reset
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  void reset() const noexcept {
        m_cBox->getBranchPredictor().clear();
        setPalMode(true,true);
        setCMMode(Mode_Privilege::Kernel);                      // start in Kernel Mode
        qDebug() << m_iprGlobalMaster->h->pc;
    }

    // ========================================================================
    // SRM Console (CPU 0 only)
    // ========================================================================

    void enterSRMConsole() noexcept {
        if (m_cpuId != 0) {
            haltUntilSRMExit();
            return;
        }

        auto settings = global_EmulatorSettings();
        auto envStore = global_SRMEnvStore();

        if (!s_srmConsole) {
            s_srmConsole = std::make_unique<SRMConsole>(settings, envStore);
            s_srmConsole->initialize(m_cpuId);
        }

        s_srmConsole->start();
        while (s_srmConsole->isRunning()) {
            s_srmConsole->step();
        }
    }

    // ========================================================================
    // Main Execution (Hot Path - Inline)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  void runOneInstruction() noexcept {
     
        // 1. Check for external events (interrupts)
        if (m_pending->hasDeliverable(m_iprGlobalMaster->h->getIPL())) {
            handleInterrupt();
            return;
        }

        // 2. Fetch and decode
        //FetchResult fetchResult;
       //m_iBox->fetchAndDecode(fetchResult);
        FetchResult fetchResult = m_iBox->fetchNext(); 

        // 3. Supply to pipeline
        BoxResult boxResult =  m_alphaPipeline->tick(fetchResult);

        
        // 4. Handle BoxResult flags
        // Note:  Pipeline payloads to register occurs only during stage_WB 
        // defer routing to PAL until after stage_WB - Only handle faults if they reached retirement (dispatched from WB)
        if (boxResult.hasFault() && boxResult.faultWasDispatched()) {

            m_alphaPipeline->flush("flush::Box-faultWasDispatched");       // flush the pipeline

            quint64 vector = getFaultVector(boxResult.getFaultClass());
            BoxResult bx =   m_pBox->enterPal(getFaultReason(boxResult.getFaultClass()), vector, boxResult.getFaultPC()  );
          
            if (boxResult.needsWriteDrain())
                m_cBox->drainWriteBuffers();

            if (boxResult.needsMemoryBarrier())
                m_cBox->issueMemoryBarrier(MemoryBarrierKind::PAL, m_cpuId+1);

            if (boxResult.needsHalted())
                haltCPU();

            // ========================================================================
            // CRITICAL: Don't call handleException() here!
            // The next iteration of runOneInstruction() will fetch from the new PC
            // and execute PAL code naturally through the normal pipeline
            // ========================================================================
            // 3c. Dispatch to specific handler
  //          m_pBox->handleException(boxResult);

            // ========================================================================
            // SET PC TO PAL HANDLER ADDRESS
            // ========================================================================
            quint64 palBase = m_iprGlobalMaster->x->scbb;  // Or however you get PAL_BASE
            quint64 palEntryPC = palBase + vector;
            /*
             ; DTB_MISS_SINGLE handler (at offset 0x0200)
            MFPR    R16, EXC_ADDR       ; Get faulting VA
            MFPR    R17, MM_STAT        ; Get fault status
            SRL     R16, #13, R18       ; VA >> 13 = VPN
            LDQ     R19, ptbr(R31)      ; Load page table base
            ; ... walk page table ...
            ; ... find PTE ...
            MTPR    R20, DTB_PTE        ; Write TLB entry
            HW_REI                      ; Return to faulting instruction
            ```

            This is **NOT** a CALL_PAL instruction. It's the actual exception handler code.

            ## Order of Operations for DTB_MISS:
            ```
            1. LDQ executes at PC 0x2000800C
               
            2. Calculate VA = 0x20008110
               v
            3. TLB lookup -> MISS
               v
            4. Hardware exception logic:
               - Save faulting PC (0x2000800C) -> EXC_PC IPR
               - Save faulting VA (0x20008110) -> EXC_ADDR IPR
               - Calculate PAL entry: PAL_BASE + 0x0200 = 0x20008200
               - Set PC <- 0x20008200 (with bit 0 = 1 for PAL mode)
               - Set PAL mode flag
               v
            5. Next instruction fetch:
               - Fetch from PC 0x20008200
               - This fetches the FIRST instruction of the PAL handler
               - Execute it (MFPR, LDQ, whatever it is)
               v
            6. Continue executing PAL handler instructions sequentially
               v
            7. PAL handler writes TLB entry via MTPR DTB_PTE
               v
            8. PAL handler executes HW_REI:
               - Restore PC <- EXC_PC (back to 0x2000800C)
               - Clear PAL mode
               v
            9. LDQ retries:
               - TLB lookup -> HIT (entry was just loaded)
               - Translation succeeds
               - Load completes
             */

            if (boxResult.needsEnterPalmode()) {
                m_alphaPipeline->flush("flush::needsEnterPalMode");

                // stage_WB already computed palVector and palFunction in PipelineStepResult
                // But those are lost because execute() returns BoxResult, not PipelineStepResult
                // Need to route the PAL function code through BoxResult or handle differently

                // Option A: Let PalBox::enterPal handle it (cleanest)
                quint8 palFunc = boxResult.palFunction;  // Need to add this field to BoxResult
                quint64 callPC = boxResult.faultingPC;   // Need to repurpose or add field

                
                BoxResult palResult = m_pBox->enterPal(
                    PalEntryReason::CALL_PAL_INSTRUCTION, palFunc, callPC + 4);

                // Handle side-effects from enterPal
                if (palResult.needsPipelineFlush())
                    m_alphaPipeline->flush("flush::palResult.needsPipelineFlush()");
                return;
            }
            qDebug() << "==========================================";
            qDebug() << "+   JUMPING TO PAL HANDLER                +";
            qDebug() << "==========================================+";
            qDebug() << "Fault Class:" << static_cast<int>(boxResult.getFaultClass());
            qDebug() << "Vector Offset:" << Qt::hex << vector;
            qDebug() << "PAL Base:" << Qt::hex << palBase;
            qDebug() << "PAL Entry PC:" << Qt::hex << palEntryPC;

            m_iprGlobalMaster->h->pc = (palEntryPC | 0x1);  // Set PC with PAL mode bit (bit 0)
            m_alphaPipeline->flush("flush::JMP to Pal Handler");

            return;
        }

        // if (boxResult.needsPipelineFlush()) {
        //     m_alphaPipeline->flush("flush::ACP-Instruction Loop");
        // }

        if (boxResult.needsPipelineFlush()) {
            qDebug() << "UNEXPECTED flush request at PC:" << Qt::hex << fetchResult.virtualAddress;
        }

        if (boxResult.needsMemoryBarrier()) {
            // TODO: Handle memory barrier via CBox
        }
    }


    // ========================================================================
    // PAL Mode Management (Inline)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  void enterPalMode(quint64 faultVector, quint64 faultPC) noexcept {
        // Save complete context
        m_iprGlobalMaster->saveContext();

        // Set EXC_ADDR (faulting instruction or retry address)
        m_iprGlobalMaster->h->exc_addr = faultPC;

        // Enter PAL mode at fault vector
        m_iprGlobalMaster->h->pc = faultVector | 0x1;
        m_iprGlobalMaster->h->setIPL_Unsynced( 7);
        m_iprGlobalMaster->h->setCM(CM_KERNEL);

        // Flush pipeline
        m_alphaPipeline->flush("Flush::enterPalMode");
    }

    AXP_HOT AXP_ALWAYS_INLINE  void enterPalMode(PalEntryReason reason, quint64 vector, quint64 faultPC) noexcept {
        DEBUG_LOG(QString("AlphaCPU: Entering PAL mode, reason=%1")
            .arg(static_cast<int>(reason)));

        // 1. Save complete context
        m_iprGlobalMaster->saveContext();

        // 2. Compute entry PC based on reason
        quint64 entryPC;
        if (reason == PalEntryReason::CALL_PAL_INSTRUCTION) {
            entryPC = m_iprGlobalMaster->computeCallPalEntry( static_cast<quint8>(vector));
        }
        else {
            entryPC = vector;
        }

        m_iprGlobalMaster->h->exc_addr = faultPC;

        // Enter PAL mode at fault vector
        m_iprGlobalMaster->h->pc = entryPC | 0x1;
        m_iprGlobalMaster->h->setIPL_Unsynced( 7);
        m_iprGlobalMaster->h->setCM(CM_KERNEL);

        // 5. Activate PAL shadow registers if needed
        if (reason == PalEntryReason::CALL_PAL_INSTRUCTION) {
            m_iprGlobalMaster->setShadowEnabled( true);
        }

        // 6. Flush pipeline
        m_alphaPipeline->flush("flush: enterPalMode w/EntryReason");

        DEBUG_LOG(QString("AlphaCPU: PAL entry complete, PC=0x%1")
            .arg(entryPC, 16, 16, QChar('0')));
    }

    AXP_HOT inline  BoxResult executeREI(PipelineSlot& slot) const noexcept {
        // 1. Restore COMPLETE context (HWPCB + registers)
        m_iprGlobalMaster->restoreContext();

        // 2. Get return PC (now restored)
        quint64 returnPC = m_iprGlobalMaster->h->pc;

#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_PAL_EXIT(m_cpuId, returnPC, m_iprGlobalMaster->h->ipl, m_iprGlobalMaster->h->cm);
#endif

        // 3. Setup redirect
        slot.reiTarget = returnPC;
        slot.pcModified = true;

        // 4. Flush pipeline
        return BoxResult().flushPipeline();
    }

    AXP_HOT AXP_ALWAYS_INLINE  void commitPalReturnValue(const PalResult& pr) const noexcept {
        if (!pr.doesReturn) return;

        quint8 dr = palReturnRegToIntReg(pr.returnReg);
        if (pr.hasReturnValue) {
            m_iprGlobalMaster->i->write(dr, pr.returnValue);
        }
    }

    // ========================================================================
    // Code Modification & Cache Management (Inline)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE  void handleCodeModification(quint64 startPC, quint64 endPC) noexcept {
        DEBUG_LOG(QString("AlphaCPU: Code modification detected PC=0x%1-0x%2")
            .arg(startPC, 16, 16, QChar('0'))
            .arg(endPC, 16, 16, QChar('0')));

        m_pendingEvent.type = PendingEventType::CodeModification;
        m_pendingEvent.startPC = startPC;
        m_pendingEvent.endPC = endPC;

        DEBUG_LOG("AlphaCPU: Code modification event queued for next cycle");
    }


    AXP_HOT AXP_ALWAYS_INLINE  void contextSwitch(CPUIdType cpuId) const noexcept
    {
        m_reservationManager->breakReservation(cpuId);
    }

    AXP_HOT AXP_ALWAYS_INLINE void setPC(quint64 pc) const noexcept
    {
        m_iprGlobalMaster->h->pc = pc;
    }

    AXP_HOT AXP_ALWAYS_INLINE void setCMMode(Mode_Privilege mode) const noexcept
    {
        m_iprGlobalMaster->h->cm =  static_cast<quint8>(mode);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setPalMode(bool bSetPal, bool reset = false) const noexcept
    {
        m_pBox->palService()->setPalMode(bSetPal, reset);
    }
    AXP_HOT AXP_ALWAYS_INLINE void setIPL(IPLType ipl ) const noexcept
    {
        m_iprGlobalMaster->h->setIPL_Unsynced( ipl);
    }

    // ========================================================================
    // Interrupt/Trap Handling (Inline)
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE void handleInterrupt() noexcept
    {
        const quint8 currentIPL = m_iprGlobalMaster->h->getIPL();
        if (!m_pending->hasDeliverable(currentIPL))
            return;

        ClaimedInterrupt claimed = m_pending->claimNext(currentIPL);
        if (!claimed.valid)
            return;

        m_pBox->palService()->clearSisrIfSoftware(claimed);
        m_reservationManager->breakReservation(m_cpuId);
        m_pBox->palService()->deliverInterrupt(claimed);
        m_alphaPipeline->flush("flush::interruptDelivery");
#if AXP_INSTRUMENTATION_TRACE
        EXECTRACE_INTERRUPT(
            m_cpuId,
            m_iprGlobalMaster->h->pc,
            claimed.vector,
            static_cast<quint8>(claimed.source),
            claimed.ipl
        );
#endif

    }

    AXP_HOT AXP_ALWAYS_INLINE  void handleTrap(const ExceptionClass_EV6& trapClass) noexcept {
           quint64 vectorPC = static_cast<quint64>(mapExceptionToPalVector(trapClass));

        enterPalMode(mapExceptionToPalEntry(trapClass), vectorPC, m_iprGlobalMaster->h->pc);

        DEBUG_LOG(QString("CPU %1: Trap - class=%2 vector=0x%3")
            .arg(m_cpuId)
            .arg(static_cast<int>(trapClass))
            .arg(vectorPC, 16, 16, QChar('0')));
    }

    AXP_HOT AXP_ALWAYS_INLINE  quint64 computeTrapVector(ExceptionClass_EV6 trapClass) const noexcept {
        auto& iprsExt = globalIPRHotExt(m_cpuId);
        const quint64 PAL_BASE = iprsExt.scbb;

        constexpr quint64 TRAP_TABLE_OFFSET = 0x100;
        const quint64 trapOffset = static_cast<quint64>(trapClass) * 0x10;

        return PAL_BASE + TRAP_TABLE_OFFSET + trapOffset;
    }


    AXP_HOT AXP_ALWAYS_INLINE  void handleRedirect(RedirectReason reason, quint64 metadata1, quint64 metadata2) noexcept {
        quint64 vectorPC = 0;

        switch (reason) {
        case RedirectReason::PALEntry:
            vectorPC = m_iprGlobalMaster->computeCallPalEntry( static_cast<quint8>(metadata1));
            enterPalMode(PalEntryReason::CALL_PAL_INSTRUCTION, vectorPC, m_iprGlobalMaster->h->pc);
            break;

        case RedirectReason::Trap:
            vectorPC = computeTrapVector(static_cast<ExceptionClass_EV6>(metadata1));
            enterPalMode(PalEntryReason::TRAP, vectorPC, m_iprGlobalMaster->h->pc);
            break;

        case RedirectReason::Interrupt:
        {
            PalVectorId_EV6 vectorId = PalVectorId_EV6::INTERRUPT;
            quint64 palBase = globalIPRHotExt(m_cpuId).scbb;
            vectorPC = computePalVectorPC(vectorId, palBase);
            enterPalMode(PalEntryReason::INTERRUPT, vectorPC, m_iprGlobalMaster->h->pc);
            break;
        }
        case RedirectReason::BranchMisprediction:
        case RedirectReason::BranchTaken:
        case RedirectReason::Jump:
        case RedirectReason::Return:
            m_iprGlobalMaster->h->pc = (metadata1);
            break;

        case RedirectReason::PALReturn:
            m_iprGlobalMaster->restoreContext();
            break;

        default:
            WARN_LOG(QString("CPU %1: Unknown redirect reason: %2")
                .arg(m_cpuId)
                .arg(getRedirectReasonName(reason)));
            return;
        }

        if (requiresPipelineFlush(reason)) {
            m_alphaPipeline->flush("flush::handleRedirect w/Reason");
        }

        DEBUG_LOG(QString("CPU %1: Redirect - %2 -> PC=0x%3")
            .arg(m_cpuId)
            .arg(getRedirectReasonName(reason))
            .arg(vectorPC ? vectorPC : metadata1, 16, 16, QChar('0')));
    }

    AXP_HOT AXP_ALWAYS_INLINE bool hasInterruptPending() const noexcept
    {
        return m_pending->hasDeliverable(m_iprGlobalMaster->h->getIPL());
    }

    

    AXP_HOT AXP_ALWAYS_INLINE  bool hasPendingTrap() const noexcept {
       return m_faultDispatcher->hasPendingTrap();
    }

    AXP_HOT AXP_ALWAYS_INLINE void checkInterrupts(PipelineSlot& slot) const noexcept
    {
        const quint8 currentIPL = m_iprGlobalMaster->h->getIPL();
        if (!m_pending->hasDeliverable(currentIPL))
            return;

        ClaimedInterrupt claimed = m_pending->claimNext(currentIPL);
        if (!claimed.valid)
            return;

        // Clear SISR bit for software interrupts (IPR SSOT, not pending state)
        if (IrqSource::isSoftwareSource(claimed.source)) {
            m_iprGlobalMaster->h->sisr &=
                ~static_cast<quint16>(1u << claimed.ipl);
        }

        m_pBox->palService()->deliverInterrupt( claimed);
    }

    // ========================================================================
    // IPI Handling (Inline)
    // ========================================================================


    AXP_ALWAYS_INLINE void handleIPIInterrupt() const noexcept;
    AXP_ALWAYS_INLINE void initializeFirmware() noexcept;


    /**
 * @brief Handle TLB shootdown IPI on receiving CPU
 * Called when IPI interrupt is processed
 */
    // ReSharper disable once CppMemberFunctionMayBeStatic
    AXP_ALWAYS_INLINE  void handleTLBShootdownIPI(CPUIdType cpuId, quint64 ipiData) const noexcept
    {
        IPICommand cmd = decodeIPICommand(ipiData);
        ASNType asn_g = m_iprGlobalMaster->h->asn; // getASN_Active(cpuId);
        switch (cmd) {
        case IPICommand::TLB_INVALIDATE_VA_ITB:
        {
            // Decode VA from IPI data
            quint64 va = decodeIPIVA(ipiData);

            // Get current ASN (or invalidate for all ASNs)
         

            // Invalidate local ITB entry
            m_tlb->invalidateTLBEntry(cpuId, Realm::I, va, asn_g);

            DEBUG_LOG(QString("CPU %1: Processed ITB shootdown for VA=0x%2")
                .arg(cpuId)
                .arg(va, 16, 16, QChar('0')));
            break;
        }

        case IPICommand::TLB_INVALIDATE_VA_DTB:
        {
            quint64 va = decodeIPIVA(ipiData);
   
            m_tlb->invalidateTLBEntry(cpuId, Realm::D, va, asn_g);
            break;
        }

        case IPICommand::TLB_INVALIDATE_ASN:
        {
            // Decode ASN from IPI data
            const ASNType asn = decodeIPIASN(ipiData);

            // Invalidate all TLB entries for this ASN
            m_tlb->invalidateTLBsByASN(cpuId, asn);

            DEBUG_LOG(QString("CPU %1: Processed TLB shootdown for ASN=%2")
                .arg(cpuId)
                .arg(asn));
            break;
        }

        case IPICommand::TLB_INVALIDATE_ALL:
        {
            // Invalidate entire TLB
            m_tlb->invalidateAllTLBs(cpuId);

            DEBUG_LOG(QString("CPU %1: Processed full TLB flush").arg(cpuId));
            break;
        }

        default:
            WARN_LOG(QString("CPU %1: Unknown TLB shootdown command %2")
                .arg(cpuId)
                .arg(static_cast<int>(cmd)));
            break;
        }
    }


    AXP_HOT AXP_ALWAYS_INLINE  void handleCacheCoherency(quint32 p1, quint32 p2, quint64 p3) const noexcept {
        // TODO: Implement cache coherency
        DEBUG_LOG(QString("CPU%1: Cache coherency IPI").arg(m_cpuId));
    }

    AXP_HOT AXP_ALWAYS_INLINE  void handleBarrierSync(quint32 barrierIdHigh, quint32 barrierIdLow, quint64 flags) const noexcept {
        const quint64 barrierId = (static_cast<quint64>(barrierIdHigh) << 32) | barrierIdLow;
        // TODO: Implement barrier sync
        DEBUG_LOG(QString("CPU%1: Barrier sync barrier=%2")
            .arg(m_cpuId)
            .arg(barrierId));
    }



    // ========================================================================
    // Wake-up Checks (Inline)
    // ========================================================================
    AXP_HOT AXP_ALWAYS_INLINE bool checkForWakeupInterrupts() const noexcept
    {
        return m_pending->pendingLevelsMask.load(std::memory_order_acquire) != 0;
    }


    // ====================================================================
    // Error Handling
    // ====================================================================

    /**
     * @brief Report error condition
     * @param reason Error description
     * @param severity Severity level
     */
    void reportError(const QString& reason, ErrorSeverity severity) noexcept;

public slots:
    /**
     * @brief Main execution loop - called when worker thread starts
     * Runs until stop() is called or fatal error occurs
     */
    void executeLoop();
    void error(CPUIdType cpuId, QString reason) noexcept // Signal, NO implementation
    {

    }

signals:
    /**
     * @brief Emitted when CPU enters halted state
     */
    void halted(CPUIdType cpuId, quint32 haltCode);

    /**
     * @brief Emitted on fatal error
     */
    void fatalError(CPUIdType cpuId, const QString& errorMessage);

private:
    // ========================================================================
    // Private Helpers (Inline)
    // ========================================================================

    void handlePendingEventInLoop() noexcept {
        if (!m_pendingEvent.isPending()) {
            return;
        }

        DEBUG_LOG(QString("CPU %1: Handling pending event type %2")
            .arg(m_cpuId)
            .arg(static_cast<int>(m_pendingEvent.type)));

        switch (m_pendingEvent.type) {
        case PendingEventType::CodeModification:
            if (m_iBox) {
                m_alphaPipeline->flush("flush::handlePendingEventInLoop");
                m_iBox->invalidateDecodeCache();
            }
            DEBUG_LOG(QString("CPU %1: Code modification handled PC=0x%2-0x%3")
                .arg(m_cpuId)
                .arg(m_pendingEvent.startPC, 16, 16, QChar('0'))
                .arg(m_pendingEvent.endPC, 16, 16, QChar('0')));
            break;

        case PendingEventType::CacheInvalidation:
            if (m_iBox) {
                m_iBox->invalidateDecodeCache();
            }
            DEBUG_LOG(QString("CPU %1: Cache invalidation handled").arg(m_cpuId));
            break;

        default:
            WARN_LOG(QString("CPU %1: Unknown pending event type %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(m_pendingEvent.type)));
            break;
        }

        m_pendingEvent.clear();
    }

    void shutdownGracefully() noexcept {
        DEBUG_LOG(QString("CPU %1: Beginning graceful shutdown").arg(m_cpuId));

        if (m_alphaPipeline) {
            try {
                m_alphaPipeline->flush("flush::shutdownGraceFully");
                DEBUG_LOG(QString("CPU %1: Pipeline flushed").arg(m_cpuId));
            }
            catch (const std::exception& e) {
                ERROR_LOG(QString("CPU %1: Exception during pipeline flush: %2")
                    .arg(m_cpuId).arg(e.what()));
            }
        }

        m_pendingEvent.clear();

        if (m_halted.load(std::memory_order_acquire)) {
            emit halted(m_cpuId, 0);
        }

        DEBUG_LOG(QString("CPU %1: Graceful shutdown complete").arg(m_cpuId));
    }

    bool checkForWakeup() noexcept {
        // TODO: Implement full wake-up logic
        return checkForWakeupInterrupts();
    }

    // Halt stubs (TODO)
    void haltCPU() noexcept { /* TODO */ }
    void setHaltCode(quint8 code) noexcept { /* TODO */ }
    void setHalted(bool state) noexcept { /* TODO */ }
    void notifyHalt() noexcept { /* TODO */ }
    void haltUntilSRMExit() noexcept { /* TODO */ }

    // Error handlers
    void handleWarning(const QString& reason) noexcept;
    void handleRecoverableError(const QString& reason) noexcept;
    void handleFatalError(const QString& reason) noexcept;
    void handleMachineCheck(const QString& reason) noexcept;
};

#endif // AlphaCPU_h__
