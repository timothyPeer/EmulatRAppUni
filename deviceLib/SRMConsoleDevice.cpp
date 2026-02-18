// ============================================================================
// SRMConsoleDevice.cpp
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// SRM Console Device Implementation
// ============================================================================

#include "SRMConsoleDevice.h"
#include "../coreLib/LoggingMacros.h"
#include <QProcess>
#include <QHostAddress>

#define COMPONENT_NAME "SRMConsole"

// ============================================================================
// ASCII Control Characters
// ============================================================================
namespace ASCII {
    constexpr quint8 NUL  = 0x00;
    constexpr quint8 BS   = 0x08;  // Backspace
    constexpr quint8 LF   = 0x0A;  // Line feed
    constexpr quint8 CR   = 0x0D;  // Carriage return
    constexpr quint8 DEL  = 0x7F;  // Delete
    constexpr quint8 ESC  = 0x1B;  // Escape
}

// ============================================================================
// Construction
// ============================================================================

SRMConsoleDevice::SRMConsoleDevice(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    // Wire up server signals
    connect(&m_server, &QTcpServer::newConnection,
            this, &SRMConsoleDevice::onNewConnection);
}

SRMConsoleDevice::~SRMConsoleDevice()
{
    stop();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool SRMConsoleDevice::start()
{
    QMutexLocker lock(&m_mutex);
    
    if (m_running) {
        WARN_LOG("SRM Console already running");
        return true;
    }
    
    // Start TCP server
    if (!m_server.listen(QHostAddress::Any, m_config.port)) {
        ERROR_LOG(QString("SRM Console: Failed to listen on port %1: %2")
                      .arg(m_config.port)
                      .arg(m_server.errorString()));
        return false;
    }
    
    m_running = true;
    
    INFO_LOG(QString("SRM Console: Listening on TCP port %1").arg(m_config.port));
    
    // Auto-launch PuTTY if configured
    if (m_config.autoLaunchPutty) {
        launchPutty();
    }
    
    return true;
}

void SRMConsoleDevice::stop()
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_running) {
        return;
    }
    
    // Disconnect client
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    
    // Stop server
    m_server.close();
    
    // Clear buffers
    m_rxQueue.clear();
    
    // Wake any blocked readers
    m_rxCondition.wakeAll();
    
    m_running = false;
    
    INFO_LOG("SRM Console: Stopped");
}

bool SRMConsoleDevice::isRunning() const
{
    QMutexLocker lock(&m_mutex);
    return m_running;
}

// ============================================================================
// CSERVE 0x01 - GETC (Get Character)
// ============================================================================

int SRMConsoleDevice::getChar(bool blocking, quint32 timeoutMs)
{
    QMutexLocker lock(&m_mutex);
    
    // Non-blocking mode - return immediately
    if (!blocking) {
        if (m_rxQueue.isEmpty()) {
            return -1;
        }
        return static_cast<int>(m_rxQueue.dequeue());
    }
    
    // Blocking mode - wait for data
    quint32 timeout = timeoutMs;
    if (timeout == 0) {
        timeout = m_config.defaultTimeoutMs;
    }
    
    while (m_rxQueue.isEmpty()) {
        if (timeout == UINT32_MAX) {
            // Infinite wait
            m_rxCondition.wait(&m_mutex);
        } else {
            // Timed wait
            if (!m_rxCondition.wait(&m_mutex, timeout)) {
                TRACE_LOG("SRM Console: GETC timeout");
                return -1;  // Timeout
            }
        }
        
        // Check if we were woken up for shutdown
        if (!m_running) {
            return -1;
        }
    }
    
    int ch = static_cast<int>(m_rxQueue.dequeue());
    
    TRACE_LOG(QString("SRM Console: GETC -> 0x%1 ('%2')")
                  .arg(ch, 2, 16, QChar('0'))
                  .arg(QChar(ch)));
    
    return ch;
}

// ============================================================================
// CSERVE 0x02 - PUTC (Put Character)
// ============================================================================

void SRMConsoleDevice::putChar(quint8 ch)
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        // Discard if not connected (per SRM spec)
        return;
    }
    
    writeRaw(ch);
    
    TRACE_LOG(QString("SRM Console: PUTC <- 0x%1 ('%2')")
                  .arg(ch, 2, 16, QChar('0'))
                  .arg(QChar(ch)));
}

// ============================================================================
// CSERVE 0x09 - PUTS (Put String)
// ============================================================================

quint64 SRMConsoleDevice::putString(const quint8* data, quint64 len)
{
    if (!data || len == 0) {
        return 0;
    }
    
    QMutexLocker lock(&m_mutex);
    
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return 0;  // Not connected
    }
    
    if (!writeRaw(data, static_cast<qint64>(len))) {
        ERROR_LOG("SRM Console: PUTS write failed");
        return 0;
    }
    
    TRACE_LOG(QString("SRM Console: PUTS <- %1 bytes").arg(len));
    
    return len;
}

// ============================================================================
// CSERVE 0x0C - GETS (Get String with Line Editing)
// ============================================================================

