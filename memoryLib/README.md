# CLEAN MEMORY SUBSYSTEM - OPTION A ARCHITECTURE
**ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.**

**Date**: 2025-01-27  
**Version**: 1.0 (Option A - Clean Implementation)

---

## PACKAGE CONTENTS

```
memoryLib/
├── MemorySpan.h           - Span structure for safe cross-page access
├── SafeMemory.h           - Clean offset-based RAM interface
├── SafeMemory.cpp         - Implementation using SparseMemoryBacking
├── SRMFirmwareRegion.h    - Read-only SRM firmware handler
├── SRMFirmwareRegion.cpp  - Implementation
├── GuestMemory.h          - Clean PA router with offsetBase
├── GuestMemory.cpp        - Complete routing implementation
└── README.md              - This file
```

---

## ARCHITECTURE OVERVIEW

**Option A**: SafeMemory is the single source of truth for all RAM.

### Key Principles

1. **Single Source of Truth**: SafeMemory stores ALL writable RAM bytes
2. **Offset-Based Subsystems**: All memory subsystems use offsets (PA-agnostic)
3. **GuestMemory is PA Router**: ONLY GuestMemory knows PA mappings
4. **No Duplication**: No AlphaMemorySystem storage (HWRPB lives in SafeMemory)

### PA Routing Table (Canonical)

| PA Range                        | Target        | Offset Mapping                    |
|---------------------------------|---------------|-----------------------------------|
| [0x0, 0x10000)                  | SafeMemory    | offset = pa - 0x0 + 0x0           |
| [0x20000000, 0x20200000)        | SRMFirmware   | offset = pa - 0x20000000 + 0x0    |
| [0x80000000, 0x880000000)       | SafeMemory    | offset = pa - 0x80000000 + 0x10000|
| [0xF0000000, 0x100000000)       | MMIOManager   | PA direct (no offset)             |

### Memory Layout

```
SafeMemory Internal Layout (offset-based):
  0x0000_0000 - 0x0000_FFFF:  Low 64 KB
    - HWRPB at offset 0x2000
    - Scratch/reserved
  0x0001_0000 - 0x8_0001_0000: Main RAM (32 GB)

Total SafeMemory Size: 64 KB + 32 GB = 0x8_0001_0000 bytes
```

---

## INSTALLATION

### Step 1: Backup Current Implementation

```bash
cd /path/to/project
mv memoryLib memoryLib-old-2025-01-27
```

### Step 2: Extract Clean Implementation

```bash
unzip clean-memory-subsystem.zip
# This creates memoryLib/ directory with clean files
```

### Step 3: Keep SparseMemoryBacking

```bash
# SparseMemoryBacking is proven and working - keep it!
cp memoryLib-old-2025-01-27/SparseMemoryBacking.h memoryLib/
```

### Step 4: Verify memory_core.h

```bash
# Ensure memory_core.h has required types
# Should already exist in memoryLib-old-2025-01-27/
cp memoryLib-old-2025-01-27/memory_core.h memoryLib/
```

---

## INTEGRATION CHECKLIST

### Update Initialization Code

**File**: `initLib/EmulatR_init.cpp`

```cpp
// Phase 4: Memory System Initialization
bool EmulatR_init::initializePhase4_MemorySystem()
{
    INFO_LOG("=== PHASE 4: Memory System Initialization ===");
    
    // 4.1: Initialize SafeMemory (64 KB + 32 GB)
    auto& safeMem = global_SafeMemory();
    constexpr quint64 lowMemSize = 0x0001_0000;      // 64 KB
    constexpr quint64 mainRamSize = 0x8_0000_0000;   // 32 GB
    constexpr quint64 totalSize = lowMemSize + mainRamSize;
    
    if (!safeMem.initialize(totalSize)) {
        ERROR_LOG("Failed to initialize SafeMemory");
        return false;
    }
    
    // 4.2: Initialize MMIO Manager
    auto& mmio = global_MMIOManager();
    if (!mmio.initialize()) {
        ERROR_LOG("Failed to initialize MMIOManager");
        return false;
    }
    
    // 4.3: Create SRM Firmware Region
    auto& srm = global_SRMFirmwareRegion();
    // (Will be loaded in Phase 5)
    
    // 4.4: Attach subsystems to GuestMemory
    auto& guestMem = global_GuestMemory();
    guestMem.attachSubsystems(&safeMem, &mmio, &srm);
    
    // 4.5: Initialize PA routing table
    guestMem.initDefaultPARoutes();
    
    INFO_LOG("Phase 4: Memory System initialized successfully");
    return true;
}
```

