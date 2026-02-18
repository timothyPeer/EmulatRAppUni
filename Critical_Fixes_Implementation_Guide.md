# Critical Fixes Implementation Guide
## Week 1 Priority Repairs - AlphaCPU Architecture

**Date:** 2026-01-15  
**Priority:** CRITICAL  
**Timeline:** 3-5 days

---

## Fix #1: Remove Bypass Path (HIGHEST PRIORITY)

### Problem
AlphaCPU::runOneInstruction() calls `m_iBox->step()` which bypasses the entire 3-phase boundary controller.

### Impact
- **CATASTROPHIC** - Every instruction bypasses contract guarantees
- Interrupt recognition bypassed
- Trap handling bypassed
- PAL entry bypassed

### Solution

#### Step 1: Fix AlphaCPU::runOneInstruction()

**File:** `AlphaCPU.h` (line 217+)

**Current Code:**
```cpp
AXP_HOT AXP_FLATTEN void runOneInstruction() {
    // Boundary checks ...
    
    m_iBox->step();  // ❌ REMOVE THIS
}
```

**Replace With:**
```cpp
AXP_HOT AXP_FLATTEN void runOneInstruction() {
    // ========================================================================
    // 1. Handle pending events first
    // ========================================================================
    handlePendingEventInLoop();
    
    // ========================================================================
    // 2. Call IBox boundary controller - THE ONLY EXECUTION PATH
    // ========================================================================
    StepResult result = m_iBox->stepOneInstruction();
    
    // ========================================================================
    // 3. Handle rare outcomes (most instructions just return Committed)
    // ========================================================================
    switch (result.type) {
        case StepResultType::Committed:
            // Normal case - do nothing, PC already updated
            break;
            
        case StepResultType::Halted:
            m_halted.store(true, std::memory_order_release);
            emit halted(m_cpuId, static_cast<quint32>(result.metadata1));
            break;
            
        case StepResultType::Redirect:
            // IBox already handled redirect, just note it
            DEBUG_LOG(QString("CPU %1: Redirect handled - reason %2")
                .arg(m_cpuId)
                .arg(static_cast<int>(result.redirectReason)));
            break;
            
        case StepResultType::Fault:
            // Fault already dispatched by IBox boundary handler
            break;
    }
}
```

#### Step 2: Delete or Deprecate IBox::step()

**File:** `IBoxBase.h` (line 248)

**Option A: Delete Completely (RECOMMENDED)**
```cpp
// DELETE THIS METHOD ENTIRELY:
// void step() noexcept { getPipeLine()->step(); }
```

**Option B: Deprecate and Redirect**
```cpp
[[deprecated("Use stepOneInstruction() - this bypasses boundary logic")]]
void step() noexcept { 
    WARN_LOG("IBox::step() is deprecated - use stepOneInstruction()");
    // Force use of proper entry point
    stepOneInstruction();
}
```

**Recommendation:** Delete completely. Any code calling this is violating the contract.

---

## Fix #2: Connect Pipeline to IBox::stepOneInstruction()

### Problem
IBox::stepOneInstruction() Phase 2 calls legacy `getPipeLine()->step()` and hardcodes result as "Committed".

### Impact
- Pipeline outcome never reported
- PAL entry redirects never triggered
- Fault outcomes lost

### Solution

#### Step 1: Define Pipeline Execute Method

**File:** `AlphaPipeline.h` (create or update)

```cpp
class AlphaPipeline {
public:
    // ... existing methods ...
    
    /**
     * @brief Execute one instruction and return outcome
     * @return StepResult describing what happened
     */
    StepResult executeOneInstruction() noexcept;
    
private:
    // ... existing members ...
};
```

#### Step 2: Implement Pipeline Execute Method

**File:** `AlphaPipeline.cpp` (create or update)

