// ============================================================================
// arithExtender_helpers.h - Sign-extend an 8-bit value to 64 bits.
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#pragma once
#include <bit>
#include <QtCore>
#include <QTime>
#include <QRandomGenerator>


// Macro to choose the random API based on Qt version
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
// For Qt 5.10+ and Qt 6.x: use QRandomGenerator::global()->bounded(N)
#define Q_EMULATR_RANDOM_INT(N) (QRandomGenerator::global()->bounded(N))
#else
	// For Qt <= 5.9.x: use qrand()
	// You MUST seed qsrand() ONCE at application startup!
#define Q_EMULATR_RANDOM_INT(N) (qrand() % (N))
#endif

// Example seeding function for Qt <= 5.9.x:
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
#include <QTime>
inline void seedEmulatrRandom() {
	qsrand(static_cast<quint32>(QTime::currentTime().msec()));
}
#else
inline void seedEmulatrRandom() {} // No-op for QRandomGenerator
#endif

//---------------------------------
// Sign & Zero-Extend Helpers
//---------------------------------

/**
 * @brief Sign-extend an 8-bit value to 64 bits.
 */
inline quint64 signExtend8(quint8 value) {
	return static_cast<quint64>(static_cast<qint64>(static_cast<qint8>(value)));
}
/**
 * @brief Sign-extend a 16-bit value to 64 bits.
 */
inline quint64 signExtend16(quint16 value) {
	return static_cast<quint64>(static_cast<qint64>(static_cast<qint16>(value)));
}

/**
 * @brief Sign-extend a 32-bit value to 64 bits.
 */
inline quint64 signExtend32(quint32 value) {
	return static_cast<quint64>(static_cast<qint64>(static_cast<qint32>(value)));
}

inline quint64 signExtend21(quint32 value) {
	constexpr quint32 signBit = 1u << 20;
	constexpr quint32 mask = ~((1u << 21) - 1);

	quint64 extended = (value & signBit) ? (value | mask) : value;

	return static_cast<quint64>(static_cast<qint64>(static_cast<qint32>(extended)));
}

/**
 * @brief Zero-extend an 8-bit value to 64 bits.
 */
static inline quint64 zeroExtend8(qsizetype v) {
	return static_cast<quint64>(v);
}

/**
 * @brief Zero-extend a 16-bit value to 64 bits.
 */
static inline quint64 zeroExtend16(quint16 v) {
	return static_cast<quint64>(v);
}

/**
 * @brief Zero-extend a 32-bit value to 64 bits.
 */
static inline quint64 zeroExtend32(quint32 v) {
	return static_cast<quint64>(v);
}


inline uint countTrailingZeros64(quint64 x) {
	return qCountTrailingZeroBits(x);
}

