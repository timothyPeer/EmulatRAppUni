#pragma once
#include <QtGlobal>
#include <QString>

#include "Axp_Attributes_core.h"

// ================================================================
// Zero-Cost Trace Facility for Alpha EmulatR
// ================================================================
//
// To enable traces:
//      #define AXP_TRACE_ENABLED 1
//
// To disable (default):
//      leave undefined or #define AXP_TRACE_ENABLED 0
//
// When disabled:
//   • all trace calls are compiled out
//   • no strings remain in the binary
//   • absolutely no runtime cost
// ================================================================

// ----------------------------------------------------------------
// INTERNAL: Do-nothing stub. Fully eliminated by optimizer.
// ----------------------------------------------------------------
AXP_FLATTEN static void axp_trace_noop(const QString&) noexcept
{
    // intentionally empty
}

// ----------------------------------------------------------------
// INTERNAL: Actual trace function (only compiled when enabled).
// We declare it 'hot' to encourage better codegen.
// ----------------------------------------------------------------
#if AXP_TRACE_ENABLED
AXP_HOT inline void axp_trace_real(const QString& msg)
{
    // For now simply use Qt logging.
    // Can be replaced with lock-free ring buffer later.
    qDebug().noquote() << msg;
}
#endif

// ----------------------------------------------------------------
// PUBLIC MACRO: AXP_TRACE(msg)
// Compiles to zero instructions when disabled.
// ----------------------------------------------------------------
#if AXP_TRACE_ENABLED
#   define AXP_TRACE(msg) axp_trace_real(QStringLiteral(msg))
#else
#   define AXP_TRACE(msg) axp_trace_noop(QString())
#endif

// ----------------------------------------------------------------
// PUBLIC MACRO: AXP_TRACEF(fmt, ...)
// Allows formatted messages using QString::asprintf.
// Still collapses to zero when disabled.
// ----------------------------------------------------------------
#if AXP_TRACE_ENABLED
#   define AXP_TRACEF(fmt, ...) \
axp_trace_real(QString().asprintf(fmt, __VA_ARGS__))
#else
#   define AXP_TRACEF(fmt, ...) \
axp_trace_noop(QString())
#endif


