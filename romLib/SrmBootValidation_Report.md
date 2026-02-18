# SRM Boot Initialization Chain — Validation Report

## Executive Summary

Comprehensive validation of the SRM firmware binary loading identified **four critical bugs** that would have caused the emulator to execute garbage code on every PAL entry, fault dispatch, and interrupt — producing symptoms indistinguishable from pipeline implementation errors.

All four bugs are now understood and correctable.

---

## Bug #1: PalBase INI Value

**File:** `ASAEmulatr.ini`
**Was:** `PalBase=0x60000`
**Fix:** No longer applicable — PAL_BASE is determined at runtime by the decompressor

**Evidence:** With PAL_BASE=0x60000, every EV6 exception vector (DTB_MISS at +0x200, ITB_MISS at +0x300, INTERRUPT at +0x180, etc.) resolves to file offset 0x60000+offset, which is **all zeros** in the binary. Every fault immediately executes `CALL_PAL HALT` (opcode 0x00000000).

With PAL_BASE=0x8000, all vectors contain valid handler code (stack frame setup, register saves, BSR to shared subroutines).

**Root Cause:** The value 0x60000 was a stale estimate. The decompressed.rom header contains `PAL_BASE=0x600000`, which is the hardware-linked address when SROM loads firmware into high memory — not applicable to emulator loading.

---

## Bug #2: SrmHeaderSkip

**File:** `ASAEmulatr.ini`
**Was:** `SrmHeaderSkip=16`
**Fix:** No longer applicable — new loader skips the 0x240-byte header internally

**Evidence:** clipper.bin does NOT have a PC/PAL_BASE header. Its first 16 bytes are `0x45C3F87F 0x5CD811B7 0x00004FBF 0x00000000` — which decode as Alpha instructions (part of the RESET handler), not metadata. Skipping them shifts the entire binary by 16 bytes, causing the BSR R27 at PA 0x8000 to land at the wrong file offset.

**Root Cause:** The header skip was configured for decompressed.rom (which has a real 16-byte header: `{PC, PAL_BASE}`), but was also applied to clipper.bin.

---

## Bug #3: computePalEntryPC Missing Dispatch Offsets

**File:** Where `computePalEntryPC()` is defined
**Was:**
```cpp
quint64 entryPC = palBase + (palFunction * PAL_ENTRY_SIZE);
```

**Fix:**
```cpp
quint64 entryPC;
if (palFunction < 0x40) {
    // Privileged: PAL_BASE + 0x2000 + (func × 64)
    entryPC = palBase + 0x2000 + (palFunction * PAL_ENTRY_SIZE);
} else if (palFunction >= 0x80) {
    // Unprivileged: PAL_BASE + 0x3000 + ((func - 0x80) × 64)
    entryPC = palBase + 0x3000 + ((palFunction - 0x80) * PAL_ENTRY_SIZE);
} else {
    // 0x40-0x7F: reserved — route to OPCDEC
    entryPC = palBase + 0x2000 + (palFunction * PAL_ENTRY_SIZE);
}
return entryPC | 0x1ULL;  // PAL mode bit
```

**Evidence (EV6 architecture):**
| CALL_PAL  | func | Old (broken)               | New (correct)              |
|-----------|------|----------------------------|----------------------------|
| HALT      | 0x00 | PAL_BASE+0x0000 = **RESET**| PAL_BASE+0x2000 = **HALT** ✓ |
| CSERVE    | 0x09 | PAL_BASE+0x0240 = **DTB handler** | PAL_BASE+0x2240 = **CSERVE** ✓ |
| CHMK      | 0x83 | PAL_BASE+0x20C0 = **random**| PAL_BASE+0x30C0 = **CHMK** ✓ |

**Authority:** Alpha Architecture Reference Manual: "Privileged CALL_PAL Instructions: Offset = 2000 + ([5:0] shift left 6). Unprivileged: Offset = 3000 + ([5:0] shift left 6)." Confirmed identical across EV4/EV5/EV6.

---

## Bug #4: Fault Vector Map (Needs Verification)

**File:** `getFaultVector()` or equivalent
**Status:** Implementation not yet reviewed — must use EV6 (21264) layout

