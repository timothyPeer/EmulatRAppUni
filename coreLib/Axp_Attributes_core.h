#ifndef AXP_ATTRIBUTES_CORE_H
#define AXP_ATTRIBUTES_CORE_H

// ============================================================================
// Compiler Detection
// ============================================================================

#if defined(_MSC_VER)
#define AXP_COMPILER_MSVC 1
#else
#define AXP_COMPILER_MSVC 0
#endif

#if defined(__clang__)
#define AXP_COMPILER_CLANG 1
#else
#define AXP_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !AXP_COMPILER_CLANG
#define AXP_COMPILER_GCC 1
#else
#define AXP_COMPILER_GCC 0
#endif

// ============================================================================
// INLINING HINTS
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_ALWAYS_INLINE inline __forceinline
#define AXP_NEVER_INLINE __declspec(noinline)
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_ALWAYS_INLINE inline __attribute__((always_inline))
#define AXP_NEVER_INLINE __attribute__((noinline))
#else
#define AXP_ALWAYS_INLINE inline
#define AXP_NEVER_INLINE
#endif

// ============================================================================
// HOT/COLD PATH OPTIMIZATION
// ============================================================================

#if AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_HOT  __attribute__((hot))
#define AXP_COLD __attribute__((cold))
#else
#define AXP_HOT
#define AXP_COLD
#endif

// ============================================================================
// FLATTEN - Inline ALL calls within this function
// Use for instruction dispatch where you want zero call overhead
// ============================================================================

#if AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_FLATTEN __attribute__((flatten))
#else
#define AXP_FLATTEN
#endif

// ============================================================================
// PURE/CONST - Function has no side effects
// ============================================================================

#if AXP_COMPILER_GCC || AXP_COMPILER_CLANG
	// PURE: Return value depends only on parameters (may read globals)
#define AXP_PURE __attribute__((pure))

// CONST: Return value depends ONLY on parameters (no memory reads)
#define AXP_CONST __attribute__((const))
#else
#define AXP_PURE
#define AXP_CONST
#endif

// ============================================================================
// BRANCH PREDICTION HINTS
// ============================================================================

#if AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_LIKELY(x)   (__builtin_expect(!!(x), 1))
#define AXP_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define AXP_LIKELY(x)   (x)
#define AXP_UNLIKELY(x) (x)
#endif

// ============================================================================
// UNREACHABLE CODE
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_UNREACHABLE() __assume(0)
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_UNREACHABLE() __builtin_unreachable()
#else
#define AXP_UNREACHABLE() ((void)0)
#endif

// ============================================================================
// ASSUME - Optimization hint
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_ASSUME(cond) __assume(cond)
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define AXP_ASSUME(cond) ((void)0)
#endif

// ============================================================================
// RESTRICT - Pointer aliasing hint (critical for memory ops)
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_RESTRICT __restrict
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_RESTRICT __restrict__
#else
#define AXP_RESTRICT
#endif

// ============================================================================
// PREFETCH - Cache line prefetch hints
// ============================================================================

#if AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_PREFETCH_READ(addr)  __builtin_prefetch(addr, 0, 3)
#define AXP_PREFETCH_WRITE(addr) __builtin_prefetch(addr, 1, 3)
#else
#define AXP_PREFETCH_READ(addr)  ((void)0)
#define AXP_PREFETCH_WRITE(addr) ((void)0)
#endif

// ============================================================================
// NODISCARD - Return value should not be ignored
// ============================================================================

#if __cplusplus >= 201703L
#define AXP_NODISCARD [[nodiscard]]
#else
#define AXP_NODISCARD
#endif

// ============================================================================
// PACKED - Structure packing (no padding)
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_PACKED __declspec(align(1))
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_PACKED __attribute__((packed))
#else
#define AXP_PACKED
#endif

// ============================================================================
// ALIGNED - Specify alignment
// ============================================================================

#if AXP_COMPILER_MSVC
#define AXP_ALIGNED(n) __declspec(align(n))
#elif AXP_COMPILER_GCC || AXP_COMPILER_CLANG
#define AXP_ALIGNED(n) __attribute__((aligned(n)))
#else
#define AXP_ALIGNED(n)
#endif

#endif // AXP_ATTRIBUTES_CORE_H

/*

CALL_PAL Executors: 

AXP_HOT AXP_FLATTEN void executeCALL_PAL_SWPIPL(PipelineSlot& slot) noexcept
{
	// Hot + always inline = maximum speed
}

Grain Execute:
AXP_HOT AXP_FLATTEN void execute(PipelineSlot& slot, AlphaProcessorContext* ctx) const override
{
	// Flatten inlines ALL calls within execute
	ctx.getMBox()->executeCALL_PAL_SWPIPL(slot);
}

Pure Getters: 

AXP_PURE AXP_FLATTEN
inline quint8 getIPL_Active(CPUIdType cpuId) noexcept
{
	return globalIPRHot(cpuId).ipl;
}

AXP_CONST AXP_FLATTEN
inline quint64 signExtend(quint32 value) noexcept
{
	// CONST = even better than PURE (no memory reads)
	return static_cast<qint32>(value);
}

Memory Operations: 

AXP_HOT bool readPA_Quad(quint64 pa, quint64* AXP_RESTRICT out) noexcept
{
	// RESTRICT tells compiler 'out' doesn't alias other pointers
}


Branch Hints.

if (AXP_LIKELY(perm & AccessKind::READ)) {
	// Fast path
} else {
	// Fault path (unlikely)
}

*/