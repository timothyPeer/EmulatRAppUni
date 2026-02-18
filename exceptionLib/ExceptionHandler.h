#ifndef _EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONHANDLER_H
#define _EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONHANDLER_H

#pragma once

// Forward declare AlphaCPU so this header stays lightweight.
class AlphaCPU;

/**
 * ExceptionHandler
 *
 * Represents a handler entry in the Alpha exception vector table.
 * Each entry corresponds to a specific architectural exception/trap
 * defined by the Alpha System Architecture (ASA), e.g. integer overflow,
 * arithmetic trap, access violation, page fault, machine check, etc.
 *
 * The handler contains:
 *  - A handler function pointer (invoked when exception occurs)
 *  - A human-readable name for debugging/logging
 *  - An optional architectural vector number (for documentation)
 *  - Optional flags describing handler behavior
 *
 * This struct remains lightweight and header-only.
 */
struct ExceptionHandler
{
    /**
     * Exception handler function signature.
     * All exception handlers accept an AlphaCPU* pointing to
     * the executing CPU context where the exception occurred.
     */
    using HandlerFn = void (*)(AlphaCPU*);

    /// Function pointer to the handler implementation.
    HandlerFn handler = nullptr;

    /// Human-readable symbolic name (e.g. "Arithmetic Trap", "Access Violation")
    const char* name = nullptr;

    /// Architectural vector number (optional; useful for tracing/debugging)
    unsigned vector = 0;

    /// Optional flags for future use (PAL-required, synchronous, fatal, etc)
    unsigned flags = 0;

    /// Default constructor (empty/invalid entry)
    constexpr ExceptionHandler() noexcept = default;

    /// Full constructor for easy table initialization
    constexpr ExceptionHandler(const char* nm,
        HandlerFn fn,
        unsigned vec = 0,
        unsigned fl = 0) noexcept
        : handler(fn), name(nm), vector(vec), flags(fl)
    {
    }

    /// Invoke the handler (safe wrapper)
    inline void invoke(AlphaCPU* cpu) const noexcept
    {
        if (handler)
            handler(cpu);
    }

    /// Returns true if the handler is valid
    inline bool isValid() const noexcept
    {
        return handler != nullptr;
    }
};


#endif
