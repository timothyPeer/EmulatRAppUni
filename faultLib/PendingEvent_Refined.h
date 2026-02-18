// ============================================================================
// PendingEvent_Refined.h
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   Unified event structure for all exceptions, interrupts, and machine checks.
//   Used by pipeline to communicate events to FaultDispatcher/PAL handler.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef PENDINGEVENT_REFINED_H
#define PENDINGEVENT_REFINED_H

#include <QtGlobal>
#include <QDateTime>
#include <QString>
#include <chrono>
#include "../coreLib/types_core.h"
#include "fault_core.h"
#include "../palLib_EV6/PalVectorId_refined.h"
#include "../memoryLib/memory_core.h"
#include "../coreLib/enum_MCES.h"
#include "../exceptionLib/ExceptionClass_EV6.h"
#include "../exceptionLib/PendingEventKind.h"
#include "../coreLib/memory_enums_structs.h"
#include "../coreLib/stdLibCore.h"

// ============================================================================
// Event Classification Enums
// ============================================================================

/**
 * @brief High-level event classification
 */
enum class EventClass : quint8 {
    None = 0,
    Exception,          ///< Synchronous exception (fault/trap)
    Interrupt,          ///< Asynchronous interrupt (hardware/software)
    MachineCheck,       ///< Machine check error
    Reset,              ///< System/CPU reset
    InternalError,      ///< Internal processor error
    SystemEvent         ///< System-level event
};

/**
 * @brief Event priority for dispatching
 */
enum class EventPriority : quint8 {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,       ///< Machine checks, double faults
    Reset = 4           ///< Highest priority
};

// ============================================================================
// PendingPropertyInfo - Detailed Event Properties
// ============================================================================

struct PendingPropertyInfo final {
    // Access type flags
    bool isWrite{ false };
    bool isExecute{ false };
    bool isUnaligned{ false };
    bool isInstruction{ false };        // ITB vs DTB

    // Fault type flags
    bool isMachineCheck{ false };
    bool isAccessViolation{ false };
    bool isInvalidPTE{ false };
    bool isIllegalInstruction{ false };
    bool isNonCanonical{ false };
    bool isFaultOnExecute{ false };
    bool isFaultOnWrite{ false };
    bool isFaultOnRead{ false };
    bool isDoubleMiss{ false };

    // Associated data
    quint64 physicalAddress{ 0 };
    quint64 pteValue{ 0 };
    quint64 mchkSummary{ 0 };
    quint64 irqSummary{ 0 };
    quint64 irqVector{ 0 };
    quint64 logoutFrame{ 0 };
    quint64 pmCounterIndex{ 0 };

    // Categorization
    MachineCheckReason machineCheckReason{ MachineCheckReason::NONE };
    MemoryFaultType    faultType{ MemoryFaultType::NONE };
    MemoryAccessType   accessType{ MemoryAccessType::READ };
    TrapCode_Class     trapCode{ TrapCode_Class::NONE };

    // SMP fault info
    CPUIdType initiatingCpu{ 0 };       ///< CPU that initiated operation
    qsizetype participatingCpus{ 0 };   ///< CPUs participating in operation
    qsizetype acknowledgedCpus{ 0 };    ///< CPUs that acknowledged
    bool isSmpRendezvousFailure{ false };
};

// ============================================================================
// PendingEvent - Unified Exception/Interrupt/MachineCheck Event
// ============================================================================

/**
 * @brief Single structure for all pipeline events flowing into FaultDispatcher
 *
 * Design principle:
 *   - Pipeline code creates PendingEvent with ExceptionClass
 *   - FaultDispatcher resolves PalVectorId during preparation
 *   - All sync/async events use this structure
 */
struct PendingEvent final {
    // ====================================================================
    // Core Event Identification (NEW - Multi-CPU Support)
    // ====================================================================

    CPUIdType        cpuId{ 0 };                    ///< CPU this event belongs to
    EventClass       eventClass{ EventClass::None }; ///< High-level classification
    EventPriority    priority{ EventPriority::Normal }; ///< Dispatch priority
    QString          description;                    ///< Human-readable description

    // ====================================================================
    // Event Classification (Existing)
    // ====================================================================

