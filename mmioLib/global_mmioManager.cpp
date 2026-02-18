// ============================================================================
// global_mmioManager.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global MMIOManager singleton implementation
// ============================================================================

#include "global_mmioManager.h"
#include "mmio_Manager.h"

MMIOManager& global_MMIOManager() noexcept
{
    static MMIOManager instance;
    return instance;
}
