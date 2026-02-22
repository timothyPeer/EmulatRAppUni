// ============================================================================
// cpu_struct_inline.h - CPU Access Information
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

#pragma once
#include <QtGlobal>


// CPU Access Information
struct CPUAccessInfo
{
	quint16 cpuId = 0;
	qint64 lastAccessTime = 0;
	quint64 accessCount = 0;
	bool hasReservation = false;
	quint64 reservationAddr = 0;
};