    PendingEventKind kind{ PendingEventKind::None };
    ExceptionClass_EV6 exceptionClass{ ExceptionClass_EV6::None };

    // PAL vector (resolved during preparation, not at creation)
    PalVectorId_EV6 palVectorId{ PalVectorId_EV6::INVALID };

    // ====================================================================
    // Address/Context Info
    // ====================================================================

    quint64 faultVA{ 0 };      ///< Virtual address of fault
    quint64 dtbFaultVA{ 0 };   ///< DTB Miss Double
    quint64 faultPC{ 0 };      ///< Faulting PC
    ASNType asn{ 0 };          ///< Address space number
    CMType  cm{ 0 };           ///< Current mode at time of event

    // ====================================================================
    // Instruction Info
    // ====================================================================

    quint8  palFunction{ 0 };  ///< PAL function op{00.}fn
    quint8  opcode{ 0 };       ///< Opcode
    quint32 palFunc{ 0 };      ///< Encoded PAL instruction code

    // ====================================================================
    // Registers
    // ====================================================================

    quint64 palR16{ 0 };
    quint64 palR17{ 0 };
    quint64 destReg{ 0 };      ///< Destination register (unaligned)

    // ====================================================================
    // Multi-Purpose Fields
    // ====================================================================

    // Multi-purpose field:
    // - CALL_PAL: PAL function code (0x00-0xBF)
    // - GENTRAP: trap code
    // - ARITHMETIC: EXC_SUM bits
    // - MCHK: machine check syndrome
    quint64 extraInfo{ 0 };
    quint64 eventOperand{ 0 }; ///< Operand of the event

    // ====================================================================
    // Interrupt-Specific
    // ====================================================================

    quint64 deviceInterruptVector{ 0 }; ///< Device interrupt vector
    quint32 hwVector{ 0 };              ///< Hardware interrupt vector
    quint8  hwIPL{ 0 };                 ///< Hardware IPL
    quint8  swiLevel{ 0 };              ///< Software interrupt level
    quint8  astMode{ 0 };               ///< AST target mode
    quint8  astsr{ 0 };

    // ====================================================================
    // D-Stream Fault Specifics
    // ====================================================================

    enum class DStreamFaultType : quint8 {
        None,
        DTB_MISS_SINGLE,
        DTB_MISS_DOUBLE_3,
        DTB_MISS_DOUBLE_4,
        DFAULT_ACV,
        DFAULT_FOE,
        DFAULT_FOW,
        DFAULT_FOR,
        UNALIGN
    };

    DStreamFaultType dstreamType{ DStreamFaultType::None };
    quint8 mmAccessType{ 0 };   ///< Read/Write/Execute
    quint8 mmFaultReason{ 0 };  ///< ACV/FOE/FOW/FOR

    // ====================================================================
    // Arithmetic Fault Specifics
    // ====================================================================

    quint64 exc_Sum{ 0 };       ///< Arithmetic exception summary
    quint64 exc_Mask{ 0 };      ///< Arithmetic exception mask

    // ====================================================================
    // Machine Check Specifics
    // ====================================================================

    MachineCheckReason mcReason{ MachineCheckReason::NONE };
    quint64 mchkCode{ 0 };      ///< Machine check code
    quint64 mchkAddr{ 0 };      ///< Physical address of error

    // ====================================================================
    // Detailed Event Properties
    // ====================================================================

    PendingPropertyInfo pendingEvent_Info;

    // ====================================================================
    // Timestamp
    // ====================================================================

    EventTimestamp timestamp{ std::chrono::steady_clock::now() };

    // ====================================================================
    // Constants
    // ====================================================================

    static constexpr quint8 getIPL_UNSPECIFIED() noexcept { return 31; }

    // ====================================================================
    // Validation Helpers
    // ====================================================================

    /**
     * @brief Check if PAL vector has been resolved
     */
    inline bool isResolved() const noexcept {
        return palVectorId != PalVectorId_EV6::INVALID;
    }

    /**
     * @brief Check if this is a valid pending event
     */
    inline bool isValid() const noexcept {
        return kind != PendingEventKind::None &&
            exceptionClass != ExceptionClass_EV6::None;
    }

