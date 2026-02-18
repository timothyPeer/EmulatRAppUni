#ifndef TLBSHOOTDOWNTYPES_H
#define TLBSHOOTDOWNTYPES_H

// ============================================================================
// TLBShootdownTypes.h
// ============================================================================
// Type definitions and message structures for TLB shootdown protocol
//
// Supports:
// - TBIAP (invalidate ASN on all CPUs)
// - TBIS (invalidate single VA on all CPUs, both ITB+DTB)
// - TBISD (invalidate single VA on all CPUs, DTB only)
// - TBISI (invalidate single VA on all CPUs, ITB only)
// - TBIA (invalidate all TLBs on all CPUs)
//
// Protocol:
// - Sender invalidates local TLB
// - Sender sends IPI to remote CPUs
// - Remote CPUs recognize IPI at instruction boundary
// - Remote CPUs bump local epochs (fast O(1) operation)
// - Optional: Remote CPUs send ACK
//
// References:
// - Alpha Architecture Reference Manual, TLB management
// - EV6 Hardware Reference, IPR operations
// ============================================================================

#include <QtGlobal>
#include <QString>
#include <QSettings>
#include <atomic>
#include "../coreLib/types_core.h"

namespace TLBShootdown {

    // ============================================================================
    // Shootdown Type (encodes scope and realm)
    // ============================================================================


    /**
     * @brief TLB shootdown configuration
     *
     * Loaded from ASAEmulatr.ini [TLBShootdown] section.
     */
    struct Config {
        bool enableACKs;                    // Wait for ACKs (default: false)
        bool enablePreciseInvalidation;     // Precise VA invalidation (default: false)
        bool enableLogging;                 // Debug logging (default: false)
        quint8 maxShootdownSeq;             // Max seq before wrap (default: 255)

        // Default configuration
        Config() noexcept
            : enableACKs(false)
            , enablePreciseInvalidation(false)
            , enableLogging(false)
            , maxShootdownSeq(255)
        {
        }

        /**
         * @brief Load configuration from QSettings
         *
         * Call during emulator initialization.
         */
        void loadFromSettings(QSettings& settings) noexcept
        {
            settings.beginGroup("TLBShootdown");

            enableACKs = settings.value("EnableACKs", false).toBool();
            enablePreciseInvalidation = settings.value("EnablePreciseInvalidation", false).toBool();
            enableLogging = settings.value("EnableLogging", false).toBool();
            maxShootdownSeq = static_cast<quint8>(settings.value("MaxShootdownSeq", 255).toUInt());

            settings.endGroup();
        }

        /**
         * @brief Save configuration to QSettings
         */
        void saveToSettings(QSettings& settings) const noexcept
        {
            settings.beginGroup("TLBShootdown");

            settings.setValue("EnableACKs", enableACKs);
            settings.setValue("EnablePreciseInvalidation", enablePreciseInvalidation);
            settings.setValue("EnableLogging", enableLogging);
            settings.setValue("MaxShootdownSeq", maxShootdownSeq);

            settings.endGroup();
        }
    };

    // Global configuration instance (initialized at startup)
    inline Config g_config;

    // ============================================================================
    // ACK Tracking (when enabled)
    // ============================================================================

    /**
     * @brief Track pending ACKs for a shootdown
     *
     * Only used when Config::enableACKs is true.
     */
    struct ACKTracker {
        quint8 seq;                     // Shootdown sequence number
        std::atomic<quint32> pending;   // Number of pending ACKs

        ACKTracker() noexcept
            : seq(0)
            , pending(0)
        {
        }

        void init(quint8 sequence, quint32 count) noexcept
        {
            seq = sequence;
            pending.store(count, std::memory_order_release);
        }

        bool isComplete() const noexcept
        {
            return pending.load(std::memory_order_acquire) == 0;
        }

        void decrementPending() noexcept
        {
            pending.fetch_sub(1, std::memory_order_release);
        }

        quint32 getPendingCount() const noexcept
        {
            return pending.load(std::memory_order_acquire);
        }
    };

    // ============================================================================
    // Statistics Tracking
    // ============================================================================

    /**
     * @brief Per-CPU TLB shootdown statistics
     */
    struct Statistics {
        std::atomic<quint64> shootdownsSent{ 0 };
        std::atomic<quint64> shootdownsReceived{ 0 };
        std::atomic<quint64> ipiQueueFull{ 0 };
        std::atomic<quint64> acksSent{ 0 };
        std::atomic<quint64> acksReceived{ 0 };

        void reset() noexcept
        {
            shootdownsSent.store(0, std::memory_order_relaxed);
            shootdownsReceived.store(0, std::memory_order_relaxed);
            ipiQueueFull.store(0, std::memory_order_relaxed);
            acksSent.store(0, std::memory_order_relaxed);
            acksReceived.store(0, std::memory_order_relaxed);
        }

        QString toString() const noexcept
        {
            return QString("Sent=%1, Rcvd=%2, QueueFull=%3, ACKs_Sent=%4, ACKs_Rcvd=%5")
                .arg(shootdownsSent.load(std::memory_order_relaxed))
                .arg(shootdownsReceived.load(std::memory_order_relaxed))
                .arg(ipiQueueFull.load(std::memory_order_relaxed))
                .arg(acksSent.load(std::memory_order_relaxed))
                .arg(acksReceived.load(std::memory_order_relaxed));
        }
    };
} // namespace TLBShootdown

#endif // TLBSHOOTDOWNTYPES_H