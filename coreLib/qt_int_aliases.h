#pragma once

// ============================================================================
// qt_int_aliases.h
// ----------------------------------------------------------------------------
// Unified integer alias definitions for Alpha Emulator.
// Provides consistent typedefs for qint* / quint* based on Qt's <QtGlobal>.
//
// This header is header-only safe, pure ASCII, and may be included anywhere.
//
// Source Reference:
//   Qt Documentation: QtGlobal (defines qint8, qint16, qint32, qint64, etc.)
//   ANSI C99: stdint.h (defines int8_t, uint8_t, etc.)
// ============================================================================

#include <QtGlobal>    // Provides qint8, qint16, qint32, qint64, quint8, ...

// ---------------------------------------------------------------------------
// Qt integer aliases (verbatim from QtGlobal)
// ---------------------------------------------------------------------------
using qt_i8 = qint8;
using qt_i16 = qint16;
using qt_i32 = qint32;
using qt_i64 = qint64;

using qt_u8 = quint8;
using qt_u16 = quint16;
using qt_u32 = quint32;
using qt_u64 = quint64;

// ---------------------------------------------------------------------------
// Standard-library parallel aliases (useful for non-Qt compilation units)
// ---------------------------------------------------------------------------
#include <cstdint>

using std_i8 = std::int8_t;
using std_i16 = std::int16_t;
using std_i32 = std::int32_t;
using std_i64 = std::int64_t;

using std_u8 = std::quint8;
using std_u16 = std::quint16;
using std_u32 = std::uint32_t;
using std_u64 = std::quint64;

// ---------------------------------------------------------------------------
// Emulator-global preferred aliases
// ---------------------------------------------------------------------------
// These may be used inside your emulator as canonical integer types.
// They map directly to Qt's types to ensure compatibility with QAtomic*,
// QDataStream, QVariant, QMetaType, etc.

using i8 = qint8;
using i16 = qint16;
using i32 = qint32;
using i64 = qint64;

using u8 = quint8;
using u16 = quint16;
using u32 = quint32;
using u64 = quint64;

// ---------------------------------------------------------------------------
// Compile-time safety checks
// ---------------------------------------------------------------------------

static_assert(sizeof(i8) == 1, "qt_int_aliases: i8 must be 1 byte");
static_assert(sizeof(i16) == 2, "qt_int_aliases: i16 must be 2 bytes");
static_assert(sizeof(i32) == 4, "qt_int_aliases: i32 must be 4 bytes");
static_assert(sizeof(i64) == 8, "qt_int_aliases: i64 must be 8 bytes");

static_assert(sizeof(u8) == 1, "qt_int_aliases: u8 must be 1 byte");
static_assert(sizeof(u16) == 2, "qt_int_aliases: u16 must be 2 bytes");
static_assert(sizeof(u32) == 4, "qt_int_aliases: u32 must be 4 bytes");
static_assert(sizeof(u64) == 8, "qt_int_aliases: u64 must be 8 bytes");


// ============================================================================
// END OF FILE
// ============================================================================