    /**
     * @brief Check if this is a memory-related fault
     */
    inline bool isMemoryFault() const noexcept {
        return pendingEvent_Info.faultType != MemoryFaultType::NONE;
    }

    /**
     * @brief Check if this is an ITB fault
     */
    inline bool isITBFault() const noexcept {
        return pendingEvent_Info.isInstruction && isMemoryFault();
    }

    /**
     * @brief Check if this is a DTB fault
     */
    inline bool isDTBFault() const noexcept {
        return !pendingEvent_Info.isInstruction && isMemoryFault();
    }

    /**
     * @brief Check if this is a machine check
     */
    inline bool isMachineCheck() const noexcept {
        return eventClass == EventClass::MachineCheck ||
            exceptionClass == ExceptionClass_EV6::MachineCheck;
    }

    /**
     * @brief Check if this is a critical event
     */
    inline bool isCritical() const noexcept {
        return priority == EventPriority::Critical ||
            priority == EventPriority::Reset;
    }

    // ====================================================================
    // State Management
    // ====================================================================

    /**
     * @brief Clear the event to initial state
     */
    inline void clear() noexcept {
        cpuId = 0;
        eventClass = EventClass::None;
        priority = EventPriority::Normal;
        description.clear();

        kind = PendingEventKind::None;
        exceptionClass = ExceptionClass_EV6::None;
        palVectorId = PalVectorId_EV6::INVALID;

        faultVA = 0;
        dtbFaultVA = 0;
        faultPC = 0;
        asn = 0;
        cm = 0;

        palFunction = 0;
        opcode = 0;
        palFunc = 0;

        palR16 = 0;
        palR17 = 0;
        destReg = 0;

        extraInfo = 0;
        eventOperand = 0;

        deviceInterruptVector = 0;
        hwVector = 0;
        hwIPL = 0;
        swiLevel = 0;
        astMode = 0;
        astsr = 0;

        dstreamType = DStreamFaultType::None;
        mmAccessType = 0;
        mmFaultReason = 0;

        exc_Sum = 0;
        exc_Mask = 0;

        mcReason = MachineCheckReason::NONE;
        mchkCode = 0;
        mchkAddr = 0;

        pendingEvent_Info = PendingPropertyInfo{};
        timestamp = std::chrono::steady_clock::now();
    }

    /**
     * @brief Set event properties from TrapCode
     */
    void setPendingFromTrapCode_Class(TrapCode_Class tc) noexcept {
        switch (tc) {
        case TrapCode_Class::NONE:
            break;

        case TrapCode_Class::FP_OVERFLOW:
        case TrapCode_Class::INTEGER_OVERFLOW:
        case TrapCode_Class::ARITHMETIC_TRAP:
            exceptionClass = ExceptionClass_EV6::Arithmetic;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::DTB_MISS:
            exceptionClass = ExceptionClass_EV6::Dtb_miss_single;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::DTB_FAULT:
        case TrapCode_Class::DTB_ACCESS_VIOLATION:
            exceptionClass = ExceptionClass_EV6::Dfault;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::FP_DISABLED:
            exceptionClass = ExceptionClass_EV6::Fen;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::OPCODE_RESERVED:
        case TrapCode_Class::ILLEGAL_INSTRUCTION:
            exceptionClass = ExceptionClass_EV6::OpcDec;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::ITB_ACCESS_VIOLATION:
            exceptionClass = ExceptionClass_EV6::ItbAcv;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::ITB_FAULT:
        case TrapCode_Class::ITB_MISS:
            exceptionClass = ExceptionClass_EV6::ItbMiss;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::ITB_MISALIGN_FAULT:
        case TrapCode_Class::UN_ALIGNED:
            exceptionClass = ExceptionClass_EV6::Unalign;
            eventClass = EventClass::Exception;
            break;

        case TrapCode_Class::MACHINE_CHECK:
            exceptionClass = ExceptionClass_EV6::MachineCheck;
            eventClass = EventClass::MachineCheck;
            priority = EventPriority::Critical;
            break;

        default:
            break;
        }
    }
};

#endif // PENDINGEVENT_REFINED_H