```cpp
StepResult AlphaPipeline::executeOneInstruction() noexcept
{
    StepResult result;
    result.type = StepResultType::Committed;  // Default
    
    // 1. Fetch/Decode (if not already done)
    // ... your existing fetch logic ...
    
    // 2. Execute instruction via Box delegation
    BoxResult boxResult = delegateToBox(slot);
    
    // 3. Determine outcome based on execution
    if (boxResult.needsEnterPalmode()) {
        // CALL_PAL detected
        result.type = StepResultType::Redirect;
        result.redirectReason = RedirectReason::PALEntry;
        result.metadata1 = slot.palDecoded.palFunction;  // PAL function
        result.metadata2 = slot.di.pc;                   // callPC
        return result;  // NO COMMIT
    }
    
    if (slot.faultPending) {
        // Fault during execution
        result.type = StepResultType::Fault;
        result.redirectReason = RedirectReason::Trap;
        result.metadata1 = static_cast<quint64>(slot.trapCode);
        return result;  // NO COMMIT
    }
    
    // 4. Commit instruction (update PC, writeback registers)
    commitInstruction(slot);
    
    result.type = StepResultType::Committed;
    result.nextPC = m_ctx->getPC();  // PC after commit
    
    return result;
}
```

#### Step 3: Update IBox::stepOneInstruction() Phase 2

**File:** `iBoxBase.cpp` (line 372-395)

**Current Code:**
```cpp
// PHASE 2: Execute One Instruction
getPipeLine()->step();

// TODO: Replace with:
// StepResult result = getPipeLine()->executeOneInstruction();

// Temporary: simulate Committed result
StepResult result;
result.type = StepResultType::Committed;
result.nextPC = m_ctx->getPC();
```

**Replace With:**
```cpp
// ====================================================================
// PHASE 2: Execute One Instruction
// ====================================================================

StepResult result = getPipeLine()->executeOneInstruction();

// ====================================================================
// PHASE 3: Apply Boundary Actions
// ====================================================================

switch (result.type) {
    case StepResultType::Committed:
        // Normal case - instruction committed, PC updated
        return result;
        
    case StepResultType::Redirect:
        // PAL entry, trap, or interrupt - handle redirect
        return handleRedirect(result);
        
    case StepResultType::Fault:
        // Synchronous fault - already handled in handleTrapBoundary
        // (if needed, could call it here)
        return result;
        
    case StepResultType::Halted:
        // CPU halt instruction executed
        return result;
}
```

---

## Fix #3: Complete PAL Entry Redirect Handler

### Problem
IBox::handleRedirect() has TODO comment for PAL entry routing.

### Impact
CALL_PAL instructions won't enter PAL mode correctly.

### Solution

#### Step 1: Fix PalBox::computePALEntry()

**File:** `PalBoxBase.cpp` (line 35-45)

**Current Code:**
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

**Replace With:**
```cpp
PALEntryInfo PalBox::computePALEntry(quint8 palFunction, quint64 callPC) const noexcept
{
    PALEntryInfo info;
    
    // Get PAL_BASE from IPR storage
    quint64 palBase = m_ctx->readIPR(IPR::PAL_BASE);
    
    // Use architectural calculation from CallPalHelpers_inl.h
    info.vectorPC = calculateCallPalEntryPC(palBase, palFunction);
    info.shadowRegsActive = true;
    info.exc_addr_value = callPC;
    
    DEBUG_LOG(QString("PalBox: Computed PAL entry func=0x%1 vector=0x%2")
        .arg(palFunction, 2, 16, QChar('0'))
        .arg(info.vectorPC, 16, 16, QChar('0')));
    
    return info;
}
```

**Add Include:** At top of `PalBoxBase.cpp`:
```cpp
#include "../palLib_EV6/CallPalHelpers_inl.h"
```

#### Step 2: Uncomment PAL Entry Code in IBox::handleRedirect()

**File:** `iBoxBase.cpp` (line 470-483)

