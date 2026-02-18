#ifndef WRITEBUFFERENTRY_H
#define WRITEBUFFERENTRY_H
// ============================================================================
// WriteBufferEntry.h
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================



#include "Axp_Attributes_core.h"
#include <QtGlobal>

static constexpr int WRITE_BUFFER_SIZE = 4; // used with WriteBufferEntry

/**
 * @brief Single pending write in CPU write buffer.
 *
 * Tracks a store that has been issued but may not yet be globally visible.
 * Field names match what CBox expects for seamless integration.
 */
struct WriteBufferEntry {
    quint64 address;      // Physical address (CBox expects 'address', not 'pa')
    quint64 bufferData;         // Data to write
    quint8  bufferSize;         // Write size (CBox expects 'size', not 'width')
    bool    valid;        // Entry is occupied
    bool    mmio;         // Is MMIO write (needs device notification)
    quint64 timestamp;    // Issue cycle (for ordering enforcement)

    WriteBufferEntry()
        : address(0), bufferData(0), bufferSize(0), valid(false), mmio(false), timestamp(0)
    {
    }

    void clear() {
        valid = false;
        address = 0;
        bufferData = 0;
        bufferSize = 0;
        mmio = false;
        timestamp = 0;
    }

    // Compatibility accessors for other code that might expect 'pa' and 'width'
    quint64 pa() const { return address; }
    void setPa(quint64 physAddr) { address = physAddr; }

    quint8 width() const { return bufferSize; }
    void setWidth(quint8 w) { bufferSize = w; }
};

#endif // WRITEBUFFERENTRY_H