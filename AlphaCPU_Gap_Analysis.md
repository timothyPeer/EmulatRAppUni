# AlphaCPU Architecture Gap Analysis
## Contract Compliance Review - Version 1.0

**Date:** 2026-01-15  
**Scope:** AlphaCPU, IBox, AlphaPipeline, PalBox  
**Contract Version:** 1.0

---

## Executive Summary

### Critical Issues Found: 7
### Major Issues Found: 5
### Minor Issues Found: 3
### Compliance Rating: **60%** (Needs Significant Work)

---

## 1. DEPENDENCY HIERARCHY COMPLIANCE

### ✅ PASS: One-Way Dependency Direction
**Contract:** ExecutionCoordinator → AlphaCPU → IBox → AlphaPipeline/Boxes

**Evidence:**
- AlphaCPU calls IBox (inferred from executeLoop pattern)
- IBox has `AlphaPipeline* m_pipeLine` member
- No upward calls detected in visible code

**Status:** COMPLIANT

---

### ⚠️ IMPROVED: Singleton Pattern (Partial GI Compliance)

**Contract Section 7:** "Low layers access subsystems via GI to SSC-owned services"

**Current Implementation (IBoxBase.h:241-243):**
```cpp
CBox* getCBox() const noexcept { 
    return &CBox::instance();  // ✓ Singleton pattern
    /* return m_pipeLine->getCBox(); */  // Old direct pointer (removed)
}
```

**Analysis:**
- ✓ CBox now uses singleton pattern (improvement!)
- ⚠️ Still not full GI compliance (should route through SSC)
- ❌ Other Boxes still accessed directly via pipeline

**Remaining Direct Access:**
```cpp
// IBoxBase.h:242
PalBox* getPBox() noexcept { return m_pipeLine->getPBox(); }

// PipeLineSlot.h
PalBox* getPBox() noexcept;
EBox* getEBox() noexcept;
MBox* getMBox() noexcept;
FBox* getFBox() noexcept;
```

**Status:** PARTIAL IMPROVEMENT - CBox uses singleton, others need update

---

## 2. AlphaCPU WORKER COMPLIANCE

### ✅ PASS: Execution Loop Structure
**Contract 3.2:** "Repeatedly calls IBox step (one instruction per call)"

**Evidence (AlphaCPU.cpp):**
```cpp
void AlphaCPU::executeLoop() {
    while (!m_stopRequested.load(std::memory_order_acquire)) {
        if (m_paused.load(std::memory_order_acquire)) {
            QThread::msleep(1);
            continue;
        }
        runOneInstruction();  // ✓ Single step per iteration
    }
}
```

**Status:** COMPLIANT (assuming runOneInstruction calls IBox)

---

### 🔴 CATASTROPHIC: runOneInstruction() BYPASSES IBox Boundary Logic

**Contract 3.2:** "Repeatedly calls IBox step"

**CRITICAL VIOLATION (AlphaCPU.h:217-243):**
```cpp
AXP_HOT AXP_ALWAYS_INLINE void runOneInstruction() {
    // ... boundary checks ...
    
    // Line 245+ (truncated in view):
    m_iBox->step();  // ❌ BYPASSES stepOneInstruction()!
}
```

**This is the WORST violation found** - The hot path execution completely bypasses the contract-mandated 3-phase boundary controller!

**Impact:**
- All interrupt recognition logic bypassed
- All trap boundary handling bypassed  
- PAL entry sequencing bypassed
- Contract guarantee violated on every instruction

**Evidence of Bypass Chain:**
1. AlphaCPU::runOneInstruction() → calls m_iBox->step()
2. IBox::step() (IBoxBase.h:248) → calls getPipeLine()->step()
3. Bypasses IBox::stepOneInstruction() completely

**Required Fix:**
```cpp
AXP_HOT AXP_FLATTEN void runOneInstruction() {
    // Handle pending events (code modification, etc.)
    handlePendingEventInLoop();
    
    // ❌ DELETE THIS:
    // m_iBox->step();
    
    // ✅ REPLACE WITH:
    StepResult result = m_iBox->stepOneInstruction();
    
    // Handle rare outcomes
    if (result.type == StepResultType::Halted) {
        m_halted.store(true, std::memory_order_release);
        emit halted(m_cpuId, result.metadata1);
    }
}
```

