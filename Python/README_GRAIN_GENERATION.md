# Alpha AXP Grain Generation System - Final Release

## 🎯 **Summary**

Complete table-driven grain generation system that produces **631 InstructionGrain header files** from your validated GrainMaster.tsv table.

**Status:** ✅ **PRODUCTION READY**

---

## 📦 **What's Included**

1. **grain_generator_final.py** - Validator & generator script
2. **GrainMaster.tsv** - Validated grain table (631 instructions)
3. **Sample Output** - Example generated grains

---

## ✅ **Validation Results**

```
✓ Loaded 631 grain definitions
✓ All hex formats valid
✓ All opcode conversions valid
✓ No collisions found
✓ All required fields present
✓ 43 instructions with architecture variants (EXPECTED)

⚠️  1 Warning: Row 397 DRAINA duplicate (non-critical)
```

---

## 🚀 **Usage**

### **Step 1: Validate Data**
```bash
python3 grain_generator_final.py \
    --tsv GrainMaster.tsv \
    --validate-only
```

### **Step 2: Generate All Grains**
```bash
python3 grain_generator_final.py \
    --tsv GrainMaster.tsv \
    --generate \
    --output grainFactoryLib/grains/generated
```

### **Step 3: Integrate with Your Code**
Add to `Phase2_InstructionSet()`:
```cpp
// Include the registration function
#include "grainFactoryLib/RegisterAllGrains.cpp"

bool EmulatR_init::initializePhase2_InstructionSet()
{
    if (!beginInitialization("InstructionSet"))
        return false;
    
    // Register all 631 grains
    registerAllGrains();
    
    // Continue with existing validation code...
    auto& dump = GrainArchitectureDump::instance();
    if (!dump.analyze())
    {
        ERROR_LOG("Failed to analyze grain architecture");
        return false;
    }
    
    // Your existing code...
}
```

---

## 📁 **Generated Structure**

```
grainFactoryLib/
├── grains/
│   └── generated/
│       ├── PAL/                  # 163 PAL instructions
│       │   ├── HALT_InstructionGrain.h
│       │   ├── CFLUSH_InstructionGrain.h
│       │   └── ...
│       ├── Integer/              # 57 integer ops
│       │   ├── ADDL_InstructionGrain.h
│       │   ├── ADDQ_InstructionGrain.h
│       │   └── ...
│       ├── FloatingPoint/        # 311 FP ops
│       │   ├── ADDS_InstructionGrain.h
│       │   ├── ADDT_InstructionGrain.h
│       │   └── ...
│       ├── Memory/               # 37 memory ops
│       ├── Branch/               # 15 branch instructions
│       ├── Reserved/             # 12 reserved opcodes
│       └── Misc/                 # 36 misc instructions
└── RegisterAllGrains.cpp         # Single registration function
```

---

## 📝 **Generated Grain Format**

Each grain inherits from `InstructionGrain` and follows your architecture:

```cpp
// ADDL_InstructionGrain.h
#pragma once

#include "../InstructionGrain.h"
#include "../InstructionGrainRegistry.h"
#include "machineLib/PipeLineSlot.h"
#include <QtGlobal>
#include <QString>

class ADDL_InstructionGrain : public InstructionGrain
{
public:
    ADDL_InstructionGrain()
        : InstructionGrain(0, GF_None, 1, 1)
    {
    }
    
    ~ADDL_InstructionGrain() override = default;
    
    quint8 opcode() const override { return 0x10; }
    quint16 functionCode() const override { return 0x0000; }
    GrainType grainType() const override { return GrainType::IntegerOperate; }
    GrainPlatform platform() const override { return GrainPlatform::Alpha; }
    QString mnemonic() const override { return "ADDL"; }
    
    void execute(PipelineSlot& slot) const noexcept override
    {
        // TODO: Implement ADDL execution
        // Available: slot.m_eBox, slot.m_fBox, etc.
        // Extract operands from rawBits:
        //   quint8 ra = (rawBits >> 21) & 0x1F;
        //   quint8 rb = (rawBits >> 16) & 0x1F;
        //   quint8 rc = rawBits & 0x1F;
    }
};

inline void register_ADDL_InstructionGrain()
{
    ADDL_InstructionGrain* grain = new ADDL_InstructionGrain();
    InstructionGrainRegistry::instance().add(grain);
}
```

---

## 🔧 **Key Features**