### Initialize HWRPB (NEW - Phase 4.5)

**File**: `initLib/EmulatR_init.cpp`

```cpp
bool EmulatR_init::initializePhase4_5_HWRPB()
{
    INFO_LOG("=== PHASE 4.5: HWRPB Initialization ===");
    
    // HWRPB is at PA 0x2000
    constexpr quint64 hwrpbPA = 0x0000_2000;
    constexpr quint64 hwrpbSize = 0x4000;  // 16 KB
    
    // Get span from GuestMemory (routes to SafeMemory)
    auto& guestMem = global_GuestMemory();
    MemorySpan hwrpbSpan = guestMem.getSpanToPA(hwrpbPA, hwrpbSize, AccessIntent::WriteOnly);
    
    if (!hwrpbSpan.isValid() || hwrpbSpan.len < hwrpbSize) {
        ERROR_LOG(QString("Failed to get HWRPB span: got %1 bytes, need %2")
            .arg(hwrpbSpan.len)
            .arg(hwrpbSize));
        return false;
    }
    
    // Zero entire HWRPB region
    memset(hwrpbSpan.data, 0, hwrpbSize);
    
    // Populate HWRPB structure
    struct HWRPB {
        quint64 physicalBase;
        quint64 signature;
        quint64 revision;
        quint64 size;
        // ... more fields ...
    };
    
    HWRPB* hwrpb = reinterpret_cast<HWRPB*>(hwrpbSpan.data);
    
    hwrpb->physicalBase = 0x2000;
    hwrpb->signature = 0x4250525748ULL;  // "HWRPB"
    hwrpb->revision = 6;
    hwrpb->size = 0x4000;
    
    INFO_LOG("HWRPB initialized in SafeMemory at PA 0x2000");
    
    return true;
}
```

### Load SRM Firmware (Phase 5)

**File**: `initLib/EmulatR_init.cpp`

```cpp
bool EmulatR_init::initializePhase5_FirmwareLoading()
{
    INFO_LOG("=== PHASE 5: Firmware Loading ===");
    
    // Load clipper.bin
    auto& srm = global_SRMFirmwareRegion();
    QString firmwarePath = "firmware/clipper.bin";
    
    if (!srm.loadFromFile(firmwarePath)) {
        ERROR_LOG(QString("Failed to load SRM firmware from %1").arg(firmwarePath));
        return false;
    }
    
    // CRITICAL: Patch embedded HWRPB physicalBase field
    quint8* hwrpbPtr = srm.getWritablePointerForPatching(0x2000, 8);
    if (hwrpbPtr) {
        quint64* physicalBase = reinterpret_cast<quint64*>(hwrpbPtr);
        quint64 oldValue = *physicalBase;
        *physicalBase = 0x20002000ULL;  // Patch from 0x2000 to 0x20002000
        
        INFO_LOG(QString("Patched embedded HWRPB physicalBase: 0x%1 -> 0x%2")
            .arg(oldValue, 16, 16, QChar('0'))
            .arg(*physicalBase, 16, 16, QChar('0')));
    }
    
    // Set entry point (skip entry stub, jump to main code)
    m_srmEntryPoint = 0x20010000;
    
    INFO_LOG(QString("SRM firmware loaded, entry point: 0x%1")
        .arg(m_srmEntryPoint, 16, 16, QChar('0')));
    
    return true;
}
```

---

## REMOVED FILES

**AlphaMemorySystem** has been **ELIMINATED ENTIRELY**:

```bash
# These files are NO LONGER NEEDED:
memoryLib/AlphaMemorySystem.h
memoryLib/AlphaMemorySystem.cpp
```

**HWRPB now lives in SafeMemory** at offset 0x2000 (PA 0x2000).

---

## TESTING

### Unit Tests