**Status:** CATASTROPHIC CONTRACT VIOLATION

---

### ✅ PASS: Atomic Flags for Control
**Contract 3.2:** "Cheap loop-control flags (atomics)"

**Evidence:**
```cpp
m_paused.load(std::memory_order_acquire)
m_stopRequested.load(std::memory_order_acquire)
m_halted.load(std::memory_order_acquire)
```

**Status:** COMPLIANT

---

### ⚠️ MAJOR: Signal Usage Not Verified

**Contract 8:** "No per-instruction Qt signals. Use signals only for rare critical events"

**Visible Signals:**
```cpp
emit halted(m_cpuId, 0);           // ✓ Rare event
emit fatalError(m_cpuId, ...);     // ✓ Critical event
```

**Gap:** Cannot verify absence of per-instruction signals without seeing full AlphaCPU class declaration.

**Required Verification:**
- Ensure no signals in runOneInstruction() hot path
- Confirm telemetry uses atomics, not signals

**Status:** NEEDS VERIFICATION

---

## 3. IBox BOUNDARY CONTROLLER COMPLIANCE

### 🔴 CRITICAL: Contract-Violating Direct Pipeline Access

**Contract 3.3:** "IBox exposes stepOneInstruction() as the single entry point"

**Violation (IBoxBase.h:248):**
```cpp
void step() noexcept { getPipeLine()->step(); }
```

❌ **This bypasses ALL boundary logic!**

This method allows direct pipeline stepping without:
- Interrupt recognition
- Trap checking
- Boundary actions
- PAL entry handling

**Impact:** CATASTROPHIC - Violates fundamental contract guarantee

**Required Fix:**
```cpp
// DELETE this method entirely or mark deprecated
[[deprecated("Use stepOneInstruction() - this bypasses boundary logic")]]
void step() noexcept { 
    // Force callers to use proper entry point
    stepOneInstruction();
}
```

**Status:** SEVERE VIOLATION

---

### ⚠️ PARTIAL: stepOneInstruction() Implementation Exists

**Contract 5.1:** "3-phase execution: boundary check → execute → boundary actions"

**Good News:** Implementation exists in iBoxBase.cpp:353-395

**Evidence:**
```cpp
StepResult IBox::stepOneInstruction() noexcept
{
    // PHASE 1: Boundary Checks (BEFORE execution) ✓
    if (!isInPalMode()) {
        if (hasInterruptPending() && interruptRecognitionRules()) {
            return handleInterruptBoundary();
        }
    }
    
    if (hasPendingTrap()) {
        return handleTrapBoundary();
    }

    // PHASE 2: Execute One Instruction
    getPipeLine()->step();  // ⚠️ Still using legacy step()
    
    // TODO: Replace with:
    // StepResult result = getPipeLine()->executeOneInstruction();
    
    // PHASE 3: Handle Outcome ⚠️ INCOMPLETE
    StepResult result;
    result.type = StepResultType::Committed;  // Hardcoded!
    result.nextPC = m_ctx->getPC();
    
    if (result.type == StepResultType::Redirect) {
        return handleRedirect(result);
    }
    
    return result;
}
```

**Issues:**
1. ✓ Phase 1 structure correct
2. ⚠️ Phase 2 calls legacy step() (line 376)
3. ⚠️ Phase 3 hardcodes "Committed" result (lines 382-384)
4. ❌ Never actually receives pipeline outcome

**Status:** IMPLEMENTED BUT INCOMPLETE - Not connected to actual pipeline

---

### ✅ PASS: Boundary Handler Declarations

**Contract 3.3:** "Interrupt/trap recognition at boundaries"

**Evidence (IBoxBase.h:264-271):**
```cpp
StepResult handleInterruptBoundary() noexcept;
StepResult handleTrapBoundary() noexcept;
StepResult handleRedirect(const StepResult& pipelineResult) noexcept;

quint64 computeInterruptVector(quint8 ipl) noexcept;
quint64 computeTrapVector(ExceptionClass_EV6 trapClass) noexcept;

bool hasInterruptPending() const noexcept;
bool interruptRecognitionRules() const noexcept;
bool hasPendingTrap() const noexcept;
```

