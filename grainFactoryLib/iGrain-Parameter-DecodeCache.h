#pragma once
#include <QVector>
#include <QtCore/QAtomicInteger>
#include <QAtomicInteger>

// ============================================================================
// Forward Declarations
// ============================================================================
template<typename KeyType, unsigned Ways = 4, unsigned Buckets = 4096>
class DecodeCache;

struct DecodedInstruction; // Also need this forward declaration

// ============================================================================
// Decode Cache Entry - Metadata only
// ============================================================================

template<typename KeyType>
struct DecodeCacheEntry {
    KeyType key;                    // PC or PA (4-byte granularity)
    DecodedInstruction decoded;     // Pre-decoded instruction metadata

    // Cache management
    quint32 generation{ 0 };
    bool valid : 1;
    bool locked : 1;                // Pin hot instructions
    bool transitioning : 1;

    quint8 accessCount{ 0 };        // For LRU eviction

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return valid && !transitioning;
    }
};

// ============================================================================
// Decode Cache Bucket - Set-associative
// ============================================================================

template<typename KeyType, unsigned Ways = 4>
class DecodeCacheBucket {
public:
    DecodeCacheBucket() = default;

    QAtomicInteger<quint32> version{ 0 };
    QAtomicInteger<quint64> occupancy{ 0 };

    DecodeCache<KeyType, Ways>* parent{ nullptr };  // <- Only 2 params (Buckets has default)
    DecodeCacheEntry<KeyType> entries[Ways];        // <- Storage array

    static constexpr quint64 FULL_MASK =
        (Ways == 64) ? ~0ULL : ((1ULL << Ways) - 1ULL);

    // ========================================================================
    // Lock-free lookup
    // ========================================================================

    [[nodiscard]] const DecodedInstruction* find(const KeyType& key) noexcept {
        for (;;) {
            const quint32 v0 = version.loadAcquire();
            if (v0 & 1u) continue;  // Writer active

            const quint64 occ = occupancy.loadRelaxed();
            quint32 v1 = v0;  // <- Declare v1 here

            for (unsigned i = 0; i < Ways; ++i) {
                if (!((occ >> i) & 1ULL)) continue;

                auto* entry = &entries[i];
                if (!entry->isValid()) continue;
                if (!(entry->key == key)) continue;

                // Generation check
                quint32 currentGen = parent->m_generation.loadRelaxed();
                if (entry->generation != currentGen) continue;

                v1 = version.loadAcquire();  // <- Load v1 here
                if (v0 == v1) {
                    // Hot path hit
                    if (entry->accessCount < 255) {
                        entry->accessCount++;
                    }
                    return &entry->decoded;
                }
                break;  // Retry
            }

            if (v1 == v0) {  // <- v1 now declared and assigned
                v1 = version.loadAcquire();  // <- Final check
            }
            if (v0 == v1) return nullptr;  // Stable miss
        }
    }

    // ========================================================================
    // Insert decoded instruction
    // ========================================================================

    bool insert(const KeyType& key, const DecodedInstruction& decoded) noexcept {
        unsigned slot;
        if (!tryClaimSlot(slot)) {
            return false;
        }

        beginWrite();
        entries[slot].key = key;
        entries[slot].decoded = decoded;
        entries[slot].accessCount = 0;
        entries[slot].valid = true;
        entries[slot].generation = parent->m_generation.loadRelaxed();
        endWrite();

        return true;
    }

    // ========================================================================
    // Invalidation
    // ========================================================================

    void invalidateKey(const KeyType& key) noexcept {
        beginWrite();
        const quint64 occ = occupancy.loadRelaxed();

        for (unsigned i = 0; i < Ways; ++i) {
            if (!((occ >> i) & 1ULL)) continue;
            if (entries[i].key == key) {
                invalidateSlot(i);
            }
        }
        endWrite();
    }

private:
    bool tryClaimSlot(unsigned& slot) noexcept {
        for (;;) {
            const quint64 cur = occupancy.loadRelaxed();
            const quint64 used = cur & FULL_MASK;

            if (used == FULL_MASK) {
                slot = findLRU();
                return true;
            }

            const quint64 freeBits = (~used) & FULL_MASK;
            const int bit = qCountTrailingZeroBits(freeBits);
            const quint64 want = cur | (1ULL << bit);

            if (occupancy.testAndSetRelaxed(cur, want)) {
                slot = static_cast<unsigned>(bit);
                return true;
            }
        }
    }

    unsigned findLRU() const noexcept {
        unsigned lru = 0;
        quint8 minCount = 255;

        for (unsigned i = 0; i < Ways; ++i) {
            if (entries[i].locked) continue;
            if (entries[i].accessCount < minCount) {
                minCount = entries[i].accessCount;
                lru = i;
            }
        }
        return lru;
    }

    void invalidateSlot(unsigned slot) noexcept {
        entries[slot].valid = false;
        const quint64 mask = ~(1ULL << slot);
        quint64 cur;
        do {
            cur = occupancy.loadRelaxed();
        } while (!occupancy.testAndSetRelaxed(cur, cur & mask));
    }

    inline void beginWrite() noexcept { version.fetchAndAddRelease(1); }
    inline void endWrite() noexcept { version.fetchAndAddRelease(1); }
};

// ============================================================================
// Global Decode Cache Manager
// ============================================================================

template<typename KeyType, unsigned Ways, unsigned Buckets >
class DecodeCache {
    static_assert((Buckets& (Buckets - 1)) == 0, "Buckets must be power of 2");

public:
    using Bucket = DecodeCacheBucket<KeyType, Ways>;

    DecodeCache() {
        m_buckets.resize(Buckets);
        for (unsigned i = 0; i < Buckets; ++i) {
            m_buckets[i].parent = this;
        }
    }

    // ========================================================================
    // Lookup decoded instruction
    // ========================================================================

    [[nodiscard]] const DecodedInstruction* lookup(const KeyType& key) noexcept {
        const unsigned idx = bucketIndex(key);
        return m_buckets[idx].find(key);
    }

    // ========================================================================
    // Insert decoded instruction
    // ========================================================================

    bool insert(const KeyType& key, const DecodedInstruction& decoded) noexcept {
        const unsigned idx = bucketIndex(key);
        return m_buckets[idx].insert(key, decoded);
    }

    // ========================================================================
    // Invalidation
    // ========================================================================

    void invalidate(const KeyType& key) noexcept {
        const unsigned idx = bucketIndex(key);
        m_buckets[idx].invalidateKey(key);
    }

    void invalidateAll() noexcept {
        m_generation.fetchAndAddRelaxed(1);
    }

    // Make m_generation accessible to bucket
    QAtomicInteger<quint32> m_generation{ 0 };

private:
    static unsigned bucketIndex(const KeyType& key) noexcept {
        return static_cast<unsigned>(key.hash()) & (Buckets - 1);
    }

    QVector<Bucket> m_buckets;
};