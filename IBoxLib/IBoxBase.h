#ifndef IBOX_BASE_H
#define IBOX_BASE_H
// ============================================================================
// IBoxBase.h - Alpha Instruction Box (Header-Only)
// ============================================================================
// Copyright (c) 2025 EmulatR Project. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root.
// ============================================================================
// Purpose:
//   Alpha CPU IBox - Instruction fetch and decode coordination
//
// Responsibilities (ONLY):
//   - Instruction fetch coordination (PC management)
//   - Instruction decode (via grain resolver)
//   - Decode cache management
//   - FetchResult generation for pipeline
//
// Dependencies:
//   - MBox: VA->PA translation (ITB/DTB)
//   - GuestMemory: Physical memory instruction fetch
//   - CBox: Branch prediction (not here)
// ============================================================================

#include <array>
#include "../coreLib/types_core.h"
#include "../faultLib/FaultDispatcher.h"
#include "../grainFactoryLib/DecodedInstruction.h"
#include "../grainFactoryLib/iGrain-KeyIdenties.h"
#include "../grainFactoryLib/DecodedInstruction_inl.h"
#include "../grainFactoryLib/InstructionGrain.h"
#include "../grainFactoryLib/iGrain-DualLookup_inl.h"
#include "../grainFactoryLib/instructionFormatClassifier.h"
#include "../coreLib/LoggingMacros.h"
#include "../faultLib/PendingEvent_refined.h"
#include "../exceptionLib/ExceptionFactory.h"
#include "../emulatrLib/global_ExecutionCoordinator.h"
#include "coreLib/FetchResult.h"
#include "../memoryLib/GuestMemory.h"
#include "coreLib/ExecTraceMacros.h"
#include "pteLib/ev6Translation_struct.h"

// Forward declarations
struct InstructionGrain;
class GrainResolver;
class BranchPredictor;
struct PcKey;
struct PaKey;
class CBox;
class PalBox;
class ExecutionCoordinator;
class GuestMemory;



// ============================================================================
// Decode Cache Types
// ============================================================================

/**
 * @brief PC-based decode cache (software identity)
 */
class PcDecodeCache {
public:
    static constexpr size_t CACHE_SIZE = 64;
    static constexpr size_t INDEX_MASK = CACHE_SIZE - 1;

    struct CacheEntry {
        PcKey key;
        DecodedInstruction instruction;
        bool valid;

        CacheEntry() : key{}, valid(false) {}
    };

    PcDecodeCache() noexcept {
        invalidate();
    }

    DecodedInstruction* lookup(const PcKey& key) noexcept {
        size_t index = computeIndex(key);
        CacheEntry& entry = m_entries[index];

        if (entry.valid && entry.key == key) {
            return &entry.instruction;
        }

        return nullptr;
    }

    void insert(const PcKey& key, const DecodedInstruction& di) noexcept {
        size_t index = computeIndex(key);
        CacheEntry& entry = m_entries[index];

        entry.key = key;
        entry.instruction = di;
        entry.valid = true;
    }

    void invalidate() noexcept {
        for (auto& entry : m_entries) {
            entry.valid = false;
        }
    }
    void invalidateEntry(const PcKey& key) noexcept {
        size_t index = computeIndex(key);
        m_entries[index].valid = false;
    }

    size_t getValidEntries() const noexcept {
        size_t count = 0;
        for (const auto& entry : m_entries) {
            if (entry.valid) ++count;
        }
        return count;
    }

private:
    std::array<CacheEntry, CACHE_SIZE> m_entries;

    static size_t computeIndex(const PcKey& key) noexcept {
        return key.hash() & INDEX_MASK;
    }
};

/**
 * @brief PA-based decode cache (hardware identity)
 */
class PaDecodeCache {
public:
    static constexpr size_t CACHE_SIZE = 64;
    static constexpr size_t INDEX_MASK = CACHE_SIZE - 1;

    struct CacheEntry {
        PaKey key;
        DecodedInstruction instruction;
        bool valid;

        CacheEntry() : key{}, valid(false) {}
    };

    PaDecodeCache() noexcept {
        invalidate();
    }

    DecodedInstruction* lookup(const PaKey& key) noexcept {
        size_t index = computeIndex(key);
        CacheEntry& entry = m_entries[index];

        if (entry.valid && entry.key == key) {
            return &entry.instruction;
        }

        return nullptr;
    }

