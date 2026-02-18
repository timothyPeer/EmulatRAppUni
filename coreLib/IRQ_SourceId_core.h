#ifndef IRQ_SOURCEID_CORE_H
#define IRQ_SOURCEID_CORE_H
// ============================================================================
// IRQ_SourceId_core.h  (header-only, ASCII-128 clean)
// ============================================================================
// Interrupt source ID namespace, trigger mode, and shared types.
//
// DESIGN:
//   Source IDs are bit positions in quint64 per-level source masks.
//   The namespace is partitioned to prevent collisions between SW, platform,
//   and MMIO device sources.
//
// INVARIANT:
//   All source IDs must be < 64 (quint64 bitmask width).
//
// REFERENCE:
//   Alpha AXP System Reference Manual v6, 1994, Ch 6.4 (SIRR/SISR).
//   OpenVMS IPL conventions: SW 1..15, devices 20..23, MCHK 31.
// ============================================================================

#include <QtGlobal>
#include <cstdint>

// ============================================================================
// Source ID type
// ============================================================================
using IrqSourceId    = quint8;
using ScbVectorIndex = quint16;

// ============================================================================
// Source ID allocation
// ============================================================================
//
// Partition:
//   0-15     Software interrupts (1:1 with SISR bits, source N = level N)
//   16       AST (single source, delivered at IPL 2)
//   17       Clock / timer (edge-triggered, periodic)
//   18       IPI  (edge-triggered, coalescing signal)
//   19       Reserved
//   20-31    MMIO devices (assigned by router at registration)
//   32-47    Future expansion (MSI, additional devices)
//   48       Performance counter overflow
//   49       Power fail
//   50       Machine check
//   51-63    Reserved
//
namespace IrqSource
{
    // Software interrupt sources (1:1 with SISR bit positions)
    // Source 0 is unused (IPL 0 never delivers).
    static constexpr IrqSourceId SW_BASE        = 0;
    static constexpr IrqSourceId SW_MAX         = 15;

    // Platform sources
    static constexpr IrqSourceId AST            = 16;
    static constexpr IrqSourceId CLOCK          = 17;
    static constexpr IrqSourceId IPI            = 18;

    // MMIO device range (assigned dynamically by InterruptRouter)
    static constexpr IrqSourceId DEVICE_BASE    = 20;
    static constexpr IrqSourceId DEVICE_MAX     = 31;

    // Future expansion
    static constexpr IrqSourceId EXPANSION_BASE = 32;
    static constexpr IrqSourceId EXPANSION_MAX  = 47;

    // Fixed platform sources (high-priority / non-maskable)
    static constexpr IrqSourceId PERF_COUNTER   = 48;
    static constexpr IrqSourceId POWER_FAIL     = 49;
    static constexpr IrqSourceId MACHINE_CHECK  = 50;

    // Maximum valid source ID (quint64 bitmask limit)
    static constexpr IrqSourceId MAX_SOURCES    = 64;

    // ---- Helpers ----

    // True if sourceId is a software interrupt source (SISR domain)
    static constexpr bool isSoftwareSource(IrqSourceId id) noexcept
    {
        return (id >= 1) && (id <= SW_MAX);
    }

    // True if sourceId is in the MMIO device range
    static constexpr bool isDeviceSource(IrqSourceId id) noexcept
    {
        return (id >= DEVICE_BASE) && (id <= DEVICE_MAX);
    }

    // True if sourceId is valid (within quint64 bitmask width)
    static constexpr bool isValid(IrqSourceId id) noexcept
    {
        return id < MAX_SOURCES;
    }

    // Next available device source ID from a running counter.
    // Returns 0xFF if range exhausted.
    static constexpr IrqSourceId allocateDevice(IrqSourceId& counter) noexcept
    {
        if (counter > DEVICE_MAX) return 0xFF;
        return counter++;
    }
}

// ============================================================================
// Trigger mode (per-source, static configuration)
// ============================================================================
enum class IrqTriggerMode : quint8
{
    Edge,       // One-shot: cleared automatically on claim (SIRR, IPI, timer)
    Level       // Held: remains pending until device deasserts via MMIO clear
};

// ============================================================================
// IPL constants (OpenVMS conventions)
// ============================================================================
// These are platform policy labels, not architectural mandates.
// The Alpha ISA defines the IPL mechanism (0..31); the OS defines the mapping.
//
namespace IrqIPL
{
    static constexpr quint8 MIN            = 0;
    static constexpr quint8 AST            = 2;
    static constexpr quint8 SW_MAX         = 15;

    // Device interrupt levels (OpenVMS convention)
    static constexpr quint8 DEVICE_20      = 20;
    static constexpr quint8 DEVICE_21      = 21;
    static constexpr quint8 DEVICE_22      = 22;
    static constexpr quint8 DEVICE_23      = 23;

    // Platform sources
    static constexpr quint8 IPI            = 22;
    static constexpr quint8 CLOCK          = 22;
    static constexpr quint8 PERF           = 29;
    static constexpr quint8 POWER          = 30;
    static constexpr quint8 MCHK           = 31;

    static constexpr quint8 NUM_LEVELS     = 32;
}

// ============================================================================
// ClaimedInterrupt
// ============================================================================
// Returned by IRQPendingState::claimNext().
// Contains everything PAL delivery needs to vector through SCBB.
//
struct ClaimedInterrupt final
{
    bool            valid  { false };
    quint8          ipl    { 0 };       // Delivered IPL level (0..31)
    IrqSourceId     source { 0 };       // Source that was claimed
    ScbVectorIndex  vector { 0 };       // SCB vector offset for delivery
    IrqTriggerMode  trigger{ IrqTriggerMode::Edge };
};

// ============================================================================
// Source registration descriptor (used by InterruptRouter)
// ============================================================================
struct IrqSourceDescriptor final
{
    IrqSourceId     sourceId   { 0 };
    quint8          ipl        { 0 };
    ScbVectorIndex  vector     { 0 };
    IrqTriggerMode  trigger    { IrqTriggerMode::Edge };
    qint32          affinityCpu{ 0 };   // Target CPU (-1 = use routing policy)
};

#endif // IRQ_SOURCEID_CORE_H
