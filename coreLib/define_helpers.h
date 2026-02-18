#pragma once

//
// ---------------------------------------------------------------------------
//  LOGGING MACROS WITH FULL QChar / QString / QByteArray SUPPORT
// ---------------------------------------------------------------------------
//

#include <QString>
#include <QChar>

#define MEM_BARRIER()   std::atomic_thread_fence(std::memory_order_seq_cst)
#define MEM_WMB()       std::atomic_thread_fence(std::memory_order_release)
#define MEM_RMB()       std::atomic_thread_fence(std::memory_order_acquire)
#define TRAP_BARRIER()  std::atomic_thread_fence(std::memory_order_seq_cst)


// Console
// 
// --- VT/ANSI Control Strings ---
#define VT_ESC "\033"
#define VT_CLEAR_SCREEN VT_ESC "[2J"
#define VT_CURSOR_HOME  VT_ESC "[H"
#define VT_RESET        VT_ESC "c"
#define VT_ERASE_LINE   VT_ESC "[2K"
#define VT_BELL         "\007"

//==============================================================================
// ALPHA PLATFORM CONFIGURATION
//==============================================================================
// Define platform-specific macros to control PAL instruction availability
// and platform-specific behavior.
//
// To configure, define one of the following in your build:
//   - ALPHA_PLATFORM_TRU64   (Digital UNIX/Tru64)
//   - ALPHA_PLATFORM_OPENVMS (OpenVMS)
//   - ALPHA_PLATFORM_WINDOWS (Windows NT)
//   - ALPHA_PLATFORM_SRM     (SRM Console/Linux)
//   - ALPHA_PLATFORM_CUSTOM  (Custom environment)
//
// If none is defined, a default environment will be selected.
//==============================================================================

// Default platform if none is specified
#if !defined(ALPHA_PLATFORM_TRU64) && !defined(ALPHA_PLATFORM_OPENVMS) && !defined(ALPHA_PLATFORM_WINDOWS) &&          \
		!defined(ALPHA_PLATFORM_SRM) && !defined(ALPHA_PLATFORM_CUSTOM)
#define ALPHA_PLATFORM_OPENVMS 1
#endif

// Ensure only one platform is enabled
#if (defined(ALPHA_PLATFORM_TRU64) && defined(ALPHA_PLATFORM_OPENVMS)) ||                                              \
		                                (defined(ALPHA_PLATFORM_TRU64) && defined(ALPHA_PLATFORM_WINDOWS)) ||                                              \
		    (defined(ALPHA_PLATFORM_TRU64) && defined(ALPHA_PLATFORM_SRM)) ||                                                  \
		    (defined(ALPHA_PLATFORM_TRU64) && defined(ALPHA_PLATFORM_CUSTOM)) ||                                               \
		    (defined(ALPHA_PLATFORM_OPENVMS) && defined(ALPHA_PLATFORM_WINDOWS)) ||                                            \
		    (defined(ALPHA_PLATFORM_OPENVMS) && defined(ALPHA_PLATFORM_SRM)) ||                                                \
		    (defined(ALPHA_PLATFORM_OPENVMS) && defined(ALPHA_PLATFORM_CUSTOM)) ||                                             \
		    (defined(ALPHA_PLATFORM_WINDOWS) && defined(ALPHA_PLATFORM_SRM)) ||                                                \
		    (defined(ALPHA_PLATFORM_WINDOWS) && defined(ALPHA_PLATFORM_CUSTOM)) ||                                             \
		    (defined(ALPHA_PLATFORM_SRM) && defined(ALPHA_PLATFORM_CUSTOM))
#error "Only one Alpha platform can be enabled at a time"
#endif

// Additional platform configuration
#ifdef ALPHA_PLATFORM_TRU64
#define ALPHA_PLATFORM_NAME "Digital UNIX/Tru64"
#define ALPHA_PAL_STYLE_OSF 1
// Default processor for Tru64
#ifndef CPU_EV
#define CPU_EV 5 // EV5 is common for Tru64 systems
#endif
#endif

#ifdef ALPHA_PLATFORM_OPENVMS
#define ALPHA_PLATFORM_NAME "OpenVMS"
#define ALPHA_PAL_STYLE_VMS 1
// Default processor for OpenVMS
#ifndef CPU_EV
#define CPU_EV 6 // EV6 is common for OpenVMS systems
#endif
#endif

#ifdef ALPHA_PLATFORM_WINDOWS
#define ALPHA_PLATFORM_NAME "Windows NT"
#define ALPHA_PAL_STYLE_WINDOWS 1
// Default processor for Windows NT
#ifndef CPU_EV
#define CPU_EV 4 // EV4 was common for Windows NT Alpha systems
#endif
#endif

#ifdef ALPHA_PLATFORM_SRM
#define ALPHA_PLATFORM_NAME "SRM Console/Linux"
#define ALPHA_PAL_STYLE_SRM 1
// Default processor for SRM/Linux
#ifndef CPU_EV
#define CPU_EV 6 // EV6 is common for Linux Alpha systems
#endif
#endif

#ifdef ALPHA_PLATFORM_CUSTOM
#define ALPHA_PLATFORM_NAME "Custom Alpha Environment"
// Custom platforms must define their PAL style explicitly
#ifndef ALPHA_PAL_STYLE_CUSTOM
#define ALPHA_PAL_STYLE_CUSTOM 1
#endif
// No default CPU_EV for custom platforms - must be specified
#endif

