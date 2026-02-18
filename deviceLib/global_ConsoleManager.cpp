// ============================================================================
// global_ConsoleManager.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global ConsoleManager singleton implementation
// ============================================================================

#include "global_ConsoleManager.h"
#include "ConsoleManager.h"

ConsoleManager& global_ConsoleManager() noexcept
{
    static ConsoleManager instance;
    return instance;
}
