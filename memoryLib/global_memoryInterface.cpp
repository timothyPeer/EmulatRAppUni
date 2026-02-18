// ============================================================================
// global_memoryInterface.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global memory interface implementation
// ============================================================================

#include "global_memoryInterface.h"
#include "../memoryLib/GuestMemory.h"
#include "../cpuCoreLib/global_ReservationManager.h"
#include "cpuCoreLib/ReservationManager.h"

// ============================================================================
// Global Memory Accessor
// ============================================================================

GuestMemory& globalBoxMemory() noexcept
{
    static GuestMemory instance;
    return instance;
}


MEM_STATUS writePA(quint64 pa, const void* data, quint32 size) noexcept
{
    MEM_STATUS status = globalBoxMemory().writePA(pa, data, size);
    if (status == MEM_STATUS::Ok) {
        // For multi-byte writes, break reservations on all affected cache lines
        quint64 startLine = pa & ~63ULL;
        quint64 endLine = (pa + size - 1) & ~63ULL;

        // Most writes fit in single cache line
        globalReservationManager().breakReservationsOnCacheLine(startLine);

        // If write spans cache lines, break both
        if (endLine != startLine) {
            globalReservationManager().breakReservationsOnCacheLine(endLine);
        }
    }
    return status;
}

// ============================================================================
// Convenience Functions
// ============================================================================

MEM_STATUS readPA_Quad(quint64 pa, quint64& outValue) noexcept
{
    return globalBoxMemory().readPA(pa, &outValue, 8);
}

MEM_STATUS writePA_Quad(quint64 pa, quint64 value) noexcept
{
    MEM_STATUS status = globalBoxMemory().writePA(pa, &value, 8);
    if (status == MEM_STATUS::Ok) {
        globalReservationManager().breakReservationsOnCacheLine(pa);
    }
    return status;
}

MEM_STATUS readPA_Long(quint64 pa, quint32& outValue) noexcept
{
    return globalBoxMemory().readPA(pa, &outValue, 4);
}

MEM_STATUS writePA_Long(quint64 pa, quint32 value) noexcept
{
    // Step 1: Perform the write
    MEM_STATUS status = globalBoxMemory().writePA(pa, &value, 4);

    // Step 2: On success, break reservations (cache coherency)
    if (status == MEM_STATUS::Ok) {
        globalReservationManager().breakReservationsOnCacheLine(pa);
    }

    return status;
}

MEM_STATUS readPA_Word(quint64 pa, quint16& outValue) noexcept
{
    return globalBoxMemory().readPA(pa, &outValue, 2);
}

MEM_STATUS writePA_Word(quint64 pa, quint16 value) noexcept
{
    MEM_STATUS status = globalBoxMemory().writePA(pa, &value, 2);
    if (status == MEM_STATUS::Ok) {
        globalReservationManager().breakReservationsOnCacheLine(pa);
    }
    return status;
}

MEM_STATUS readPA_Byte(quint64 pa, quint8& outValue) noexcept
{
    return globalBoxMemory().readPA(pa, &outValue, 1);
}

MEM_STATUS writePA_Byte(quint64 pa, quint8 value) noexcept
{
    MEM_STATUS status = globalBoxMemory().writePA(pa, &value, 1);
    if (status == MEM_STATUS::Ok) {
        globalReservationManager().breakReservationsOnCacheLine(pa);
    }
    return status;
}