    void insert(const PaKey& key, const DecodedInstruction& di) noexcept {
        size_t index = computeIndex(key);
        CacheEntry& entry = m_entries[index];

        entry.key = key;
        entry.instruction = di;
        entry.valid = true;
    }

    void invalidate() noexcept {
        for (auto& entry : m_entries) {
            entry.valid = false;
        }
    }

    void invalidateEntry(const PaKey& key) noexcept {
        size_t index = computeIndex(key);
        m_entries[index].valid = false;
    }

    size_t getValidEntries() const noexcept {
        size_t count = 0;
        for (const auto& entry : m_entries) {
            if (entry.valid) ++count;
        }
        return count;
    }

private:
    std::array<CacheEntry, CACHE_SIZE> m_entries;

    static size_t computeIndex(const PaKey& key) noexcept {
        return key.hash() & INDEX_MASK;
    }
};

// ============================================================================
// IBox Class Declaration
// ============================================================================

class IBox final
{
public:
    // ====================================================================
    // Construction
    // ====================================================================

    explicit IBox(CPUIdType cpuId, ExecutionCoordinator* coordinator,
        FaultDispatcher* faultSink,  GuestMemory* memory) noexcept
        : m_executionCoordinator(coordinator)
        , m_cpuId(cpuId)
        , m_faultSink(faultSink)
        , m_guestMemory(memory)
        , m_iprGlobalMaster(getCPUStateView(cpuId))
    {
        m_ev6Translator.reset(new Ev6Translator(cpuId));
        DEBUG_LOG(QString("CPU %1: IBox initialized").arg(m_cpuId));

    }

    ~IBox() = default;

    IBox(const IBox&) = delete;
    IBox& operator=(const IBox&) = delete;
    IBox(IBox&&) = delete;
    IBox& operator=(IBox&&) = delete;

    // ====================================================================
    // Core Fetch/Decode Interface
    // ====================================================================

    AXP_HOT AXP_FLATTEN FetchResult fetchNext() noexcept {
        FetchResult fr{};
        fr.valid = false;
        fr.m_cpuId = m_cpuId;

        fr.virtualAddress = m_iprGlobalMaster->h->pc;

        if (!fetchAndDecode(fr)) {
            m_stats.translationFaults++;
            return fr;      
        }

        m_stats.fetchCount++;

        DEBUG_LOG(QString("CPU %1: Fetch successful PC=0x%2 grain=%3")
            .arg(m_cpuId)
            .arg(fr.di.pc, 16, 16, QChar('0'))
            .arg(fr.di.grain ? "valid" : "null"));

        return fr;
    }

    AXP_HOT AXP_FLATTEN bool fetchAndDecode(FetchResult& fr) noexcept {
        const quint64 pc = m_iprGlobalMaster->h->pc;
        
        fr.virtualAddress = pc;
        fr.physicalAddress = 0;

        // ================================================================
        // 1) TRY CACHE FIRST
        // ================================================================
        if (tryFetchFromCache(fr)) {
            m_stats.cacheHits++;
            return true;
        }


        // ================================================================  
        // 2) CACHE MISS - FETCH FROM MEMORY
        // ================================================================
        m_stats.cacheMisses++;

        if (!fetchFromMemory(fr)) {
            return false;
        }

        // ================================================================
        // 3) DECODE INSTRUCTION - is performed in fetchFromMemory
        // ================================================================
 //       decodeInstruction(fr.di, fr);

        if (fr.di.grain == nullptr) {
            PendingEvent ev = makeIllegalInstruction(TrapCode_Class::ILLEGAL_INSTRUCTION, pc);
            m_faultSink->setPendingEvent(ev);

            DEBUG_LOG(QString("CPU %1: Decode failed for PC=0x%2 instr=0x%3 - no grain found")
                .arg(m_cpuId)
                .arg(pc, 16, 16, QChar('0'))
                .arg(getRaw(fr.di), 8, 16, QChar('0')));

            return false;
        }

        // ================================================================
        // 4) CHECK FOR CALL_PAL
        // ================================================================
        if (isCallPal(fr.di)) {
            fr.isCallPal = true;
            fr.palFunction = getRaw(fr.di) & 0x7F;

            DEBUG_LOG(QString("CPU %1: CALL_PAL instruction PC=0x%2 function=0x%3")
                .arg(m_cpuId)
                .arg(pc, 16, 16, QChar('0'))
                .arg(fr.palFunction, 2, 16, QChar('0')));
        }

        // ================================================================
        // 5) UPDATE CACHES
        // ================================================================
        updateCaches(fr);

        fr.valid = true;

        DEBUG_LOG(QString("CPU %1: Decode successful PC=0x%2 grain=%3 format=%4")
            .arg(m_cpuId)
            .arg(pc, 16, 16, QChar('0'))
            .arg(fr.di.grain ? "valid" : "null")
            .arg(getInstructionFormatName(getInstructionFormat(fr.di))));

      


        return true;
    }