**Current Code:**
```cpp
case RedirectReason::PALEntry: {
    // Route to PalBox
    // TODO: Implement when PalBox has computePALEntry
    // PALEntryInfo entry = getPBox()->computePALEntry(
    //     static_cast<quint8>(pipelineResult.metadata1),  // palFunction
    //     pipelineResult.metadata2  // callPC
    // );
    // vectorPC = entry.vectorPC;
    
    vectorPC = 0x4000;  // Placeholder PAL base
    break;
}
```

**Replace With:**
```cpp
case RedirectReason::PALEntry: {
    // Route to PalBox to compute PAL entry vector
    quint8 palFunction = static_cast<quint8>(pipelineResult.metadata1);
    quint64 callPC = pipelineResult.metadata2;
    
    // Validate PAL function
    quint8 currentMode = m_ctx->getMode();
    if (!isValidCallPalFunction(palFunction, currentMode)) {
        // Invalid PAL function - raise OPCDEC
        PendingEvent ev = makeIllegalInstruction(
            TrapCode_Class::ILLEGAL_INSTRUCTION, callPC);
        m_faultSink->setPendingEvent(ev);
        
        // Return to trap handler
        vectorPC = computeTrapVector(ExceptionClass_EV6::OpcDec);
    } else {
        // Valid PAL function - compute entry point
        PALEntryInfo entry = getPBox()->computePALEntry(palFunction, callPC);
        vectorPC = entry.vectorPC;
        
        // Save exception address
        m_ctx->setEXC_ADDR(entry.exc_addr_value);
        
        // Activate shadow registers if needed
        if (entry.shadowRegsActive) {
            // TODO: Activate shadow register bank
        }
    }
    
    DEBUG_LOG(QString("IBox: PAL entry redirect func=0x%1 vector=0x%2")
        .arg(palFunction, 2, 16, QChar('0'))
        .arg(vectorPC, 16, 16, QChar('0')));
    break;
}
```

**Add Include:** At top of `iBoxBase.cpp`:
```cpp
#include "../palLib_EV6/CallPalHelpers_inl.h"
```

---

## Fix #4: Verify No Other Bypass Paths

### Checklist

Run this audit on all execution-related files:

**Search Pattern:** Any calls to `pipeline->step()`, `pipeline->execute()`, or direct PC manipulation

**Files to Audit:**
- [ ] AlphaCPU.cpp - Only stepOneInstruction() path
- [ ] All grain execute() methods - No PC updates
- [ ] All Box execute() methods - No PC updates
- [ ] MBox, EBox, FBox, PalBox - No direct PC writes

**Approved PC Update Sites:**
1. ✓ AlphaPipeline::commitInstruction() - Commit path
2. ✓ IBox::handleInterruptBoundary() - Interrupt redirect
3. ✓ IBox::handleTrapBoundary() - Trap redirect
4. ✓ IBox::handleRedirect() - PAL/trap/interrupt redirect

**Any other PC write is a CONTRACT VIOLATION**

---

## Testing Strategy

### Unit Tests (Create These)

**File:** `test_IBox_BoundaryLogic.cpp`

```cpp
TEST(IBox, stepOneInstruction_InterruptPending_CallsHandler)
{
    // Setup: Interrupt pending, recognition rules met
    MockIRQController irq;
    irq.setPendingInterrupt(m_cpuId, 5);  // IPL 5
    
    // Execute
    StepResult result = m_iBox->stepOneInstruction();
    
    // Verify: Interrupt handled, no instruction executed
    EXPECT_EQ(result.type, StepResultType::Redirect);
    EXPECT_EQ(result.redirectReason, RedirectReason::Interrupt);
    EXPECT_GT(result.nextPC, 0);  // Vector PC set
}

TEST(IBox, stepOneInstruction_NormalInstruction_Commits)
{
    // Setup: No interrupts, normal instruction
    m_ctx->setPC(0x10000);
    
    // Execute
    StepResult result = m_iBox->stepOneInstruction();
    
    // Verify: Committed, PC advanced
    EXPECT_EQ(result.type, StepResultType::Committed);
    EXPECT_EQ(result.nextPC, 0x10004);  // PC + 4
}

TEST(IBox, stepOneInstruction_CALLPAL_Redirects)
{
    // Setup: CALL_PAL instruction
    setPipelineResult(StepResultType::Redirect, 
                     RedirectReason::PALEntry,
                     0x86);  // IMB function
    
    // Execute
    StepResult result = m_iBox->stepOneInstruction();
    
    // Verify: Redirect to PAL, no commit
    EXPECT_EQ(result.type, StepResultType::Redirect);
    EXPECT_EQ(result.redirectReason, RedirectReason::PALEntry);
    EXPECT_NE(result.nextPC, 0x10004);  // Not sequential
}
```

