// ============================================================================
// global_PalVectorTable.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global PalVectorTable singleton accessor implementation
// ============================================================================

#include "global_PalVectorTable.h"
#include "PalVectorTable_final.h"

// ============================================================================
// Global Accessor Implementation
// ============================================================================

PalVectorTable& global_PalVectorTable() noexcept
{
    // Delegate to PalVectorTable's singleton instance() method
    return PalVectorTable::instance();
}
