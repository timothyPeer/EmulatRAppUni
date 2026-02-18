#ifndef PAL_CONSOLE_HANDLERS_EV6_H
#define PAL_CONSOLE_HANDLERS_EV6_H
// ============================================================================
// PalConsoleHandlers_EV6.h   (header-only)
// ============================================================================
// ASCII text, UTF-8 (no BOM)
//
// EV6 PAL Console Handlers (OPA0:)
//
// Purpose:
// --------
// Implements minimal PAL console services required for SRM bring-up.
// This layer:
//   - Does NOT enter/exit PAL
//   - Does NOT dispatch faults
//   - Does NOT touch PAL mode bits or PC[0]
//   - Does NOT own CPU, PAL, or delivery policy
//
// It ONLY implements the PAL-visible console primitives.
//
// Architectural placement:
// --------------------------
//   PAL entry (PalService)
//        |
//        v
//   CALL_PAL decode
//        |
//        v
//   PalConsoleHandlers_EV6  <--- THIS FILE
//        |
//        v
//   ConsoleOPA0Device (TCP)
//
// References:
// -----------
// Alpha Architecture Reference Manual (ASA)
// - PALcode services (console I/O)
// - SRM firmware expectations
//
// Notes:
// ------
// SRM requires only:
//   - console_putc
//   - console_getc
// for initial interaction.
// ============================================================================

#include "../coreLib/types_core.h"
#include "../coreLib/LoggingMacros.h"
#include "../cpuCoreLib/AlphaProcessorContext.h"
#include "../deviceLib/ConsoleOPA0Device.h"

// ============================================================================
// PAL Console Function Codes (EV6 / SRM)
// ----------------------------------------------------------------------------
// These values are SRM-defined CALL_PAL function numbers.
// They may vary by firmware, but these are canonical for Alpha SRM.
//
// Source: SRM Console Calling Conventions (EV5/EV6)
// ============================================================================
namespace PalConsoleFn
{
static constexpr quint64 CONSOLE_PUTC = 0x81; // Output character
static constexpr quint64 CONSOLE_GETC = 0x82; // Input character (polling)
}

// ============================================================================
// PalConsoleHandlers_EV6
// ============================================================================
class PalConsoleHandlers_EV6 final
{
public:
    PalConsoleHandlers_EV6() = delete;

    // ------------------------------------------------------------------------
    // Dispatch entry
    // ------------------------------------------------------------------------
    /**
     * @brief Dispatch console-related CALL_PAL
     *
     * @param ctx AlphaProcessorContext
     * @param console ConsoleOPA0Device instance
     * @param function CALL_PAL function code (R0 or immediate, per your decode)
     *
     * @return true if handled, false if not a console PAL call
     */
    static AXP_FLATTEN bool handleCallPal(
        AlphaProcessorContext* ctx,
        ConsoleOPA0Device* console,
        quint64 logFunction) noexcept
    {
        if (!console)
            return false;

        switch (logFunction)
        {
        case PalConsoleFn::CONSOLE_PUTC:
            handlePutc(ctx, console);
            return true;

        case PalConsoleFn::CONSOLE_GETC:
            handleGetc(ctx, console);
            return true;

        default:
            return false;
        }
    }

private:
    // ------------------------------------------------------------------------
    // console_putc
    // ------------------------------------------------------------------------
    /**
     * @brief Output a single character to OPA0:
     *
     * PAL ABI:
     *   R16 = character (low 8 bits)
     *
     * Return:
     *   None (R0 undefined / unchanged)
     *
     * ASA:
     *   Console output is synchronous and may block.
     */
    static AXP_FLATTEN void handlePutc(
        AlphaProcessorContext* ctx,
        ConsoleOPA0Device* console) noexcept
    {
        const quint64 val = ctx.readIntReg(16);
        const char ch = static_cast<char>(val & 0xFF);

        console->writeChar(ch);

#if AXP_DEBUG_ENABLED
        if (ch == '\n') {
            DEBUG_LOG("PAL CONSOLE: \\n");
        }
#endif
    }

    // ------------------------------------------------------------------------
    // console_getc
    // ------------------------------------------------------------------------
    /**
     * @brief Read a single character from OPA0:
     *
     * PAL ABI:
     *   No input arguments
     *
     * Return:
     *   R0 = character (0..255) if available
     *   R0 = -1 if no input available
     *
     * ASA / SRM:
     *   Polling is acceptable.
     */
    static AXP_FLATTEN void handleGetc(
        AlphaProcessorContext* ctx,
        ConsoleOPA0Device* console) noexcept
    {
        char ch{0};

        if (console->readChar(ch)) {
            ctx.writeIntReg(0, static_cast<quint64>(static_cast<unsigned char>(ch)));
        } else {
            // SRM convention: -1 means "no character available"
            ctx.writeIntReg(0, static_cast<quint64>(-1));
        }
    }
};

#endif // PAL_CONSOLE_HANDLERS_EV6_H
