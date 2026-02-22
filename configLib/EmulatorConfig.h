// ============================================================================
// EmulatorConfig.h - High-Level Initialization Config
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

// ============================================================================
//  EmulatorConfig.h - High-Level Initialization Config
//
//  Purpose: Simple config struct for initialize() method
// ============================================================================

#ifndef EMULATORCONFIG_H
#define EMULATORCONFIG_H

#include <QString>

/**
 * @brief High-level emulator initialization configuration.
 *
 * Passed to EmulatR_init::initialize() to control startup behavior.
 */
struct EmulatorConfig {
    QString configPath{"emulatr/config"};   // Configuration directory
    QString logPath{ "/emulatr/log" };      // directory into which the application is stored.
    QString binPath{ "/emulatr/bin" };      // directory into which the application is stored.
    bool enableLogging{true};                   // Enable logging system
    bool enableConsoles{true};                  // Enable OPA consoles
    bool enableDevices{true};                   // Enable device emulation

    // Future expansion
    bool enableNetworking{false};
    bool enableGUI{false};
};

#endif // EMULATORCONFIG_H
