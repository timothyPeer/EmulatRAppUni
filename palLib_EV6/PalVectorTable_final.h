#ifndef PALVECTORTABLE_H
#define PALVECTORTABLE_H
// ============================================================================
// PalVectorTable.h - Header-Only Implementation
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   PAL vector table - header-only implementation for zero overhead.
//   Manages PAL entry points, exception translation, and handler dispatch.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================



#include <QtGlobal>
#include <QHash>
#include <functional>
#include "PAL_core.h"
#include "PalVectorId_refined.h"
#include "../coreLib/Axp_Attributes_core.h"
#include "../exceptionLib/ExceptionClass_EV6.h"
#include "../coreLib/LoggingMacros.h"
#include "PalArgumentPack_str.h"

// Forward declarations (only what's needed for pointers/references)


// ============================================================================
// HANDLER FUNCTION TYPE
// ============================================================================
/**
 * @brief PAL handler function signature.
 *
 * Handlers receive PAL arguments and CPU ID, return PalResult.
 */
using PalHandlerFunc = std::function<PalResult(PalArgumentPack&, CPUIdType)>;


// ============================================================================
// PalVectorTable - Header-Only Implementation
// ============================================================================

class PalVectorTable final
{
public:

    // Private constructor for singleton
    PalVectorTable() = default;
    ~PalVectorTable() = default;

    // ====================================================================
    // Singleton Access
    // ====================================================================

    static AXP_HOT AXP_FLATTEN PalVectorTable& instance() noexcept {
        static PalVectorTable singleton;
        return singleton;
    }

    // ====================================================================
    // PAL Base Address Management
    // ====================================================================

    AXP_HOT AXP_FLATTEN void bindPALBase(quint64 palBase) noexcept {
        m_palBase = palBase;

        // Recompute absolute entry PCs for all registered vectors
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
            quint16 offset = it.key();
            it.value().entryPC = m_palBase + offset;
        }