    // ====================================================================
    // Cache Management
    // ====================================================================

    PcDecodeCache& pcDecodeCache() noexcept { return m_pcCache; }
    const PcDecodeCache& pcDecodeCache() const noexcept { return m_pcCache; }

    PaDecodeCache& paDecodeCache() noexcept { return m_paCache; }
    const PaDecodeCache& paDecodeCache() const noexcept { return m_paCache; }

    void invalidateDecodeCache() noexcept {
        m_pcCache.invalidate();
        m_paCache.invalidate();
        DEBUG_LOG(QString("CPU %1: Decode caches invalidated").arg(m_cpuId));
    }

    // ====================================================================
    // Statistics
    // ====================================================================

    struct FetchStats {
        quint64 fetchCount = 0;
        quint64 cacheHits = 0;
        quint64 cacheMisses = 0;
        quint64 translationFaults = 0;
        quint64 memoryFaults = 0;
    };

    const FetchStats& getStats() const noexcept { return m_stats; }

    void resetStats() noexcept {
        m_stats = {};
        DEBUG_LOG(QString("CPU %1: IBox statistics reset").arg(m_cpuId));
    }

private:
    // ====================================================================
    // Helper Methods
    // ====================================================================

    AXP_HOT AXP_FLATTEN bool tryFetchFromCache(FetchResult& fr) noexcept {
  //      quint64 pc = globalHWPCBController(m_cpuId).pc;

        PcKey pcKey = PcKey::fromVA(fr.virtualAddress);
        if (DecodedInstruction* cached = m_pcCache.lookup(pcKey)) {
            fr.di = *cached;
            fr.pcKey = pcKey;
            fr.valid = true;

            DEBUG_LOG(QString("CPU %1: PC cache hit for 0x%2")
                .arg(m_cpuId).arg(fr.di.pc, 16, 16, QChar('0')));
            return true;
        }

        fr.pcKey = pcKey;
        return false;
    }

    // ====================================================================
// Helper: Convert SPAM TranslationResult to MEM_STATUS
// ====================================================================

    static MEM_STATUS convertTranslationResultToMemStatus(TranslationResult tr) noexcept {
        switch (tr) {
        case TranslationResult::Success:
            return MEM_STATUS::Ok;

        case TranslationResult::TlbMiss:      // ITB miss
        case TranslationResult::DlbMiss:      // DTB miss
            return MEM_STATUS::TlbMiss;

        case TranslationResult::AccessViolation:
            return MEM_STATUS::AccessViolation;

        case TranslationResult::FaultOnRead:
        case TranslationResult::FaultOnWrite:
        case TranslationResult::FaultOnExecute:
            return MEM_STATUS::AccessViolation;

        case TranslationResult::Unaligned:
            return MEM_STATUS::Un_Aligned;

        default:
            return MEM_STATUS::BusError;
        }
    }

