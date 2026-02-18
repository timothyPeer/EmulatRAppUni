#ifndef MMULIB_CORE_H
#define MMULIB_CORE_H

// =============================================================================
// MMU OPERATION TYPE
// =============================================================================

/**
 * @brief MMU operation type (instruction fetch, data read, data write)
 * Used in TrapFrame to indicate what caused the fault
 */
#include <qtypes.h>
enum class MMUOperation : quint8 {
    FETCH = 0,   // Instruction fetch (ITB operation)
    READ = 1,    // Data read (DTB operation)
    WRITE = 2,   // Data write (DTB operation)
    PROBE = 3    // Probe (read without fault)
};

// ============================================================================
// MMU Operation to String Conversion
// ============================================================================
inline const char* mmuOperationName(MMUOperation op) noexcept
{
    switch (op)
    {
    case MMUOperation::FETCH:   return "FETCH";
    case MMUOperation::READ:    return "READ";
    case MMUOperation::WRITE:   return "WRITE";
    case MMUOperation::PROBE:   return "PROBE";
    default:                     return "INVALID_MMU_OP";
    }
}


#endif // MMULIB_CORE_H
