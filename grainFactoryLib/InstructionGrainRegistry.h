// ============================================================================
// InstructionGrainRegistry.h - Template helper for automatic grain registration
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

#ifndef _EMULATRAPPUNI_GRAINFACTORYLIB_INSTRUCTIONGRAINREGISTRY_H
#define _EMULATRAPPUNI_GRAINFACTORYLIB_INSTRUCTIONGRAINREGISTRY_H

#include <QtCore/QHash>
#include <QtCore/QDebug>
#include <memory>
#include "grainFactoryLib/InstructionGrain.h"
#include "coreLib/LoggingMacros.h"
#include "grainFactoryLib/InstructionGrain_core.h"

#include <QHash>
#include <QList>


// ============================================================================
// Composite key: (opcode << 24) | (platform << 16) | functionCode
// ============================================================================
inline constexpr quint32 makeGrainKey(quint8 op, quint16 func, GrainPlatform flavor) noexcept
{
    const quint32 op32 = static_cast<quint32>(op);
    const quint32 f32 = static_cast<quint32>(func) & 0xFFFF;
    const quint32 plat = static_cast<quint32>(flavor) & 0xFF;
    return (op32 << 24) | (plat << 16) | f32;
}

inline constexpr quint32 makeGrainKey(quint8 op, quint16 func) noexcept
{
    return makeGrainKey(op, func, GrainPlatform::Alpha);
}

// ============================================================================
// InstructionGrain Global Registry
// ============================================================================

// ============================================================================
// InstructionGrain Global Registry
// ============================================================================
class InstructionGrainRegistry
{
public:
    static InstructionGrainRegistry& instance()
    {
        static InstructionGrainRegistry inst;
        return inst;
    }

    // Destructor - clean up owned grains
    ~InstructionGrainRegistry()
    {
        for (auto grain : m_ownedGrains) {
            delete grain;
        }
    }

    // New method for auto-registration (takes ownership via unique_ptr)
    void registerGrain(quint8 opcode, quint16 func, std::unique_ptr<InstructionGrain> grain)
    {
        const quint32 key = makeGrainKey(opcode, func, GrainPlatform::Alpha);
        InstructionGrain* rawPtr = grain.release();  // Release ownership from unique_ptr
        m_table[key] = rawPtr;                       // Store in registry
        m_ownedGrains.append(rawPtr);                // Track for cleanup

        TRACE_LOG(QString("Registered grain: opcode=0x%1 func=0x%2")
            .arg(opcode, 2, 16, QChar('0'))
            .arg(func, 4, 16, QChar('0')));
    }

    // Legacy method (for backward compatibility with raw pointers)
    void add(InstructionGrain* grain)
    {
        if (!grain) return;

        const quint32 key = makeGrainKey(grain->opcode(), grain->functionCode(), grain->platform());
        m_table[key] = grain;

        TRACE_LOG(QString("Registered grain (legacy): opcode=0x%1 func=0x%2")
            .arg(grain->opcode(), 2, 16, QChar('0'))
            .arg(grain->functionCode(), 4, 16, QChar('0')));
    }

    // Lookup methods
    InstructionGrain* lookup(quint8 opcode, quint16 func)
    {
        return lookup(opcode, func, GrainPlatform::Alpha);
    }

    InstructionGrain* lookup(quint8 opcode, quint16 func, GrainPlatform platform)
    {
        quint32 key = makeGrainKey(opcode, func, platform);

        // PAL HW opcodes: single grain handles all function codes
        if (opcode >= 0x19 && opcode <= 0x1F && opcode != 0x1A)
            func = 0;

        if (opcode == 0x1D)
            qDebug() << "break";

        auto it = m_table.find(key);
        if (it != m_table.end()) {
            return it.value();
        }



        // Fallback: Try NONE platform
        if (platform != GrainPlatform::Alpha) {
            return lookup(opcode, func, GrainPlatform::Alpha);
        }

        return nullptr;
    }

    qsizetype grainCount() const noexcept
    {
        return m_table.count();
    }

private:
    InstructionGrainRegistry() = default;

    // Prevent copying/moving
    InstructionGrainRegistry(const InstructionGrainRegistry&) = delete;
    InstructionGrainRegistry& operator=(const InstructionGrainRegistry&) = delete;

    QHash<quint32, InstructionGrain*> m_table;     // Grain registry
    QList<InstructionGrain*> m_ownedGrains;        // Track owned grains for cleanup
};

// ============================================================================
// GrainAutoRegistrar - Automatic Grain Registration Helper
// ============================================================================
/**
 * @brief Template helper for automatic grain registration
 *
 * Usage: Create a static instance in grain header to auto-register:
 *   namespace {
 *       static GrainAutoRegistrar<MyGrain> s_my_grain_registrar(0x10, 0x20);
 *   }
 *
 * The constructor executes before main(), registering the grain with the
 * global InstructionGrainRegistry.
 */
 // ============================================================================
 // GrainAutoRegistrar - Automatic Registration Template
 // ============================================================================
template<typename GrainType>
class GrainAutoRegistrar
{
public:
    GrainAutoRegistrar(quint8 opcode, quint16 functionCode)
    {
        auto grain = std::make_unique<GrainType>();
        InstructionGrainRegistry::instance().registerGrain(
            opcode,
            functionCode,
            std::move(grain)
        );
    }

    // Prevent copying/moving
    GrainAutoRegistrar(const GrainAutoRegistrar&) = delete;
    GrainAutoRegistrar& operator=(const GrainAutoRegistrar&) = delete;
};

#endif // _EMULATRAPPUNI_GRAINFACTORYLIB_INSTRUCTIONGRAINREGISTRY_H