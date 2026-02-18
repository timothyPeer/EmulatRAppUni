# Alpha AXP Grain Validation Summary

## 📊 Current Status

**Total Grain Definitions:** 635 instructions loaded from GrainMaster.tsv

## ✅ What's Working

1. **Opcode Format:** All opcode decimal/hex conversions are consistent
2. **Required Fields:** All required fields present in all rows
3. **Minimal Duplicates:** Only 1 duplicate found (DRAINA)

## ⚠️ Issues Found

### Minor Format Issues (2 errors)
- Row 596: `0xB` should be `0x0B`
- Row 597: `0xF` should be `0x0F`

**Fix:** Add leading zeros to match 0x00 format

### Opcode/Function Code Collisions (28 detected)

**These are MOSTLY LEGITIMATE** due to Alpha AXP architecture design:

#### Why Collisions Exist:
1. **Multi-OS Support:** Same instruction has different names in:
   - Alpha (base architecture)
   - Tru64 UNIX
   - Common (shared across variants)

2. **Privilege Levels:** Same opcode/function differentiates:
   - Unprivileged vs Privileged operations
   - Different PAL (Privileged Architecture Library) contexts

3. **Architecture Variants:**
   - VAX compatibility instructions
   - IEEE floating-point variants
   - Alpha native instructions

#### Example Collision (LEGITIMATE):
```
COLLISION at opcode=0x00, fc=0x0005, arch=Alpha: 
  - STQP (Row 20)
  - SWPCTX (Row 21) 
  - WRENTRY (Row 409)
```
These are different PAL operations that share the same encoding but differ in context.

## 🔍 Recommended Approach

### Option 1: Context-Based Dispatch (RECOMMENDED)
The grain registry should support **multiple handlers per opcode/fc pair**, selected by:
- Operating system variant (Alpha/Tru64/Common)
- Privilege level
- Execution context

**Implementation:**
```cpp
struct GrainKey {
    uint8_t opcode;
    uint16_t function_code;
    ArchVariant arch;     // Alpha, Tru64, Common, VAX, IEEE
    std::string context;  // PAL-Unprivileged, PAL-Tru64, etc.
};
```

### Option 2: Separate by Architecture
Generate separate grain files for each architecture variant:
- `PAL/Alpha/STQP_Grain.h`
- `PAL/Tru64/SWPCTX_Grain.h`
- `PAL/Common/WRENTRY_Grain.h`

### Option 3: Flatten with Disambiguation
Use fully qualified names:
- `STQP_Alpha_PAL_Grain.h`
- `SWPCTX_Tru64_PAL_Grain.h`

## 📋 Next Steps

1. **Fix Format Issues**
   - Update rows 596, 597 with proper 0x0B, 0x0F format

2. **Decide Collision Strategy**
   - Choose one of the 3 options above
   - Update generator accordingly

3. **Validate Architecture Field**
   - Ensure all grains have correct architecture tags
   - This is critical for proper dispatch

4. **Generate Headers**
   - Run with `--generate` flag once clean

## 🚀 Commands

### Validate Only:
```bash
python3 grain_validator.py --tsv GrainMaster.tsv --validate-only
```

### Validate + Generate:
```bash
python3 grain_validator.py --tsv GrainMaster.tsv --generate --output grainFactoryLib/grains/generated
```

### Generate to Custom Location:
```bash
python3 grain_validator.py --tsv GrainMaster.tsv --generate --output /path/to/output
```

## 📁 Generated Structure

The generator will create subdirectories automatically:
```
grainFactoryLib/grains/generated/
├── PAL/              # PAL instructions
├── Integer/          # Integer operations  
├── FloatingPoint/    # FP operations
├── Memory/           # Memory operations
├── Branch/           # Branch instructions
├── Reserved/         # Reserved opcodes
└── Misc/             # Everything else
```

## 🎯 Recommendation

The "collisions" are actually a **feature, not a bug**. Alpha AXP's design allows the same encoding to have different meanings in different contexts. 

**I recommend Option 1 (Context-Based Dispatch)** because it:
1. Matches the hardware design
2. Allows runtime selection
3. Supports multiple OS variants
4. Most flexible for emulation

Would you like me to update the generator to handle context-based dispatch?
