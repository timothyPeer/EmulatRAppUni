#ifndef DECODEMMSTATEFAULTTYPE_H
#define DECODEMMSTATEFAULTTYPE_H


// ============================================================================
// MM_STAT Register Decoder - Maps hardware encoding to your fault types
// ============================================================================

/// \brief Decode MM_STAT fault type bits to MemoryFaultType
/// \param faultTypeBits Raw bits [10:8] from MM_STAT register
/// \param isWrite Bit [0] from MM_STAT indicating write access
#include "fault_core.h"


inline MemoryFaultType decodeMMStatFaultType(quint8 faultTypeBits, bool isWrite) noexcept
{
    // EV6 MM_STAT encoding (bits [10:8]):
    // 0 = DTB miss (single level)
    // 1 = DTB miss (double level)
    // 2 = DTB miss (triple level)
    // 3 = Access violation
    // 4 = Fault-on-read
    // 5 = Fault-on-write
    // 6 = Fault-on-execute
    // 7 = Reserved

    switch (faultTypeBits) {
    case 0:
    case 1:
    case 2:
        // DTB miss - distinguish read vs write using bit [0]
        return isWrite ? MemoryFaultType::DTB_MISS_WRITE
                       : MemoryFaultType::DTB_MISS_READ;

    case 3:
        // Access violation - distinguish read vs write
        return isWrite ? MemoryFaultType::DTB_ACCESS_VIOLATION_WRITE
                       : MemoryFaultType::DTB_ACCESS_VIOLATION_READ;

    case 4:
        return MemoryFaultType::FAULT_ON_READ;

    case 5:
        return MemoryFaultType::FAULT_ON_WRITE;

    case 6:
        return MemoryFaultType::FAULT_ON_EXECUTE;

    case 7:
    default:
        return MemoryFaultType::NONE;
    }
}

/// \brief Check if fault type represents an actual memory management fault
inline bool isMemoryManagementFault(MemoryFaultType faultType) noexcept
{
    switch (faultType) {
    case MemoryFaultType::DTB_MISS_READ:
    case MemoryFaultType::DTB_MISS_WRITE:
    case MemoryFaultType::DTB_FAULT_READ:
    case MemoryFaultType::DTB_FAULT_WRITE:
    case MemoryFaultType::DTB_ACCESS_VIOLATION_READ:
    case MemoryFaultType::DTB_ACCESS_VIOLATION_WRITE:
    case MemoryFaultType::FAULT_ON_READ:
    case MemoryFaultType::FAULT_ON_WRITE:
    case MemoryFaultType::FAULT_ON_EXECUTE:
    case MemoryFaultType::PAGE_NOT_PRESENT:
        return true;

    default:
        return false;
    }
}
#endif // DECODEMMSTATEFAULTTYPE_H
