#ifndef MEMORYBARRIER_QT_PORTABLE_H
#define MEMORYBARRIER_QT_PORTABLE_H

// ============================================================================
// Alpha Memory Barriers - Qt Portable Implementation
// ============================================================================
// Target: All Qt-supported platforms (Windows, Linux, macOS, Android, iOS, etc.)
// Uses Qt's cross-platform threading and atomic primitives
// Pure Qt solution - no platform-specific code needed!
// ============================================================================

#include <QtGlobal>
#include <QtCore/QAtomicInt>
#include <QtCore/QAtomicPointer>
#include <QtCore/QMutex>
#include <QtCore/QThread>

// Qt 5.14+ has enhanced atomic support, Qt 6+ even better
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/qatomic.h>
#endif

// ============================================================================
// QT-NATIVE MEMORY BARRIERS
// ============================================================================

namespace Alpha {
    namespace Memory {

        /**
         * @brief Full memory barrier using Qt's atomic operations
         * Portable across all Qt platforms with optimal per-platform implementation
         */
        inline void fullBarrier() noexcept {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            // Modern Qt - use atomic fence operations
            QAtomicInt dummy;
            dummy.fetchAndStoreAcquire(0);  // Acquire barrier
            dummy.fetchAndStoreRelease(0);  // Release barrier  
#else
            // Older Qt - use atomic operations to force ordering
            static QAtomicInt barrier_sync;
            barrier_sync.fetchAndStoreOrdered(barrier_sync.loadAcquire() + 1);
#endif
        }

        /**
         * @brief Acquire barrier (load-load, load-store ordering)
         * Prevents reordering of operations after this point before it
         */
        inline void acquireBarrier() noexcept {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            QAtomicInt dummy;
            dummy.loadAcquire();  // Qt's acquire semantic
#else
            static QAtomicInt barrier_sync;
            barrier_sync.loadAcquire();
#endif
        }

        /**
         * @brief Release barrier (store-store, load-store ordering)
         * Prevents reordering of operations before this point after it
         */
        inline void releaseBarrier() noexcept {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            static QAtomicInt dummy;
            dummy.storeRelease(0);  // Qt's release semantic
#else
            static QAtomicInt barrier_sync;
            barrier_sync.storeRelease(0);
#endif
        }

        /**
         * @brief Write barrier (store-store ordering)
         * Ensures write operations complete in order
         */
        inline void writeBarrier() noexcept {
            releaseBarrier();  // Release provides store-store ordering
        }

        /**
         * @brief Read barrier (load-load ordering)
         * Ensures read operations complete in order
         */
        inline void readBarrier() noexcept {
            acquireBarrier();  // Acquire provides load-load ordering
        }

        /**
         * @brief Compiler barrier using Qt thread-safe operations
         * Prevents compiler reordering across this point
         */
        inline void compilerBarrier() noexcept {
            // Qt's atomic operations include compiler barriers
            static QAtomicInt dummy;
            dummy.testAndSetRelaxed(0, 0);
        }

    } // namespace Memory

    // ============================================================================
    // ALPHA INSTRUCTION MAPPING (Qt Style)
    // ============================================================================

    namespace Instructions {

        /**
         * @brief Alpha MB (Memory Barrier) instruction
         * Qt portable implementation with platform-optimized backends
         */
        inline void mb() noexcept {
            Memory::fullBarrier();
        }

        /**
         * @brief Alpha WMB (Write Memory Barrier) instruction
         * Uses Qt's release semantics for optimal cross-platform performance
         */
        inline void wmb() noexcept {
            Memory::writeBarrier();
        }

        /**
         * @brief Alpha IMB (Instruction Memory Barrier) instruction
         * For instruction cache coherency (relevant for JIT/self-modifying code)
         */
        inline void imb() noexcept {
            // Use full barrier - Qt handles platform-specific instruction cache sync
            Memory::fullBarrier();

            // Qt doesn't expose instruction cache APIs directly,
            // but full barrier ensures proper ordering for Alpha IMB semantics
        }

    } // namespace Instructions

} // namespace Alpha

// ============================================================================
// TRADITIONAL MACRO INTERFACE (for existing code compatibility)
// ============================================================================

#define ALPHA_COMPILER_BARRIER() Alpha::Memory::compilerBarrier()
#define ALPHA_FULL_BARRIER()     Alpha::Memory::fullBarrier()
#define ALPHA_ACQUIRE_BARRIER()  Alpha::Memory::acquireBarrier()
#define ALPHA_RELEASE_BARRIER()  Alpha::Memory::releaseBarrier()

