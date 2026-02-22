// ============================================================================
// cBox_core.h - c Box core
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

#ifndef CBOX_CORE_H
#define CBOX_CORE_H
#include <QtGlobal>

enum class SerializationType {
    Barrier_MB,		// MB
	Barrier_TRAP ,  // TRAPB
	Barrier_WRITE,  // WMB  
	Barrier_EXC,     // EXCB
	Barrier_NONE	// None
};

#endif // CBOX_CORE_H
