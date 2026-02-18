// ============================================================================
// global_SRMEnvStore.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Global SRMEnvStore singleton implementation
// Thread-safe with proper initialization/cleanup
// ============================================================================

#include "global_SRMEnvStore.h"
#include "SRMEnvStore.h"
#include "../coreLib/LoggingMacros.h"
#include <QMutex>
#include <QScopedPointer>
#include <QMutexLocker>
#include <QString>

// ============================================================================
// Singleton State
// ============================================================================

namespace {
	// Singleton instance (pointer for controlled lifetime)
	std::unique_ptr<SRMEnvStore> g_srmEnvStore;

	// Thread safety
	QMutex g_mutex;

	// Initialization state
	bool g_initialized = false;
	QString g_configPath = ".";  // Default config path
}


// ============================================================================
// Initialization and Cleanup
// ============================================================================

void initializeGlobalSRMEnvStore(const QString& configPath) noexcept
{
	QMutexLocker lock(&g_mutex);

	if (g_initialized) {
		WARN_LOG("SRMEnvStore already initialized, skipping");
		return;
	}

	try {
		g_configPath = configPath;
		g_srmEnvStore = std::make_unique<SRMEnvStore>(configPath);
		g_initialized = true;

		INFO_LOG(QString("SRMEnvStore initialized with config path: %1").arg(configPath));
		INFO_LOG(QString("Loaded %1 environment variables").arg(g_srmEnvStore->count()));
	}
	catch (const std::exception& e) {
		ERROR_LOG(QString("Failed to initialize SRMEnvStore: %1").arg(e.what()));
		g_initialized = false;
	}
	catch (...) {
		ERROR_LOG("Failed to initialize SRMEnvStore: unknown error");
		g_initialized = false;
	}
}

void shutdownGlobalSRMEnvStore() noexcept
{
	QMutexLocker lock(&g_mutex);

	if (!g_initialized) {
		return;
	}

	try {
		if (g_srmEnvStore) {
			// Ensure final save before shutdown
			g_srmEnvStore->save();
			INFO_LOG("SRMEnvStore saved before shutdown");

			g_srmEnvStore.reset();
		}

		g_initialized = false;
		INFO_LOG("SRMEnvStore shutdown complete");
	}
	catch (const std::exception& e) {
		ERROR_LOG(QString("Error during SRMEnvStore shutdown: %1").arg(e.what()));
	}
	catch (...) {
		ERROR_LOG("Unknown error during SRMEnvStore shutdown");
	}
}

bool isGlobalSRMEnvStoreInitialized() noexcept
{
	QMutexLocker lock(&g_mutex);
	return g_initialized;
}

// ============================================================================
// Global Access
// ============================================================================

SRMEnvStore& global_SRMEnvStore() noexcept
{
	QMutexLocker lock(&g_mutex);

	// Lazy initialization if not already initialized
	if (!g_initialized) {
		WARN_LOG("SRMEnvStore accessed before initialization, using default config path");

		try {
			g_srmEnvStore = std::make_unique<SRMEnvStore>(g_configPath);
			g_initialized = true;

			INFO_LOG(QString("SRMEnvStore lazy-initialized with default path: %1").arg(g_configPath));
		}
		catch (...) {
			ERROR_LOG("CRITICAL: Failed to lazy-initialize SRMEnvStore");
			// This should never happen in production, but we need to return something
			// Create a static fallback instance
			static SRMEnvStore fallback(".");
			return fallback;
		}
	}

	return *g_srmEnvStore;
}

// ============================================================================
// Convenience Functions
// ============================================================================

QString getSRMEnv(const QString& name) noexcept
{
	try {
		return global_SRMEnvStore().get(name);
	}
	catch (...) {
		ERROR_LOG(QString("Error getting SRM environment variable: %1").arg(name));
		return QString();
	}
}

void setSRMEnv(const QString& name, const QString& value) noexcept
{
	try {
		global_SRMEnvStore().set(name, value);
	}
	catch (...) {
		ERROR_LOG(QString("Error setting SRM environment variable: %1").arg(name));
	}
}

bool hasSRMEnv(const QString& name) noexcept
{
	try {
		return global_SRMEnvStore().exists(name);
	}
	catch (...) {
		ERROR_LOG(QString("Error checking SRM environment variable: %1").arg(name));
		return false;
	}
}