// Verify that a CPU_EV value is defined
#ifndef CPU_EV
#error "CPU_EV must be defined for the selected platform"
#endif

// Validation for CPU_EV value
#if CPU_EV != 4 && CPU_EV != 5 && CPU_EV != 6 && CPU_EV != 67 && CPU_EV != 68 && CPU_EV != 7
#error "Invalid CPU_EV value. Must be 4, 5, 6, 67, 68, or 7"
#endif

//==============================================================================
// HELPER MACROS FOR PLATFORM-SPECIFIC CODE
//==============================================================================

// Use these macros for conditional compilation based on platform
// #define ALPHA_IF_TRU64(code)                                                                                           \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifdef ALPHA_PLATFORM_TRU64 code #endif                                                                        \
		//     } while (0)

		// #define ALPHA_IF_OPENVMS(code)                                                                                         \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifdef ALPHA_PLATFORM_OPENVMS code #endif                                                                      \
		//     } while (0)
		//
		// #define ALPHA_IF_WINDOWS(code)                                                                                         \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifdef ALPHA_PLATFORM_WINDOWS code #endif                                                                      \
		//     } while (0)
		//
		// #define ALPHA_IF_SRM(code)                                                                                             \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifdef ALPHA_PLATFORM_SRM code #endif                                                                          \
		//     } while (0)
		//
		// #define ALPHA_IF_NOT_TRU64(code)                                                                                       \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifndef ALPHA_PLATFORM_TRU64 code #endif                                                                       \
		//     } while (0)
		//
		// #define ALPHA_IF_NOT_OPENVMS(code)                                                                                     \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifndef ALPHA_PLATFORM_OPENVMS code #endif                                                                     \
		//     } while (0)
		//
		// #define ALPHA_IF_NOT_WINDOWS(code)                                                                                     \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifndef ALPHA_PLATFORM_WINDOWS code #endif                                                                     \
		//     } while (0)
		//
		// #define ALPHA_IF_NOT_SRM(code)                                                                                         \
		//     do                                                                                                                 \
		//     {                                                                                                                  \
		//         #ifndef ALPHA_PLATFORM_SRM code #endif                                                                         \
		//     } while (0)

		// Platform detection in regular if statements
#define ALPHA_IS_TRU64 (defined(ALPHA_PLATFORM_TRU64))
#define ALPHA_IS_OPENVMS (defined(ALPHA_PLATFORM_OPENVMS))
#define ALPHA_IS_WINDOWS (defined(ALPHA_PLATFORM_WINDOWS))
#define ALPHA_IS_SRM (defined(ALPHA_PLATFORM_SRM))
#define ALPHA_IS_CUSTOM (defined(ALPHA_PLATFORM_CUSTOM))

//==============================================================================
// PLATFORM CAPABILITIES AND FEATURES
//==============================================================================

// Define platform-specific capabilities
#ifdef ALPHA_PLATFORM_TRU64
// Tru64/Digital UNIX capabilities
#define ALPHA_HAS_TRU64_SYSCALLS 1
#define ALPHA_SUPPORTS_BWT_EXTENSIONS 1 // Byte/Word/Tbyte extensions
#define ALPHA_SUPPORTS_CIX_EXTENSIONS 1 // Count extensions
#define ALPHA_SUPPORTS_MVI_EXTENSIONS 0 // Motion Video Instructions
#endif

#ifdef ALPHA_PLATFORM_OPENVMS
// OpenVMS capabilities
#define ALPHA_HAS_VMS_SYSCALLS 1
#define ALPHA_SUPPORTS_BWT_EXTENSIONS 1
#define ALPHA_SUPPORTS_CIX_EXTENSIONS 1
#define ALPHA_SUPPORTS_MVI_EXTENSIONS 1
#endif

#ifdef ALPHA_PLATFORM_WINDOWS
// Windows NT capabilities
#define ALPHA_HAS_WINDOWS_SYSCALLS 1
#define ALPHA_SUPPORTS_BWT_EXTENSIONS 0
#define ALPHA_SUPPORTS_CIX_EXTENSIONS 0
#define ALPHA_SUPPORTS_MVI_EXTENSIONS 0
#endif

#ifdef ALPHA_PLATFORM_SRM
// SRM/Linux capabilities
#define ALPHA_HAS_LINUX_SYSCALLS 1
#define ALPHA_SUPPORTS_BWT_EXTENSIONS 1
#define ALPHA_SUPPORTS_CIX_EXTENSIONS 1
#define ALPHA_SUPPORTS_MVI_EXTENSIONS 0
#endif

#ifdef ALPHA_PLATFORM_CUSTOM
// Custom platform capabilities - defaults that can be overridden
#ifndef ALPHA_SUPPORTS_BWT_EXTENSIONS
#define ALPHA_SUPPORTS_BWT_EXTENSIONS 0
#endif
#ifndef ALPHA_SUPPORTS_CIX_EXTENSIONS
#define ALPHA_SUPPORTS_CIX_EXTENSIONS 0
#endif
#ifndef ALPHA_SUPPORTS_MVI_EXTENSIONS
#define ALPHA_SUPPORTS_MVI_EXTENSIONS 0
#endif
#endif

#ifndef MAX_STACK_SIZE
#define STACK_MAX_SIZE 1024 ///< Maximum number of stack frames in synthetic stack
#endif