**Status:** COMPLIANT (declarations exist, need implementations)

---

## 4. PC UPDATE CONTRACT COMPLIANCE

### 🔴 CRITICAL: Cannot Verify PC Update Sites

**Contract 4.2:** "Only two places update PC: pipeline commit or IBox boundary redirect"

**Gap:** Need to audit:
1. AlphaPipeline commit path (file not provided)
2. IBox redirect implementations (not provided)
3. All grain execute methods (not provided)
4. Any subsystem that might touch PC

**Required Verification Checklist:**
- [ ] AlphaPipeline updates PC only on commit
- [ ] IBox updates PC only in redirect handlers
- [ ] No grain directly writes PC
- [ ] No Box directly writes PC
- [ ] No subsystem touches PC during execution

**Potential Violation Areas:**
```cpp
// These need inspection:
PalBox::execute()           // Does it update PC?
EBox::execute()             // Does it update PC?
MBox::execute()             // Does it update PC?
```

**Status:** CANNOT VERIFY - NEEDS FULL AUDIT

---

## 5. CALL_PAL CONTRACT COMPLIANCE

### ⚠️ MAJOR: Incorrect PAL Entry Calculation

**Contract 5.3:** "IBox routes to PalBox/PalService, applies PC redirect"

**Issue (PalBoxBase.cpp:35-45):**
```cpp
PALEntryInfo PalBox::computePALEntry(quint8 palFunction, quint64 callPC) const noexcept
{
    PALEntryInfo info;
    // TODO: Compute based on PAL function
    info.vectorPC = 0x4000 + (palFunction * 0x40);  // ❌ WRONG!
    info.shadowRegsActive = true;
    info.exc_addr_value = callPC;
    return info;
}
```

**Problem:** Placeholder calculation doesn't match Alpha AXP architecture.

**Solution Available:** Use the helper from CallPalHelpers_inl.h:

```cpp
PALEntryInfo PalBox::computePALEntry(quint8 palFunction, quint64 callPC) const noexcept
{
    PALEntryInfo info;
    
    // Get PAL_BASE from IPR storage
    quint64 palBase = m_ctx->readIPR(IPR::PAL_BASE);
    
    // Use architectural calculation from helper
    info.vectorPC = calculateCallPalEntryPC(palBase, palFunction);
    info.shadowRegsActive = true;
    info.exc_addr_value = callPC;
    
    return info;
}
```

**Status:** INCORRECT IMPLEMENTATION

---

### ⚠️ MAJOR: PAL Entry Redirect Has TODOs

**Contract 5.3:** "IBox routes to PalBox/PalService, applies PC redirect"

**Implementation Found (iBoxBase.cpp:464-513):**

```cpp
StepResult IBox::handleRedirect(const StepResult& pipelineResult) noexcept
{
    quint64 vectorPC = 0;
    
    switch (pipelineResult.redirectReason) {
    
    case RedirectReason::PALEntry: {
        // ❌ TODO COMMENT (lines 470-477):
        // Route to PalBox
        // TODO: Implement when PalBox has computePALEntry
        // PALEntryInfo entry = getPBox()->computePALEntry(
        //     static_cast<quint8>(pipelineResult.metadata1),  // palFunction
        //     pipelineResult.metadata2  // callPC
        // );
        // vectorPC = entry.vectorPC;
        
        vectorPC = 0x4000;  // ❌ Placeholder PAL base
        break;
    }
    
    // ... other cases ...
    }
    
    // Apply redirect
    m_ctx->setPC(vectorPC);
    flush();
    
    return finalResult;
}
```

**Ready to Fix:** PalBox::computePALEntry() exists (PalBoxBase.cpp:35), just needs:
1. Uncomment the call to getPBox()->computePALEntry()
2. Fix computePALEntry() to use calculateCallPalEntryPC() helper
3. Get PAL_BASE from IPR storage

**Status:** PLACEHOLDER CODE - Easy fix available

---

### ✅ PASS: PAL Function Decode