### Integration Tests

**File:** `test_AlphaCPU_ExecutionFlow.cpp`

```cpp
TEST(AlphaCPU, runOneInstruction_CallsStepOneInstruction)
{
    // Verify the bypass path is removed
    MockIBox* mockIBox = new MockIBox();
    m_cpu->setIBox(mockIBox);
    
    // Execute
    m_cpu->runOneInstruction();
    
    // Verify: stepOneInstruction called exactly once
    EXPECT_EQ(mockIBox->stepOneInstructionCallCount(), 1);
    EXPECT_EQ(mockIBox->stepCallCount(), 0);  // Legacy step NOT called
}
```

---

## Validation Checklist

Before committing:

### Code Changes
- [ ] AlphaCPU::runOneInstruction() calls stepOneInstruction()
- [ ] IBox::step() deleted or deprecated
- [ ] AlphaPipeline::executeOneInstruction() implemented
- [ ] IBox::stepOneInstruction() receives pipeline result
- [ ] IBox::handleRedirect() PAL entry code uncommented
- [ ] PalBox::computePALEntry() uses helper function
- [ ] All includes added (CallPalHelpers_inl.h)

### Testing
- [ ] Unit tests pass for IBox boundary logic
- [ ] Integration tests pass for full execution flow
- [ ] CALL_PAL test executes successfully
- [ ] Interrupt delivery test passes
- [ ] Normal instruction commit test passes

### Documentation
- [ ] Update architecture docs with 3-phase flow
- [ ] Document StepResult types and usage
- [ ] Add comments explaining boundary logic
- [ ] Update contract compliance status

---

## Rollout Plan

### Day 1: Bypass Removal
1. Fix AlphaCPU::runOneInstruction()
2. Delete IBox::step()
3. Run basic execution tests

### Day 2: Pipeline Connection
1. Implement AlphaPipeline::executeOneInstruction()
2. Update IBox::stepOneInstruction() Phase 2
3. Test with normal instructions

### Day 3: PAL Entry
1. Fix PalBox::computePALEntry()
2. Uncomment handleRedirect() PAL code
3. Test CALL_PAL flow

### Day 4: Testing & Validation
1. Run full test suite
2. Audit all PC update sites
3. Verify no bypass paths remain

### Day 5: Documentation & Review
1. Update architecture docs
2. Code review
3. Merge to main

---

## Risk Mitigation

### Risks
1. **Breaking existing functionality** - Test incrementally
2. **Performance regression** - Profile before/after
3. **Hidden bypass paths** - Comprehensive audit

### Mitigations
1. Maintain old code commented out during transition
2. Feature flag for new vs old path (for A/B testing)
3. Extensive logging of execution flow during testing

---

## Success Criteria

✅ **Fix is successful when:**

1. All instructions execute through stepOneInstruction()
2. No calls to legacy step() method exist
3. PAL entry redirects work correctly
4. Interrupt/trap boundaries function
5. All tests pass
6. Performance within 5% of baseline
7. Contract compliance increases to 85%+

---

**END OF IMPLEMENTATION GUIDE**