        DEBUG_LOG(QString("PAL vector table bound to base 0x%1")
            .arg(palBase, 16, 16, QChar('0')));
    }

    AXP_HOT AXP_FLATTEN quint64 palBase() const noexcept {
        return m_palBase;
    }

    // ====================================================================
    // Vector Registration
    // ====================================================================

    AXP_HOT AXP_ALWAYS_INLINE void registerVector(
        PalVectorId_EV6 vec,
        quint8 targetIPL,
        quint8 requiredCM,
        quint32 flags,
        const char* name = nullptr) noexcept
    {
        quint16 offset = static_cast<quint16>(vec);

        PalVectorEntry entry;
        entry.vectorId = vec;
        entry.entryPC = m_palBase ? (m_palBase + offset) : 0;
        entry.targetIPL = targetIPL;
        entry.requiredCM = requiredCM;
        entry.flags = flags;
        entry.name = name;

        m_entries.insert(offset, entry);
    }

    // ====================================================================
    // Handler Registration
    // ====================================================================

    AXP_HOT AXP_FLATTEN void registerHandler(quint8 palCallNumber, PalHandlerFunc handler) noexcept {
        if (handler) {
            m_handlers.insert(palCallNumber, handler);
            TRACE_LOG(QString("PAL handler registered: 0x%1")
                .arg(palCallNumber, 2, 16, QChar('0')));
        }
    }

    AXP_HOT AXP_FLATTEN void unregisterHandler(quint8 palCallNumber) noexcept {
        m_handlers.remove(palCallNumber);
    }

    AXP_HOT AXP_FLATTEN PalHandlerFunc getHandler(quint8 palCallNumber) const noexcept {
        auto it = m_handlers.constFind(palCallNumber);
        return (it == m_handlers.constEnd()) ? nullptr : it.value();
    }

    AXP_HOT AXP_FLATTEN bool hasHandler(quint8 palCallNumber) const noexcept {
        return m_handlers.contains(palCallNumber);
    }

    AXP_HOT AXP_FLATTEN bool executeHandler(
        quint8 palCallNumber,
        PalArgumentPack& args,
        CPUIdType cpuId,  // Changed from AlphaProcessorContext*
        PalResult& result) const noexcept
    {
        auto handler = getHandler(palCallNumber);
        if (!handler) {
            return false;
        }

        result = handler(args, cpuId);  // Pass CPUIdType instead of context pointer
        return true;
    }

    // ====================================================================
    // Vector Lookup
    // ====================================================================

    AXP_HOT AXP_FLATTEN const PalVectorEntry* lookup(PalVectorId_EV6 vec) const noexcept {
        quint16 offset = static_cast<quint16>(vec);
        auto it = m_entries.constFind(offset);
        return (it == m_entries.constEnd()) ? nullptr : &it.value();
    }

    // ====================================================================
    // Exception Translation
    // ====================================================================

    AXP_HOT AXP_FLATTEN static PalVectorId_EV6 mapException(ExceptionClass_EV6 exClass) noexcept {
        switch (exClass) {
        case ExceptionClass_EV6::Reset:
            return PalVectorId_EV6::RESET;
        case ExceptionClass_EV6::MachineCheck:
            return PalVectorId_EV6::MCHK;
        case ExceptionClass_EV6::Arithmetic:
            return PalVectorId_EV6::ARITH;
        case ExceptionClass_EV6::Interrupt:
            return PalVectorId_EV6::INTERRUPT;
        case ExceptionClass_EV6::ItbMiss:
            return PalVectorId_EV6::ITB_MISS;
        case ExceptionClass_EV6::ItbAcv:
            return PalVectorId_EV6::ITB_ACV;
        case ExceptionClass_EV6::OpcDec:
        case ExceptionClass_EV6::IllegalInstruction:
        case ExceptionClass_EV6::SubsettedInstruction:
            return PalVectorId_EV6::OPCDEC;
        case ExceptionClass_EV6::Fen:
            return PalVectorId_EV6::FEN;
        case ExceptionClass_EV6::CallPal:
            return PalVectorId_EV6::CALL_CENTRY_BEG;
        case ExceptionClass_EV6::Unalign:
            return PalVectorId_EV6::UNALIGN;
        case ExceptionClass_EV6::Dfault:
        case ExceptionClass_EV6::MemoryFault:
        case ExceptionClass_EV6::DStream:
            return PalVectorId_EV6::DTB_MISS_NATIVE;
        case ExceptionClass_EV6::Dtb_miss_single:
            return PalVectorId_EV6::DTB_MISS_SINGLE;
        case ExceptionClass_EV6::Dtb_miss_double_4:
            return PalVectorId_EV6::DTB_MISS_DOUBLE;
        case ExceptionClass_EV6::SoftwareTrap:
        case ExceptionClass_EV6::General:
        case ExceptionClass_EV6::BreakPoint:
            return PalVectorId_EV6::CALL_CENTRY_BEG;
        case ExceptionClass_EV6::Panic:
            return PalVectorId_EV6::CallPal_01;
        case ExceptionClass_EV6::SystemService:
            return PalVectorId_EV6::CALL_CENTRY_BEG;
        case ExceptionClass_EV6::MT_FPCR:
            return PalVectorId_EV6::ARITH;
        case ExceptionClass_EV6::None:
        default:
            return PalVectorId_EV6::CallPal_01;
        }
    }

    // ====================================================================
    // Initialization
    // ====================================================================

    AXP_HOT AXP_FLATTEN void initialize() noexcept {
        m_entries.clear();
        m_handlers.clear();

        // Hardware exception vectors
        registerVector(PalVectorId_EV6::RESET, 7, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::MODIFIES_IPL, "RESET");
        registerVector(PalVectorId_EV6::ITB_ACV, 0, 0,
            PalVectorEntry::SAVES_STATE, "IACCVIO");
        registerVector(PalVectorId_EV6::INTERRUPT, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::MODIFIES_IPL, "INTERRUPT");
        registerVector(PalVectorId_EV6::ITB_MISS, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::RESTARTABLE, "ITB_MISS");
        registerVector(PalVectorId_EV6::DTB_MISS_SINGLE, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::RESTARTABLE, "DTB_MISS_SINGLE");
        registerVector(PalVectorId_EV6::DTB_MISS_DOUBLE, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::RESTARTABLE, "DTB_MISS_DOUBLE");
        registerVector(PalVectorId_EV6::UNALIGN, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::RESTARTABLE, "UNALIGN");
        registerVector(PalVectorId_EV6::DTB_MISS_NATIVE, 0, 0,
            PalVectorEntry::SAVES_STATE, "DFAULT");
        registerVector(PalVectorId_EV6::MCHK, 31, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::MODIFIES_IPL, "MCHK");
        registerVector(PalVectorId_EV6::OPCDEC, 0, 0,
            PalVectorEntry::SAVES_STATE, "OPCDEC");
        registerVector(PalVectorId_EV6::ARITH, 0, 0,
            PalVectorEntry::SAVES_STATE | PalVectorEntry::RESTARTABLE, "ARITH");
        registerVector(PalVectorId_EV6::FEN, 0, 0,
            PalVectorEntry::SAVES_STATE, "FEN");

        // CALL_PAL vectors
        registerVector(PalVectorId_EV6::CALL_CENTRY_BEG, 0, 3,
            PalVectorEntry::NONE, "CALL_PAL_ENTRY");
        registerVector(PalVectorId_EV6::CallPal_01, 0, 3,
            PalVectorEntry::NONE, "BUGCHECK");
        registerVector(PalVectorId_EV6::CallPal_02, 0, 3,
            PalVectorEntry::NONE, "GENTRAP");

        DEBUG_LOG("PAL vector table initialized");
    }

    AXP_HOT AXP_FLATTEN void clear() noexcept {
        m_entries.clear();
        m_handlers.clear();
    }

    // ====================================================================
    // Diagnostics
    // ====================================================================

    AXP_HOT AXP_FLATTEN int count() const noexcept {
        return m_entries.count();
    }

    AXP_HOT AXP_FLATTEN int handlerCount() const noexcept {
        return m_handlers.count();
    }

    AXP_HOT AXP_FLATTEN bool isRegistered(PalVectorId_EV6 vec) const noexcept {
        return m_entries.contains(static_cast<quint16>(vec));
    }

private:


    Q_DISABLE_COPY(PalVectorTable)

        quint64 m_palBase{ 0 };
    QHash<quint16, PalVectorEntry> m_entries;    // Vector metadata
    QHash<quint8, PalHandlerFunc> m_handlers;    // Handler functions
};

#endif // PALVECTORTABLE_H