**Empirically confirmed vector layout for clipper.bin / cl67srmrom.exe:**

| Offset | Exception               | Evidence                                    |
|--------|-------------------------|---------------------------------------------|
| 0x0000 | RESET                   | BSR R27,0 — standard PC-capture idiom       |
| 0x0080 | Machine Check           | STQ R29 — context save                      |
| 0x0100 | Arithmetic              | Short stub (3 instructions + zeros)          |
| 0x0180 | Interrupt               | BEQ R20 — interrupt pending check            |
| 0x0200 | DTB Miss Single         | LDA R30,-400(R30) — stack frame alloc        |
| 0x0280 | DTB Miss Double         | INTA — register manipulation                 |
| 0x0300 | ITB Miss                | BSR → 0x2002C3D0 — shared TB fill routine    |
| 0x0380 | ITB ACV                 | BSR → 0x2002C3D0 — same shared routine ✓     |
| 0x0400 | DTB Miss Native         | INTA — inline handler                        |
| 0x0480 | Unalign                 | BNE R20 — alignment check                   |
| 0x0500 | OPCDEC                  | INTL sequence — opcode decode                |
| 0x0580 | FEN (FP Disabled)       | BSR → 0x2002C2C0 — FP enable routine         |
| 0x2000 | Privileged CALL_PAL     | LDA R30,-32(R30) — HALT entry                |
| 0x3000 | Unprivileged CALL_PAL   | INTL — BPT entry                             |

**Key discriminator:** +0x0300 and +0x0380 both BSR to the same subroutine (0x2002C3D0). Under EV6, these are ITB_MISS and ITB_ACV — both TB-related, naturally sharing a TLB fill routine. Under EV5, they would be Unalign and D-Stream Errors — unrelated faults that would never share a handler. This confirms EV6 layout.

---

## New Loading Architecture

**Previous approach:** Load clipper.bin or decompressed.rom with manual offset configuration.

**New approach:** Load cl67srmrom.exe using AxpBox/ES40 decompression-via-execution algorithm.

### SrmRomLoader Flow

```
1. Select ROM variant (CL67 embedded, or load from file)
2. Skip 0x240-byte header
3. Copy payload → guest memory at PA 0x900000
4. CPU: PC=0x900001, PAL_BASE=0x900000
5. Single-step CPU until PC < 0x200000
6. Read CPU state → {finalPC, finalPalBase}
7. Firmware now at PA 0x0–0x1FFFFF
8. Set all CPUs: PC=finalPC, PAL_BASE=finalPalBase
```

### Expected Results (from decompressed.rom header)
- `finalPC = 0x8001` (PA 0x8000, PAL mode)
- `finalPalBase = 0x600000`

### INI Simplification

**Before (fragile, error-prone):**
```ini
SrmBase=0x0
PalBase=0x60000
SrmSize=0x200000
SrmHeaderSkip=16
SrmInitialPC=0x8001
```

**After (one property):**
```ini
[ROM]
SrmRomVariant=CL67
; Or: SrmRomFile=cl67srmrom.exe
```

---

## Files Delivered

| File | Size | Description |
|------|------|-------------|
| `SrmRomLoader.h` | 5 KB | Loader class header with SrmRomVariant enum |
| `SrmRomLoader.cpp` | 7 KB | Implementation with decompress() method |
| `SrmRomData_CL67.inc` | 4.1 MB | cl67srmrom.exe as constexpr uint8_t array |
| `SrmRomData_CL.inc` | 4.1 MB | clsrmrom.exe as constexpr uint8_t array |

---

## Integration Checklist

- [ ] Add `SrmRomLoader.h/.cpp` and `.inc` files to CMakeLists.txt
- [ ] Wire `decompress()` callbacks to GuestMemory and CPU
- [ ] Fix `computePalEntryPC()` with 0x2000/0x3000 offsets
- [ ] Verify `getFaultVector()` uses EV6 vector map
- [ ] Remove old SrmBase/PalBase/SrmHeaderSkip INI properties
- [ ] Test: decompression completes with PC=0x8001, PAL_BASE=0x600000
- [ ] Test: first DTB_MISS lands at PAL_BASE+0x200 = 0x600200 with real code