```cpp
void test_SafeMemory_OffsetBased()
{
    SafeMemory sm;
    
    // Initialize with Option A size (64 KB + 32 GB)
    assert(sm.initialize(0x8_0001_0000));
    
    // Test low memory access
    quint64 val;
    assert(sm.load(0x0, 8, val) == MEM_STATUS::Ok);      // First byte
    assert(sm.load(0x2000, 8, val) == MEM_STATUS::Ok);   // HWRPB
    assert(sm.load(0xFFFF, 1, val) == MEM_STATUS::Ok);   // Last byte of low
    
    // Test main RAM access
    assert(sm.load(0x10000, 8, val) == MEM_STATUS::Ok);  // First byte of RAM
    assert(sm.load(0x10100, 8, val) == MEM_STATUS::Ok);  // Offset into RAM
}

void test_GuestMemory_Routing()
{
    SafeMemory sm;
    sm.initialize(0x8_0001_0000);
    
    MMIOManager mmio;
    SRMFirmwareRegion srm;
    srm.loadFromFile("firmware/clipper.bin");
    
    GuestMemory gm;
    gm.attachSubsystems(&sm, &mmio, &srm);
    gm.initDefaultPARoutes();
    
    // Test routing
    assert(gm.classifyPA(0x0) == GuestMemory::RouteTarget::SafeMemory);
    assert(gm.classifyPA(0x2000) == GuestMemory::RouteTarget::SafeMemory);
    assert(gm.classifyPA(0x20010000) == GuestMemory::RouteTarget::SRMFirmware);
    assert(gm.classifyPA(0x80000000) == GuestMemory::RouteTarget::SafeMemory);
    assert(gm.classifyPA(0xF0000000) == GuestMemory::RouteTarget::MMIOManager);
}
```

### Integration Test

```cpp
void test_SRM_InstructionFetch()
{
    // Full initialization
    initializePhase4_MemorySystem();
    initializePhase4_5_HWRPB();
    initializePhase5_FirmwareLoading();
    
    auto& guestMem = global_GuestMemory();
    
    // Fetch instruction from SRM entry point
    const quint64 entryPA = 0x20010000;
    quint32 instruction = 0;
    
    MEM_STATUS status = guestMem.readInst32(entryPA, instruction);
    
    assert(status == MEM_STATUS::Ok);
    assert(instruction != 0);  // Should be valid instruction
    
    printf("✓ SRM instruction fetch successful\n");
    printf("  Entry PA: 0x%016llx\n", entryPA);
    printf("  Instruction: 0x%08x\n", instruction);
}
```

---

## TROUBLESHOOTING

### Build Errors

**Error**: `SparseMemoryBacking.h: No such file`  
**Fix**: Copy from old implementation: `cp memoryLib-old/SparseMemoryBacking.h memoryLib/`

**Error**: `memory_core.h: No such file`  
**Fix**: Copy from old implementation: `cp memoryLib-old/memory_core.h memoryLib/`

**Error**: `Undefined reference to AlphaMemorySystem`  
**Fix**: Remove all includes/references to AlphaMemorySystem (it's deleted!)

### Runtime Errors

**Error**: HWRPB not accessible at PA 0x2000  
**Check**: Did you call `initializePhase4_5_HWRPB()`?

**Error**: SRM instruction fetch fails  
**Check**: Did SRM firmware load successfully? Check logs.

**Error**: Offset out of range in SafeMemory  
**Check**: Is SafeMemory initialized with correct size (0x8_0001_0000)?

---

## PERFORMANCE NOTES

**Span Pattern**: Use `getSpanToPA()` for buffer access instead of loops:

```cpp
// SLOW - Loop with individual reads
for (int i = 0; i < len; i++) {
    quint8 byte;
    guestMem.read8(pa + i, byte);
    buffer[i] = byte;
}

// FAST - Span access
MemorySpan span = guestMem.getSpanToPA(pa, len, ReadOnly);
if (span.isValid()) {
    memcpy(buffer, span.data, span.len);
}
```

---

## SUCCESS CRITERIA

After installation, verify:

- ✅ Builds without errors
- ✅ SafeMemory initialized with correct size
- ✅ GuestMemory routing table shows 4 regions
- ✅ HWRPB accessible at PA 0x2000
- ✅ SRM firmware accessible at PA 0x20010000
- ✅ No AlphaMemorySystem references in code
- ✅ Unit tests pass
- ✅ Integration test passes (SRM instruction fetch)

---

## SUPPORT

For questions or issues:
- Review CANONICAL_PA_ROUTING_TABLE.md
- Review ADDRESS_SPACE_ARCHITECTURE_CONTRACT.md
- Check logs for routing/initialization errors

---

**Clean, Simple, Correct - Option A Architecture** ✅
