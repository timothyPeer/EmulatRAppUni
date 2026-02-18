# QUICK INSTALLATION GUIDE
**Clean Memory Subsystem - Option A Architecture**

---

## TONIGHT (BEFORE SLEEP - 5 minutes)

### 1. Backup Current Implementation

```bash
cd /path/to/project

# Option 1: Git commit (BEST)
git add -A
git commit -m "Pre-refactor backup: Before clean memory subsystem"
git push

# Option 2: Archive (GOOD)
tar -czf backup-2025-01-27-pre-clean-memory.tar.gz memoryLib/

# Option 3: Rename (OK)
mv memoryLib memoryLib-old-2025-01-27
```

### 2. Verify Backup

```bash
# Check backup exists
ls -lh backup-2025-01-27-pre-clean-memory.tar.gz
# OR
ls -ld memoryLib-old-2025-01-27
# OR
git log --oneline | head -3
```

---

## TOMORROW MORNING (10 minutes)

### 1. Extract Clean Implementation

```bash
cd /path/to/project

# Download and extract
unzip clean-memory-subsystem.zip
# This creates: memoryLib/ directory with clean files
```

### 2. Copy Required Files from Old Implementation

```bash
# SparseMemoryBacking (proven, working - keep it!)
cp memoryLib-old-2025-01-27/SparseMemoryBacking.h memoryLib/

# memory_core.h (types and constants)
cp memoryLib-old-2025-01-27/memory_core.h memoryLib/
```

### 3. Build and Test

```bash
make clean
make -j8

# Should compile successfully!
```

---

## FILE CHECKLIST

After extraction, verify you have:

```
memoryLib/
├── MemorySpan.h              ✅ NEW - Span structure
├── SafeMemory.h              ✅ NEW - Clean offset-based interface
├── SafeMemory.cpp            ✅ NEW - Implementation
├── SRMFirmwareRegion.h       ✅ NEW - Read-only firmware
├── SRMFirmwareRegion.cpp     ✅ NEW - Implementation
├── GuestMemory.h             ✅ NEW - Clean router
├── GuestMemory.cpp           ✅ NEW - Routing implementation
├── SparseMemoryBacking.h     ✅ FROM OLD - Proven implementation
├── memory_core.h             ✅ FROM OLD - Types/constants
└── README.md                 ✅ NEW - Full documentation
```

---

## WHAT'S CHANGED

### Removed (NO LONGER NEEDED)
- ❌ AlphaMemorySystem.h
- ❌ AlphaMemorySystem.cpp

### Added (NEW)
- ✅ MemorySpan.h - Safe cross-page access
- ✅ SRMFirmwareRegion.h/cpp - Read-only firmware handler

### Replaced (CLEAN IMPLEMENTATION)
- ♻️ SafeMemory.h/cpp - Now offset-based, handles two PA regions
- ♻️ GuestMemory.h/cpp - Now has offsetBase routing

### Kept (PROVEN)
- ✅ SparseMemoryBacking.h - No changes needed
- ✅ memory_core.h - Types and constants

---

## INTEGRATION (Tomorrow Afternoon)

You'll need to update:

1. **Initialization Code** (see README.md Phase 4, 4.5, 5)
2. **Remove AlphaMemorySystem References** (search & remove)
3. **Add HWRPB Initialization** (Phase 4.5 - new)
4. **Update SRM Loading** (Phase 5 - patch HWRPB)

**Detailed instructions in README.md**

---

## ROLLBACK (If Needed)

```bash
# If something goes wrong:

# Option 1: Restore from git
git reset --hard <backup-commit-hash>

# Option 2: Restore from archive
rm -rf memoryLib
tar -xzf backup-2025-01-27-pre-clean-memory.tar.gz

# Option 3: Rename back
rm -rf memoryLib
mv memoryLib-old-2025-01-27 memoryLib

make clean && make
```

---

## ESTIMATED TIME

| Task                           | Time    |
|--------------------------------|---------|
| Backup (tonight)               | 5 min   |
| Extract & copy files (morning) | 10 min  |
| Build verification             | 5 min   |
| Integration updates            | 2 hours |
| Testing                        | 1 hour  |
| **TOTAL**                      | **~3-4 hours** |

---

## SUCCESS VERIFICATION

After installation:

```bash
# 1. Build succeeds
make clean && make -j8
# No errors

# 2. Verify routing table
./emulator --dump-memory-map
# Should show:
#   [0x0 - 0x10000)           → SafeMemory (low 64 KB)
#   [0x20000000 - 0x20200000) → SRMFirmware (2 MB)
#   [0x80000000 - ...)        → SafeMemory (32 GB)
#   [0xF0000000 - ...)        → MMIOManager (256 MB)
```

---

## NEED HELP?

See full documentation in:
- `memoryLib/README.md` - Complete integration guide
- `CANONICAL_PA_ROUTING_TABLE.md` - PA routing reference
- `ADDRESS_SPACE_ARCHITECTURE_CONTRACT.md` - Architecture contract

---

**Ready to install! Extract zip, copy required files, build!** 🚀