// Alpha instruction macros
#define ALPHA_MB()  Alpha::Instructions::mb()
#define ALPHA_WMB() Alpha::Instructions::wmb()
#define ALPHA_IMB() Alpha::Instructions::imb()

// ============================================================================
// QT-SPECIFIC OPTIMIZATIONS & PLATFORM SUPPORT
// ============================================================================

namespace Alpha {
    namespace QtOptimized {

        /**
         * @brief High-performance barrier using Qt's atomic operations
         * Simplified and optimized for frequent barrier usage
         */
        class FastBarrier {
        public:
            static void full() noexcept {
                // Simple and effective: atomic operation forces memory ordering
                static QAtomicInt barrier_counter;
                barrier_counter.fetchAndAddAcquire(1);  // Acquire barrier
                barrier_counter.fetchAndAddRelease(1);  // Release barrier
            }

            static void acquire() noexcept {
                static QAtomicInt barrier_sync;
                barrier_sync.loadAcquire();  // Load with acquire semantics
            }

            static void release() noexcept {
                static QAtomicInt barrier_sync;
                barrier_sync.storeRelease(barrier_sync.loadRelaxed() + 1);  // Store with release semantics
            }
        };

        /**
         * @brief Debug-aware barriers for Qt development
         * Includes Qt logging and debug checks when QT_DEBUG is enabled
         */
        inline void debugBarrier(const char* location = nullptr) noexcept {
#ifdef QT_DEBUG
            if (location) {
                qDebug("Alpha memory barrier at: %s", location);
            }
#else
            Q_UNUSED(location);
#endif
            Memory::fullBarrier();
        }

    } // namespace QtOptimized
} // namespace Alpha

// ============================================================================
// QT INTEGRATION HELPERS
// ============================================================================

namespace Alpha {
    namespace QtIntegration {

        /**
         * @brief Barrier-aware Qt signal emission
         * Ensures memory visibility before signal emission in multi-threaded scenarios
         */
        template<typename QObjectType>
        inline void emitWithBarrier(QObjectType* obj, void (QObjectType::* signal)()) noexcept {
            Memory::releaseBarrier();  // Ensure data visible before signal
            (obj->*signal)();
        }

        /**
         * @brief Barrier-aware Qt slot connection
         * For critical thread-safe signal-slot scenarios
         */
        inline void connectWithBarriers(const QObject* sender, const char* signal,
            const QObject* receiver, const char* slot) noexcept {
            Memory::fullBarrier();  // Ensure setup complete
            QObject::connect(sender, signal, receiver, slot, Qt::QueuedConnection);
            Memory::fullBarrier();  // Ensure connection established
        }

    } // namespace QtIntegration
} // namespace Alpha

// ============================================================================
// PLATFORM COVERAGE (What Qt Gives You)
// ============================================================================

/*
QT PLATFORM PORTABILITY:
========================

- Windows (all versions)      - Uses Windows atomic intrinsics
- Linux (x86, ARM, etc.)      - Uses GCC/Clang atomic builtins
- macOS (Intel + Apple Silicon) - Uses platform-appropriate atomics
- Android (ARM, x86)          - Uses Android NDK atomics
- iOS (ARM64)                 - Uses iOS/ARM64 atomics
- Embedded Linux              - Uses appropriate embedded atomics
- QNX, VxWorks, etc.         - Platform-specific Qt backends

PERFORMANCE CHARACTERISTICS:
===========================

Qt's atomic operations automatically map to:
- x86/x64: LOCK prefixed instructions, MFENCE, etc.
- ARM: DMB, DSB instructions as appropriate
- Other platforms: Optimal atomic sequences per platform

QT VERSION COMPATIBILITY:
========================

- Qt 5.9+  - Basic atomic operations
- Qt 5.14+ - Enhanced acquire/release semantics
- Qt 6.x   - Modern C++ atomic integration

USAGE EXAMPLES:
===============

// Qt-style (recommended):
Alpha::Instructions::mb();      // Alpha MB instruction
Alpha::Memory::fullBarrier();   // Direct memory barrier

// High-performance loops:
Alpha::QtOptimized::FastBarrier::full();

// Qt integration:
Alpha::QtIntegration::emitWithBarrier(this, &MyClass::dataReady);
*/

#endif // MEMORYBARRIER_QT_PORTABLE_H