   AXP_HOT AXP_ALWAYS_INLINE const DecodedInstruction* fetchAndDecode(quint64 pc, quint64 pa)
    {
        // ========================================================================
        // STEP 1: Try PC cache (virtual address lookup)
        // ========================================================================

        PcKey pcKey = PcKey::fromVA(pc);
        const DecodedInstruction* cachedDI = pcDecodeCache().lookup(pcKey);

        if (cachedDI && cachedDI->grain) {
            //  Cache hit - grain pointer already points to registry
            return cachedDI;
        }

        // ========================================================================
        // STEP 2: Try PA cache (physical address lookup)
        // ========================================================================

        PaKey paKey = PaKey::fromPA(pa);
        cachedDI = paDecodeCache().lookup(paKey);

        if (cachedDI && cachedDI->grain) {
            // PA cache hit - promote to PC cache
            pcDecodeCache().insert(pcKey, *cachedDI);
            return cachedDI;
        }

        // ========================================================================
        // STEP 3: Cache miss - fetch from memory
        // =====================================================================

 
        quint32 rawBits = 0;
        MEM_STATUS status = m_guestMemory->read32(pa, rawBits);

        if (status != MEM_STATUS::Ok) {
            // Fetch failed - instruction access violation
            return nullptr;
        }



        // ========================================================================
        // STEP 4: Resolve grain from registry (NOT allocated - lookup!)
        // ========================================================================

        InstructionGrain* grain = GrainResolver::instance().ResolveGrain(rawBits);

        if (!grain) {
            // Unknown instruction - not in registry
            // This is where your crash happens!
            ERROR_LOG(QString("Unknown instruction: 0x%1 at PC 0x%2")
                .arg(rawBits, 8, 16, QChar('0'))
                .arg(pc, 16, 16, QChar('0')));
            return nullptr;
        }

        // ========================================================================
        // STEP 5: Build DecodedInstruction with grain pointer
        // ========================================================================

        DecodedInstruction di;
        di.grain = grain;  //  Point to registry singleton
        di.pc = pc;
        setRaw(di, rawBits);  // SET IT BEFORE decodeInstruction!

        InstructionGrain* grain2 = GrainResolver::instance().ResolveGrain(rawBits);
        if (!grain2) {
            // Handle unknown instruction gracefully
            ERROR_LOG(QString("Unknown opcode 0x%1 at PC 0x%2")
                .arg((rawBits >> 26) & 0x3F, 2, 16, QChar('0'))
                .arg(pc, 16, 16, QChar('0')));

            // Return nullptr or create illegal instruction grain
            return nullptr;
        }
        if (grain2->opcode() == 0x19)
            qDebug() << "break";

        // Set raw bits in the grain (this is where you crashed!)
        ///grain->setRawBits(rawBits);  //  Safe now - grain is not null -- immutable.


        // ========================================================================
        // STEP 6: Decode metadata (populate di fields)
        // ========================================================================

        FetchResult fetchResult;
        fetchResult.m_cpuId = m_cpuId;
        fetchResult.virtualAddress = pc;    // Store VA before fetch
        fetchResult.physicalAddress = 0;     // Will be filled by fetchFromMemory
        decodeInstruction(di, fetchResult);  // Populates ra, rb, rc, semantics

        // ========================================================================
        // STEP 7: Cache the DecodedInstruction BY VALUE
        // ========================================================================

        paDecodeCache().insert(paKey, di);  // Copies di into PA cache
        pcDecodeCache().insert(pcKey, di);  // Copies di into PC cache

        // Return pointer to cached entry (not our local di!)
        return paDecodeCache().lookup(paKey);
    }

    // ============================================================================
    // fetchFromMemory - Integrated with Dual Cache System
    // ============================================================================