quint64 SRMConsoleDevice::getString(quint8* buffer, quint64 maxLen, bool echo)
{
    if (!buffer || maxLen < 2) {
        return 0;  // Need at least space for 1 char + null
    }
    
    QByteArray lineBuffer;
    lineBuffer.reserve(static_cast<int>(maxLen));
    
    while (true) {
        // Get next character (blocking)
        int ch = getChar(true, UINT32_MAX);  // Infinite wait
        
        if (ch < 0) {
            break;  // Timeout or error
        }
        
        quint8 c = static_cast<quint8>(ch);
        
        // Handle special characters
        if (c == ASCII::CR || c == ASCII::LF) {
            // End of line
            if (echo) {
                putChar(ASCII::CR);
                putChar(ASCII::LF);
            }
            break;
        }
        else if (c == ASCII::BS || c == ASCII::DEL) {
            // Backspace/Delete
            handleBackspace(lineBuffer, echo);
        }
        else if (c >= 0x20 && c < 0x7F) {
            // Printable character
            if (lineBuffer.size() < static_cast<int>(maxLen - 1)) {
                lineBuffer.append(static_cast<char>(c));
                if (echo) {
                    putChar(c);
                }
            }
            // else: buffer full, ignore
        }
        // else: ignore control characters
    }
    
    // Copy to output buffer
    quint64 len = static_cast<quint64>(lineBuffer.size());
    if (len > maxLen - 1) {
        len = maxLen - 1;
    }
    
    if (len > 0) {
        memcpy(buffer, lineBuffer.constData(), len);
    }
    
    // Null-terminate
    buffer[len] = 0;
    
    TRACE_LOG(QString("SRM Console: GETS -> %1 bytes").arg(len));
    
    return len;
}

// ============================================================================
// CSERVE Poll - Check Input Availability
// ============================================================================

bool SRMConsoleDevice::hasInput() const
{
    QMutexLocker lock(&m_mutex);
    return !m_rxQueue.isEmpty();
}

// ============================================================================
// Connection Status
// ============================================================================

bool SRMConsoleDevice::isConnected() const
{
    QMutexLocker lock(&m_mutex);
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

// ============================================================================
// Reset
// ============================================================================

void SRMConsoleDevice::reset()
{
    QMutexLocker lock(&m_mutex);
    
    m_rxQueue.clear();
    m_rxCondition.wakeAll();
    
    DEBUG_LOG("SRM Console: Reset");
}

// ============================================================================
// Qt Slots - Network Events
// ============================================================================

void SRMConsoleDevice::onNewConnection()
{
    QMutexLocker lock(&m_mutex);
    
    if (m_socket) {
        // Already have a client - reject
        QTcpSocket* newSocket = m_server.nextPendingConnection();
        newSocket->disconnectFromHost();
        newSocket->deleteLater();
        
        WARN_LOG("SRM Console: Rejected connection (already connected)");
        return;
    }
    
    // Accept new connection
    m_socket = m_server.nextPendingConnection();
    
    if (!m_socket) {
        ERROR_LOG("SRM Console: Failed to accept connection");
        return;
    }
    
    // Wire up socket signals
    connect(m_socket, &QTcpSocket::readyRead,
            this, &SRMConsoleDevice::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &SRMConsoleDevice::onDisconnected);
    
    INFO_LOG(QString("SRM Console: Client connected from %1:%2")
                 .arg(m_socket->peerAddress().toString())
                 .arg(m_socket->peerPort()));
    
    // Clear stale data
    m_rxQueue.clear();
    
    emit clientConnected();
}

void SRMConsoleDevice::onReadyRead()
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_socket) {
        return;
    }
    
    // Read all available data
    QByteArray data = m_socket->readAll();
    
    for (char c : data) {
        // Check buffer overflow
        if (m_rxQueue.size() >= static_cast<int>(m_config.rxBufferSize)) {
            // Drop oldest character (FIFO)
            m_rxQueue.dequeue();
            WARN_LOG("SRM Console: RX buffer overflow (dropped oldest)");
        }
        
        m_rxQueue.enqueue(static_cast<quint8>(c));
    }
    
    // Wake any blocked readers
    m_rxCondition.wakeAll();
    
    emit inputAvailable();
    
    TRACE_LOG(QString("SRM Console: Received %1 bytes (queue: %2)")
                  .arg(data.size())
                  .arg(m_rxQueue.size()));
}

void SRMConsoleDevice::onDisconnected()
{
    QMutexLocker lock(&m_mutex);
    
    INFO_LOG("SRM Console: Client disconnected");
    
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    
    // Clear buffers
    m_rxQueue.clear();
    
    // Wake any blocked readers (they'll get -1)
    m_rxCondition.wakeAll();
    
    emit clientDisconnected();
}

// ============================================================================
// Internal Helpers
// ============================================================================

void SRMConsoleDevice::launchPutty()
{
    if (m_config.puttyPath.isEmpty()) {
        return;
    }
    
    // Build PuTTY command:
    // putty.exe -raw -P <port> localhost
    QStringList args;
    args << "-raw";
    args << "-P" << QString::number(m_config.port);
    args << "localhost";
    
    INFO_LOG(QString("SRM Console: Launching PuTTY: %1 %2")
                 .arg(m_config.puttyPath)
                 .arg(args.join(" ")));
    
    if (QProcess::startDetached(m_config.puttyPath, args)) {
        DEBUG_LOG("SRM Console: PuTTY launched successfully");
    } else {
        WARN_LOG("SRM Console: Failed to launch PuTTY (non-fatal)");
    }
}

bool SRMConsoleDevice::writeRaw(const quint8* data, qint64 len)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    
    qint64 written = m_socket->write(reinterpret_cast<const char*>(data), len);
    m_socket->flush();
    
    return written == len;
}

void SRMConsoleDevice::handleBackspace(QByteArray& lineBuffer, bool echo)
{
    if (lineBuffer.isEmpty()) {
        return;  // Nothing to delete
    }
    
    // Remove last character from buffer
    lineBuffer.chop(1);
    
    if (echo) {
        // VT100 backspace sequence: BS + SPACE + BS
        // (moves cursor back, erases character, moves cursor back again)
        putChar(ASCII::BS);
        putChar(' ');
        putChar(ASCII::BS);
    }
}

void SRMConsoleDevice::handleDelete(QByteArray& lineBuffer, bool echo)
{
    // Same as backspace for line editing
    handleBackspace(lineBuffer, echo);
}