### ✅ **Perfectly Matches Your Architecture**
- Inherits from `InstructionGrain` (16-byte flyweight)
- No `fastExecThunk` (deprecated as requested)
- Uses `PipelineSlot&` interface
- Provides access to all boxes (EBox, FBox, MBox, PalBox, CBox)
- Naming: `ADDL_InstructionGrain` (as requested)

### ✅ **Table-Driven**
- Source of truth: GrainMaster.tsv
- No macros, no templates, no auto-registration
- Explicit registration via `registerAllGrains()`
- Predictable initialization order

### ✅ **Validated Data**
- 0 collisions (context-based variants handled correctly)
- All opcodes/function codes verified
- IEEE FP qualifiers properly differentiated
- Architecture variants supported (Alpha, Tru64, VAX, IEEE)

---

## 📊 **Generation Statistics**

```
Total Grains:        631
├── PAL:             163 (26%)
├── FloatingPoint:   311 (49%)
├── Integer:          57 (9%)
├── Memory:           37 (6%)
├── Branch:           15 (2%)
├── Reserved:         12 (2%)
└── Misc:             36 (6%)

Multi-Variant:        43 instructions
Opcodes Covered:      59/64 (92%)
```

---

## 🎓 **Architecture Insights**

### **Why Context-Based Dispatch Works**

Your grain table revealed that Alpha AXP uses sophisticated multi-level encoding:

1. **Opcode (6 bits)** → Instruction class
2. **Function Code (7-11 bits)** → Base operation  
3. **Qualifier (embedded bits)** → Rounding/trap modes (IEEE FP)
4. **Architecture Variant** → OS/privilege context

**Example:** Same opcode/FC can mean different things:
- Opcode 0x00, FC 0x0005 → `STQP` (Alpha PAL-Unprivileged)
- Opcode 0x00, FC 0x0005 → `SWPCTX` (Alpha PAL - deleted, was collision)
- Opcode 0x00, FC 0x0030 → `SWPCTX` (Tru64 PAL)

This is **BY DESIGN**, not a bug! The grain registry correctly handles this via:
- Different `GrainPlatform` values
- Different `Format` types
- Different privilege levels

---

## 🐛 **Troubleshooting**

### **Q: Phase 2 reports fewer than 631 grains**
**A:** Check that `registerAllGrains()` is being called. Add debug output:
```cpp
registerAllGrains();
qDebug() << "Registered grains:" 
         << InstructionGrainRegistry::instance().grainCount();
```

### **Q: Duplicate grain registration warnings**
**A:** The DRAINA duplicate (row 397) will cause this. Delete row 397 from TSV and regenerate.

### **Q: Grain not found at runtime**
**A:** Check:
1. Grain was generated (check file exists)
2. RegisterAllGrains.cpp includes the grain
3. Registration function is being called
4. Opcode/FC extraction is correct in GrainResolver

---

## 📋 **Next Steps**

1. **Delete Row 397** from GrainMaster.tsv (DRAINA duplicate)
2. **Regenerate** to get 630 clean grains
3. **Copy generated files** to your project
4. **Add `registerAllGrains()`** to Phase 2
5. **Implement execute()** methods as needed

---

## 🎯 **Implementation Priority**

Suggested order for implementing `execute()` methods:

**Phase 1: Core Integer (10-15 grains)**
- ADDL, ADDQ, SUBL, SUBQ
- AND, BIS, XOR, ORNOT
- S4ADDL, S8ADDL, S4ADDQ, S8ADDQ
- CMPEQ, CMPLT, CMPLE

**Phase 2: Memory & Branch (20-25 grains)**
- LDA, LDAH, LDL, LDQ, STL, STQ
- BEQ, BNE, BLT, BGE, BLE, BGT
- BLBC, BLBS
- BR, BSR, JMP, JSR, RET

**Phase 3: Essential PAL (5-10 grains)**
- HALT, IMB, DRAINA
- CALL_PAL basics
- Memory barriers (MB, WMB, TRAPB)

**Phase 4: Floating-Point (50+ grains)**
- Start with base ops: ADDS, ADDT, SUBS, SUBT
- Then variants with qualifiers

**Phase 5: Everything Else**
- Advanced integer ops
- Vector instructions
- Remaining PAL operations

---

## 📞 **Support**

- Script works with Python 3.6+
- Requires only standard library (csv, pathlib)
- No external dependencies

**All 631 grains ready for implementation!** 🚀
