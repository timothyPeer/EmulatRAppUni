// ============================================================================
// ConsoleManager.cpp - Get OPA device by index
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

#include "ConsoleManager.h"
#include "../coreLib/LoggingMacros.h"
#include <QMutexLocker>
#define COMPONENT_NAME "ConsoleManager"

// ============================================================================
// Construction
// ============================================================================

ConsoleManager::ConsoleManager(QObject* parent)
    : QObject(parent)
{
}

ConsoleManager::~ConsoleManager()
{
    QMutexLocker lock(&m_mutex);
    extracted();
    m_devices.clear();
}

void ConsoleManager::extracted()
{
    for (auto* device : m_devices) {
        device->reset();
        delete device;
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

bool ConsoleManager::initialize() noexcept
{
    DEBUG_LOG("ConsoleManager: Initializing");
    INFO_LOG("ConsoleManager: Initialized successfully");
    return true;
}

void ConsoleManager::shutdown() noexcept
{
    DEBUG_LOG("ConsoleManager: Shutting down");
    resetAll();
    INFO_LOG("ConsoleManager: Shutdown complete");
}

// ============================================================================
// Device Registration
// ============================================================================

bool ConsoleManager::registerDevice(const QString& name, IConsoleDevice* device)
{
    QMutexLocker lock(&m_mutex);
    
    if (m_devices.contains(name)) {
        ERROR_LOG(QString("Console %1: Already registered").arg(name));
        return false;
    }
    
    m_devices.insert(name, device);
    
    INFO_LOG(QString("Console %1: Registered").arg(name));
    
    return true;
}

bool ConsoleManager::unregisterDevice(const QString& name)
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_devices.contains(name)) {
        WARN_LOG(QString("Console %1: Not registered").arg(name));
        return false;
    }
    
    auto* device = m_devices.take(name);
    delete device;
    
    INFO_LOG(QString("Console %1: Unregistered").arg(name));
    
    return true;
}

IConsoleDevice* ConsoleManager::getDevice(const QString& name)
{
    QMutexLocker lock(&m_mutex);
    return m_devices.value(name, nullptr);
}

IConsoleDevice* ConsoleManager::getPrimaryConsole()
{
    return getDevice("OPA0");
}

bool ConsoleManager::hasDevice(const QString& name) const
{
    QMutexLocker lock(&m_mutex);
    return m_devices.contains(name);
}

QStringList ConsoleManager::deviceNames() const
{
    QMutexLocker lock(&m_mutex);
    return m_devices.keys();
}

qsizetype ConsoleManager::deviceCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_devices.size();
}

// ============================================================================
// CSERVE Entry Points (Called by PalService::executeCSERVE)
// ============================================================================

/**
 * @brief Get OPA device by index
 * Internal helper - not thread-safe (caller must lock)
 */
IConsoleDevice* ConsoleManager::getOPADevice(int opaIndex)
{
    QString deviceName = QString("OPA%1").arg(opaIndex);
    return m_devices.value(deviceName, nullptr);
}

// ----------------------------------------------------------------------------
// CSERVE 0x01 - GETC
// ----------------------------------------------------------------------------

int ConsoleManager::getCharFromOPA(int opaIndex, bool blocking, quint32 timeoutMs)
{
    QMutexLocker lock(&m_mutex);
    
    IConsoleDevice* device = getOPADevice(opaIndex);
    if (!device) {
        WARN_LOG(QString("CSERVE GETC: OPA%1 not found").arg(opaIndex));
        return -1;
    }
    
    lock.unlock();  // Release lock during potentially blocking I/O
    
    int ch = device->getChar(blocking, timeoutMs);
    
    if (ch >= 0) {
        TRACE_LOG(QString("CSERVE GETC: OPA%1 -> 0x%2")
                      .arg(opaIndex)
                      .arg(ch, 2, 16, QChar('0')));
    }
    
    return ch;
}

// ----------------------------------------------------------------------------
// CSERVE 0x02 - PUTC
// ----------------------------------------------------------------------------

bool ConsoleManager::putCharToOPA(int opaIndex, quint8 ch)
{
    QMutexLocker lock(&m_mutex);
    
    IConsoleDevice* device = getOPADevice(opaIndex);
    if (!device) {
        WARN_LOG(QString("CSERVE PUTC: OPA%1 not found").arg(opaIndex));
        return false;
    }
    
    lock.unlock();  // Release lock during I/O
    
    device->putChar(ch);
    
    TRACE_LOG(QString("CSERVE PUTC: OPA%1 <- 0x%2")
                  .arg(opaIndex)
                  .arg(ch, 2, 16, QChar('0')));
    return true;
}

