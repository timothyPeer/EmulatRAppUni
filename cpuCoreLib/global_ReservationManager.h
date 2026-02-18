// ============================================================================
// global_ReservationManager.h - Global Accessor
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// ============================================================================
// global_ReservationManager.h
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// ============================================================================

#ifndef GLOBAL_RESERVATIONMANAGER_H
#define GLOBAL_RESERVATIONMANAGER_H

// Forward declaration only - no includes!
class ReservationManager;

/**
 * @brief Global accessor for ReservationManager
 *
 * Returns reference to ReservationManager owned by ExecutionCoordinator.
 * This split pattern avoids circular dependencies.
 */
ReservationManager& globalReservationManager() noexcept;

#endif // GLOBAL_RESERVATIONMANAGER_H