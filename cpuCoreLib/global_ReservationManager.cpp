// ============================================================================
// global_ReservationManager.cpp
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#include "global_ReservationManager.h"
#include "../emulatrLib/global_ExecutionCoordinator.h"
#include "emulatrLib/ExecutionCoordinator.h"
#include "cpuCoreLib/ReservationManager.h"

static ReservationManager* g_reservationManager = nullptr;

// ============================================================================
// Global Accessor Implementation
// ============================================================================

ReservationManager& globalReservationManager() noexcept
{
    Q_ASSERT(g_reservationManager != nullptr && "ReservationManager not initialized!");
    return *g_reservationManager;
}
// ============================================================================
// Initialization function (call during system startup)
// ============================================================================

void initializeReservationManager(quint16 cpuCount) noexcept
{
    Q_ASSERT(g_reservationManager == nullptr && "ReservationManager already initialized!");
    g_reservationManager = new ReservationManager(cpuCount);
}

void shutdownReservationManager() noexcept
{
    delete g_reservationManager;
    g_reservationManager = nullptr;
}