// ----------------------------------------------------------------------------
// CSERVE 0x09 - PUTS
// ----------------------------------------------------------------------------

quint64 ConsoleManager::putStringToOPA(int opaIndex, const quint8* data, quint64 len)
{
    if (!data || len == 0) {
        return 0;
    }
    
    QMutexLocker lock(&m_mutex);
    
    IConsoleDevice* device = getOPADevice(opaIndex);
    if (!device) {
        WARN_LOG(QString("CSERVE PUTS: OPA%1 not found").arg(opaIndex));
        return 0;
    }
    
    lock.unlock();  // Release lock during I/O
    
    quint64 written = device->putString(data, len);
    
    TRACE_LOG(QString("CSERVE PUTS: OPA%1 <- %2 bytes")
                  .arg(opaIndex)
                  .arg(written));
    
    return written;
}

// ----------------------------------------------------------------------------
// CSERVE 0x0C - GETS
// ----------------------------------------------------------------------------

quint64 ConsoleManager::getStringFromOPA(int opaIndex, quint8* buffer, quint64 maxLen, bool echo)
{
    if (!buffer || maxLen < 2) {
        return 0;
    }
    
    QMutexLocker lock(&m_mutex);
    
    IConsoleDevice* device = getOPADevice(opaIndex);
    if (!device) {
        WARN_LOG(QString("CSERVE GETS: OPA%1 not found").arg(opaIndex));
        return 0;
    }
    
    lock.unlock();  // Release lock during potentially blocking I/O
    
    quint64 bytesRead = device->getString(buffer, maxLen, echo);
    
    TRACE_LOG(QString("CSERVE GETS: OPA%1 -> %2 bytes")
                  .arg(opaIndex)
                  .arg(bytesRead));
    
    return bytesRead;
}

// ----------------------------------------------------------------------------
// CSERVE Poll
// ----------------------------------------------------------------------------

bool ConsoleManager::hasInputOnOPA(int opaIndex) const
{
    QMutexLocker lock(&m_mutex);
    
    IConsoleDevice* device = const_cast<ConsoleManager*>(this)->getOPADevice(opaIndex);
    if (!device) {
        return false;
    }
    
    return device->hasInput();
}

// ----------------------------------------------------------------------------
// Connection Status
// ----------------------------------------------------------------------------

bool ConsoleManager::isAvailable_internal(int opaIndex) const
{
    // IMPORTANT: NO LOCK - caller must hold m_mutex

    // IMPORTANT: Assumes caller holds m_mutex
    QString deviceName = QString("OPA%1").arg(opaIndex);
    auto* device = m_devices.value(deviceName, nullptr);
    return device && device->isConnected();
}

bool ConsoleManager::isAvailable(int opaIndex) const
{
    QMutexLocker lock(&m_mutex);
    
    QString deviceName = QString("OPA%1").arg(opaIndex);
    auto* device = m_devices.value(deviceName, nullptr);
    
    if (!device) {
        return false;
    }
    
    return device->isConnected();
}

IConsoleDevice* ConsoleManager::getOPA(int index) const
{
	QMutexLocker lock(&m_mutex);

	QString deviceName = QString("OPA%1").arg(index);
	return m_devices.value(deviceName, nullptr);
}

// ============================================================================
// Maintenance
// ============================================================================

void ConsoleManager::resetAll()
{
    QMutexLocker lock(&m_mutex);
    
    for (auto* device : m_devices) {
        device->reset();
    }
    
    INFO_LOG("All consoles reset");
}
bool ConsoleManager::openConsole(int opaIndex)
{
	QMutexLocker lock(&m_mutex);

    // Use internal version (no double-lock)
    if (!isAvailable_internal(opaIndex)) {  //  No lock here!
        return false;
    }

	m_openedConsoles.insert(opaIndex);
	INFO_LOG(QString("Console OPA%1 opened").arg(opaIndex));
	return true;
}

bool ConsoleManager::isConsoleOpen(int opaIndex) const
{
	QMutexLocker lock(&m_mutex);
	return m_openedConsoles.contains(opaIndex);
}