    bool fetchFromMemory(FetchResult& fr) noexcept
    {
        // ========================================================================
        // Initialize FetchResult
        // ========================================================================

        fr.valid = false;
        fr.fetchStatus = MEM_STATUS::Ok;
        fr.isCallPal = false;
        fr.palFunction = 0;

        

        const quint64 pc = fr.virtualAddress;  // Virtual address

        // ========================================================================
        // STEP 1: Translate PC to Physical Address
        // ========================================================================
        quint64 va = 0;
        quint64 pa = 0;
      //  const MEM_STATUS translateStatus = m_mmu->translateInstructionFetch(pc, pa, fr.m_cpuId);
        const TranslationResult translateResult = m_ev6Translator->translateVA_Instruction(
              pc,             // Virtual address (PC)
            pa              // Output: physical address
        );

        // Record to trace BEFORE execution
        

        // Convert TranslationResult to MEM_STATUS
        const MEM_STATUS translateStatus = convertTranslationResultToMemStatus(translateResult);

        fr.physicalAddress = pa;  // Store translated PA in FetchResult

        // ========================================================================
        // STEP 2: Try PC Cache (Virtual Address Lookup - FASTEST)
        // ========================================================================

        PcKey pcKey = PcKey::fromVA(pc);
        const DecodedInstruction* cachedDI = pcDecodeCache().lookup(pcKey);

        if (cachedDI && cachedDI->grain) {
            // Verify PA hasn't changed (page table remap check)
            if (cachedDI->physicalAddress() == pa) {
                // PC CACHE HIT - Fast path!
                fr.di = *cachedDI;  // Copy cached DI to FetchResult
                fr.valid = true;
                qDebug() << cachedDI->getMneumonic() << "*** cachedDI HIT  PC:  0x" << Qt::hex << cachedDI->pc;
                TRACE_LOG(QString("PC cache HIT: PC=0x%1").arg(pc, 16, 16, QChar('0')));
                return true;
            }

            // PA mismatch - page was remapped, invalidate stale PC entry
            DEBUG_LOG(QString("PC cache STALE: PC=0x%1 (PA changed 0x%2 -> 0x%3)")
                .arg(pc, 16, 16, QChar('0'))
                .arg(cachedDI->physicalAddress(), 16, 16, QChar('0'))
                .arg(pa, 16, 16, QChar('0')));

            pcDecodeCache().invalidateEntry(pcKey);
        }

        // ========================================================================
        // STEP 3: Try PA Cache (Physical Address Lookup - Coherent)
        // ========================================================================

        PaKey paKey = PaKey::fromPA(pa);
        cachedDI = paDecodeCache().lookup(paKey);

        if (cachedDI && cachedDI->grain) {
            // PA CACHE HIT - Promote to PC cache for future hits
            fr.di = *cachedDI;
            fr.di.pc = pc;  // Update PC (may differ from cached)
            fr.di.setPhysicalAddress( pa);
            fr.valid = true;

            // Promote to PC cache
            qDebug() << cachedDI->getMneumonic() << "*** Inserted into pcDecodeCache PC:  0x" << Qt::hex << cachedDI->pc;
            pcDecodeCache().insert(pcKey, fr.di);
           
            TRACE_LOG(QString("PA cache HIT: PA=0x%1 (promoted to PC cache)")
                .arg(pa, 16, 16, QChar('0')));
            return true;
        }
        // Record to trace BEFORE execution

        // ========================================================================
        // STEP 4: CACHE MISS - Fetch Raw Instruction from Memory
        // ========================================================================

        DEBUG_LOG(QString("Cache MISS: PC=0x%1 PA=0x%2 (fetch from memory)")
            .arg(pc, 16, 16, QChar('0'))
            .arg(pa, 16, 16, QChar('0')));

        quint32 rawBits = 0;
        const MEM_STATUS fetchStatus = m_guestMemory->readInst32(pa, rawBits);

        qDebug() << QString("Fetched from PA 0x%1: 0x%2")
            .arg(pa, 16, 16, QChar('0'))
            .arg(rawBits, 8, 16, QChar('0'));


        if (fetchStatus != MEM_STATUS::Ok) {
            // Memory fetch failed - bus error, access violation, etc.
            fr.fetchStatus = fetchStatus;
            ERROR_LOG(QString("Memory fetch FAILED: PA=0x%1 status=%2")
                .arg(pa, 16, 16, QChar('0'))
                .arg(static_cast<int>(fetchStatus)));
            return false;
        }

        const quint8 opcode = (rawBits >> 26) & 0x3F;
        if (opcode == 0x19 || opcode == 0x1B || opcode == 0x1D ||
            opcode == 0x1E || opcode == 0x1F)
        {
            qDebug() << "PAL HW opcode:" << Qt::hex << opcode
                << "func:" << ((rawBits >> 0) & 0xFFFF);
        }
        TRACE_LOG(QString("Fetched instruction: 0x%1 from PA=0x%2")
            .arg(rawBits, 8, 16, QChar('0'))
            .arg(pa, 16, 16, QChar('0')));

        // ========================================================================
        // STEP 5: Resolve InstructionGrain from Registry
        // ========================================================================

        InstructionGrain* grain = GrainResolver::instance().ResolveGrain(rawBits);

        if (!grain) {
            // ILLEGAL INSTRUCTION - Not registered in grain registry
            fr.fetchStatus = MEM_STATUS::IllegalInstruction;

            const quint8 opcode = (rawBits >> 26) & 0x3F;
            if (opcode == 26)
            {
                qDebug() << " break here"; // JMP instruction
            }
            const quint16 func = GrainResolver::extractFunctionCode(rawBits, opcode);

            

            ERROR_LOG(QString("ILLEGAL INSTRUCTION: PC=0x%1 PA=0x%2 raw=0x%3 opcode=0x%4 func=0x%5")
                .arg(pc, 16, 16, QChar('0'))
                .arg(pa, 16, 16, QChar('0'))
                .arg(rawBits, 8, 16, QChar('0'))
                .arg(opcode, 2, 16, QChar('0'))
                .arg(func, 4, 16, QChar('0')));

            return false;
        }
        // Record to trace BEFORE execution


        // ========================================================================
        // STEP 6: Build DecodedInstruction
        // ========================================================================
    
        DecodedInstruction& di = fr.di;

        // Set raw bits in grain (safe now - grain is not null)


        // Initialize DecodedInstruction
        di = DecodedInstruction();  // Reset to default state
        // Extra safety : Clear semantic flags(preserve raw instruction)
        di.semantics = 0x0000000000000000;
        di.grain = grain;           // Point to registry grain (flyweight) -- we were not setting semantics... 
        di.pc = pc;
        di.setPhysicalAddress(pa);
        di.setRawBits(rawBits);
      
        TRACE_LOG(QString("Resolved grain: %1 (opcode=0x%2 func=0x%3)")
            .arg(grain->mnemonic())
            .arg(grain->opcode(), 2, 16, QChar('0'))
            .arg(grain->functionCode(), 4, 16, QChar('0')));

        // ========================================================================
        // STEP 7: Decode Instruction Metadata
        // ========================================================================

        decodeInstruction(di, fr);  // Populates ra, rb, rc, semantics, etc.

       

        if (!fr.valid) {
            // Decode failed (shouldn't happen if grain is valid, but defensive)
            ERROR_LOG(QString("Decode FAILED: PC=0x%1 grain=%2")
                .arg(pc, 16, 16, QChar('0'))
                .arg(grain->mnemonic()));
            return false;
        }

      
       

        // ========================================================================
        // Success!
        // ========================================================================

        fr.valid = true;
        fr.fetchStatus = MEM_STATUS::Ok;
        return true;
    }