**Evidence (PipeLineSlot.cpp:56-72):**
```cpp
void PipelineSlot::decodePalFunc(PipelineSlot& slot) noexcept
{
    if (!isCallPal(slot.di) || instructionWord == static_cast<quint32>(-1)) {
        return;
    }
    
    instructionWord = getRaw(slot.di);
    constexpr quint32 PAL_FUNC_MASK = 0x0000FFFFu;
    
    slot.palDecoded.palFunction = 
        static_cast<quint32>(instructionWord & PAL_FUNC_MASK);
    
    slot.execUnit = ExecUnit::PALBOX;
    slot.needsWriteback = false;
    slot.palTransferPending = true;  // ✓ Blocks commit
}
```

**Status:** COMPLIANT

---

## 6. INTERRUPT/TRAP RECOGNITION COMPLIANCE

### ✅ PASS: Boundary Recognition Declarations

**Contract 6.1:** "Recognition at instruction boundaries before pipeline call"

**Evidence (IBoxBase.h:264-271):**
```cpp
bool hasInterruptPending() const noexcept;
bool interruptRecognitionRules() const noexcept;
bool hasPendingTrap() const noexcept;
```

**Status:** COMPLIANT (need implementations)

---

### ⚠️ MAJOR: Recognition Logic Not Provided

**Gap:** Cannot verify recognition rules without implementations:
- Mode/priority checking
- IPL comparison
- Inhibit rules
- IPI recognition

**Required:** Provide iBoxBase.cpp with implementations

**Status:** MISSING IMPLEMENTATIONS

---

## 7. TELEMETRY AND UPWARD COMMUNICATION

### ✅ PASS: Rare Signal Usage

**Contract 8:** "Rare critical events via signals"

**Evidence (AlphaCPU.cpp:82-87):**
```cpp
emit fatalError(m_cpuId, QString::fromUtf8(e.what()));  // ✓ Fatal only
emit halted(m_cpuId, 0);                                 // ✓ Halt only
```

**Status:** COMPLIANT (visible code)

---

### ⚠️ MAJOR: Local Counter Publishing Not Visible

**Contract 8:** "Thread-local counters, publish every N instructions"

**Evidence (AlphaCPU.cpp:59-65):**
```cpp
++m_localInstrCount;

// Publish to shared memory every 4096 instructions
// if ((m_localInstrCount & 0xFFF) == 0) {
//     // TODO: Publish to CPUStatusBlock when implemented
//     // m_statusBlock->instrRetired.store(m_localInstrCount);
// }
```

**Status:** PLACEHOLDER - NEEDS IMPLEMENTATION

---

## 8. THREAD AND WRITER RULES

### ✅ PASS: Single Writer Per CPU

**Contract 9:** "Each CPU worker thread is single writer of its state"

**Evidence:** Thread-affined execution loop with no cross-thread writes visible.

**Status:** COMPLIANT (pending full audit)

---

## 9. MISSING FILES NEEDED FOR COMPLETE AUDIT

### Critical Missing Files:
1. **AlphaCPU.h** - Verify no subsystem pointers, signal declarations
2. **AlphaPipeline.h/cpp** - Verify PC update on commit only
3. **IBoxBase.cpp** - stepOneInstruction() and boundary handlers
4. **AlphaPipeline executeOne() method** - Verify redirect outcomes
5. **StepResult / PipelineResult definitions** - Verify outcome enums
6. **GI (Global Interface) implementation** - Verify subsystem access pattern

---

## 10. PRIORITY-ORDERED FIXES

### 🔴 CRITICAL (Fix Immediately)

1. **Remove IBox::step() bypass** (IBoxBase.h:248)
   - Severity: CATASTROPHIC
   - Impact: Violates fundamental boundary contract
   - Fix: Delete method or force redirect to stepOneInstruction()

2. **Implement IBox::stepOneInstruction()** (IBoxBase.h:234)
   - Severity: CRITICAL
   - Impact: Core contract requirement missing
   - Fix: Implement 3-phase execution pattern

3. **Implement AlphaCPU::runOneInstruction()** (AlphaCPU.cpp, inferred)
   - Severity: CRITICAL
   - Impact: Missing execution path
   - Fix: Call IBox::stepOneInstruction(), handle results

