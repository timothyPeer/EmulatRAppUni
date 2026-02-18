#pragma once
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
	qsrand(static_cast<uint>(QTime::currentTime().msec()));
}
#else
inline void seedEmulatrRandom() {} // No-op for QRandomGenerator
#endif