    AXP_HOT AXP_FLATTEN void updateCaches(const FetchResult& fr) noexcept
    {

        // ========================================================================
        // STEP 8: Cache the DecodedInstruction (BY VALUE)
        // ========================================================================
        // Insert into PA cache (survives context switches)
      
        // ================================================================
        // Guard: Only cache valid fetch results with valid keys
        // ================================================================
        if (!fr.valid) {
            DEBUG_LOG(QString("CPU %1: Skipping cache update - invalid fetch result at PC=0x%2")
                .arg(m_cpuId).arg(fr.di.pc, 16, 16, QChar('0')));
            return;
        }

        // Guard: Verify keys are valid (non-zero)
        if (!fr.paKey.isValid() || !fr.pcKey.isValid()) {
            DEBUG_LOG(QString("CPU %1: Skipping cache update - invalid keys at PC=0x%2")
                .arg(m_cpuId).arg(fr.di.pc, 16, 16, QChar('0')));
            return;
        }

        // Guard: Verify grain is assigned
        if (!fr.di.grain) {
            DEBUG_LOG(QString("CPU %1: Skipping cache update - no grain at PC=0x%2")
                .arg(m_cpuId).arg(fr.di.pc, 16, 16, QChar('0')));
            return;
        }

        // ================================================================
        // Safe to cache
        // ================================================================
        m_paCache.insert(fr.paKey, fr.di);
        m_pcCache.insert(fr.pcKey, fr.di);

        DEBUG_LOG(QString("CPU %1: Updated decode caches for PC=0x%2 (PA=0x%3)")
            .arg(m_cpuId)
            .arg(fr.di.pc, 16, 16, QChar('0'))
            .arg(fr.di.physicalAddress(), 16, 16, QChar('0')));
    }

    void handleTranslationFault(quint64 va, TranslationResult tr) noexcept {
        m_stats.translationFaults++;

        DEBUG_LOG(QString("CPU %1: Translation fault at VA=0x%2 result=%3")
            .arg(m_cpuId)
            .arg(va, 16, 16, QChar('0'))
            .arg(static_cast<int>(tr)));
    }

    void handleMemoryFault(quint64 pa, MEM_STATUS status) noexcept {
        m_stats.memoryFaults++;

        DEBUG_LOG(QString("CPU %1: Memory fault at PA=0x%2 status=%3")
            .arg(m_cpuId)
            .arg(pa, 16, 16, QChar('0'))
            .arg(static_cast<int>(status)));
    }

    // ====================================================================
    // Member Data
    // ====================================================================

    ExecutionCoordinator* m_executionCoordinator;
    CPUIdType m_cpuId;
    FaultDispatcher* m_faultSink;

    GuestMemory* m_guestMemory;

    PcDecodeCache m_pcCache;
    PaDecodeCache m_paCache;

    FetchStats m_stats;
    QScopedPointer<Ev6Translator> m_ev6Translator{ nullptr };

    CPUStateView  m_cpuView;                            // value member
    CPUStateView* m_iprGlobalMaster{ &m_cpuView };
   
};

#endif // IBOX_BASE_H