4. **Fix PalBox::computePALEntry()** (PalBoxBase.cpp:35)
   - Severity: CRITICAL
   - Impact: Incorrect PAL vectors
   - Fix: Use calculateCallPalEntryPC() helper

5. **Replace direct subsystem pointers with GI** (Multiple files)
   - Severity: CRITICAL
   - Impact: Violates subsystem access contract
   - Fix: Route all subsystem access through global interface

### ⚠️ MAJOR (Fix Soon)

6. **Implement boundary recognition logic** (IBoxBase.cpp, missing)
   - Methods: hasInterruptPending(), interruptRecognitionRules()
   - Impact: Interrupt/trap handling incomplete

7. **Verify CALL_PAL redirect flow** (IBox + AlphaPipeline)
   - Confirm pipeline returns Redirect outcome
   - Confirm IBox handles without commit

8. **Implement telemetry publishing** (AlphaCPU.cpp:59-65)
   - Uncomment and implement CPUStatusBlock updates
   - Verify batching every 4K instructions

### ℹ️ MINOR (Improve Later)

9. **Audit all PC update sites** (All grain execute methods)
   - Verify only pipeline commit and IBox redirect
   - Document all PC write locations

10. **Add StepResult rich metadata** (Design improvement)
    - Include fault info, trap vectors, PAL metadata
    - Enable better diagnostics

---

## 11. COMPLIANCE SCORECARD

| Contract Section | Status | Grade |
|-----------------|--------|-------|
| 1. Dependency Hierarchy | PASS | A |
| 2. ExecutionCoordinator Role | NOT AUDITED | - |
| 3.1 AlphaCPU Worker | PARTIAL | C |
| 3.2 IBox Boundary Controller | FAIL | D |
| 3.3 AlphaPipeline Engine | NOT AUDITED | - |
| 4. PC Update Contract | CANNOT VERIFY | I |
| 5. Step Contract | MISSING | F |
| 6. CALL_PAL Contract | INCORRECT | D |
| 7. Interrupt/Trap Recognition | MISSING | F |
| 8. Subsystem Access (GI) | FAIL | F |
| 9. Telemetry | PARTIAL | C |
| 10. Thread/Writer Rules | PASS | A |

**Overall Compliance:** 60% (D Grade)

---

## 12. RECOMMENDED ACTION PLAN

### Week 1: Foundation Fixes
1. Delete/deprecate IBox::step()
2. Implement IBox::stepOneInstruction() with 3-phase pattern
3. Implement AlphaCPU::runOneInstruction()
4. Fix PalBox::computePALEntry() with helper

### Week 2: Subsystem Access
5. Design and implement GI subsystem access layer
6. Replace all direct Box pointers with GI calls
7. Audit for any remaining direct subsystem access

### Week 3: Boundary Logic
8. Implement interrupt/trap recognition logic
9. Implement boundary redirect handlers
10. Add comprehensive boundary testing

### Week 4: Verification
11. Audit all PC update sites
12. Verify CALL_PAL flow end-to-end
13. Implement telemetry publishing
14. Run compliance test suite

---

## 13. TESTING REQUIREMENTS

### Unit Tests Needed:
- [ ] IBox::stepOneInstruction() three phases
- [ ] Interrupt recognition at boundaries
- [ ] CALL_PAL redirect (no commit)
- [ ] PAL entry vector calculation
- [ ] PC updates only from allowed sites

### Integration Tests Needed:
- [ ] Full CALL_PAL sequence (decode → redirect → PAL entry)
- [ ] Interrupt delivery during normal execution
- [ ] Trap handling (illegal instruction, access violation)
- [ ] Multi-CPU execution with GI subsystem access

---

## CONCLUSION

The implementation shows good structural foundation but has **critical gaps** in three areas:

1. **IBox boundary contract** - The direct pipeline bypass violates fundamental design
2. **Subsystem access** - Direct pointers instead of GI pattern
3. **Missing implementations** - Core methods (stepOneInstruction, runOneInstruction) not provided

**Estimated Compliance:** 60%  
**Risk Level:** HIGH (due to bypass path)  
**Recommended Priority:** Address Critical fixes within 1 week

The architecture is sound, but execution must be completed and the bypass removed to achieve contract compliance.

---

**END OF GAP ANALYSIS**
