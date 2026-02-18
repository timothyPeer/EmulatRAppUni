#ifndef EMULATRAPPUNI_CORELIB_LOGGINGHELPERS_H
#define EMULATRAPPUNI_CORELIB_LOGGINGHELPERS_H

#pragma once

// LoggingHelpers.h
// Helper macros and functions for logging with Qt and std::format
#include <QString>
#include <string>
#include <fmt/format.h>

// Convert std::string to QString efficiently
inline QString toQString(const std::string& str) {
    return QString::fromStdString(str);
}

// Convert std::string to QString (rvalue reference)
inline QString toQString(std::string&& str) {
    return QString::fromStdString(std::move(str));
}

// Helper macro that converts std::format result to QString
#define FORMAT_QSTRING(...) \
    QString::fromStdString(fmt::format(__VA_ARGS__))

// Alternative: Use fmt::format directly with QString
// This requires including fmt/format.h and potentially fmt/std.h
// Example usage:
//   TRACE_LOG(FORMAT_QSTRING("cpu={} va=0x{:016X}", cpuId, faultVA));
//
// Or if you want to use it directly:
//   LogMessage(severity, QString::fromStdString(fmt::format(...)));
#endif
