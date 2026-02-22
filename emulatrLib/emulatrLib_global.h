// ============================================================================
// emulatrLib_global.h - emulatr Lib global
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

#ifndef EMULATRLIB_GLOBAL_H
#define EMULATRLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(EMULATRLIB_LIBRARY)
#define EMULATRLIB_EXPORT Q_DECL_EXPORT
#else
#define EMULATRLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // EMULATRLIB_GLOBAL_H
