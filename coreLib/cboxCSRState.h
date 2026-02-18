#ifndef CBOXCSRSTATE_H
#define CBOXCSRSTATE_H

#include "types_core.h"
#include <QtGlobal>
#include <QVector>
#include <cstring>   // std::memset

// ============================================================================
// CboxCSRState
// ============================================================================
// This structure models the CBOX CSR state that is *not* architecturally
// visible via standard IPR reads, but instead is accessed through the
// CBOX CSR shift-port (C_DATA / C_SHFT).
//
// Architecturally, the CBOX controls:
//   - Bcache (secondary cache) configuration and enable
//   - Duplicate tag behavior
//   - Various timing / clock ratio parameters
//   - Error reporting and test / diagnostic modes
//
// The Alpha AXP System Reference Manual (ASA, 1994) describes the architectural
// role of caches and write buffers in:
//
//   Chapter 5, System Architecture and Programming Implications
//     Section 5.5  "Physical Address Space Characteristics"
//     Section 5.6  "Translation Buffers and Virtual Caches"
//     Subsection    "Caches and Write Buffers"
//
// Exact CBOX CSR layouts, bit positions, and SROM chain formats are *CPU
// implementation specific* and are defined in the processor hardware reference
// manuals (for example, the 21164 or 21264 HRMs). This header provides a
// decoded view used by the emulator; the mapping from raw shift-register
// chains into these decoded fields must be implemented with reference to the
// appropriate HRM tables for the target CPU family.
// ============================================================================

struct CboxCSRState
{
	// ------------------------------------------------------------------------
	// WRITE_ONCE chain
	// ------------------------------------------------------------------------
	// Implementation note:
	//   For EV5 / EV56 style parts the write-once chain is a fixed-length
	//   serial configuration chain loaded from SROM at reset, and generally
	//   includes Bcache sizing, core clock ratios, and some test / strap
	//   values. It is usually documented as a fixed number of bits
	//   (for example, 367 bits) grouped into several quadwords.
	//
	//   This emulator stores it as an array of quadwords for convenience.
	//
	//   Reference: Implementation-specific HRM for the target CPU.
	// ------------------------------------------------------------------------
	quint64 writeOnceChain[6]{};   // 6 * 64 bits = 384 bits (enough for a 367-bit chain)

	// ------------------------------------------------------------------------
	// WRITE_MANY chain
	// ------------------------------------------------------------------------
	// Implementation note:
	//   The write-many chain is generally larger and may contain CSRs that
	//   can be updated at runtime (for example, performance-related CBOX
	//   controls, diagnostic enables, and some error handling controls).
	//
	//   The exact length and content of this chain are defined by the
	//   implementation HRM (for example, 21164 or 21264 HRM CBOX chapter).
	// ------------------------------------------------------------------------
	quint64 writeManyChain[32]{};  // Implementation-dependent maximum

	// ------------------------------------------------------------------------
	// Decoded frequently-accessed CSRs
	// ------------------------------------------------------------------------
	// This nested struct caches *derived* values that the emulator needs
	// frequently, so we do not have to repeatedly re-scan the SROM chains
	// on every use.
	//
	// All bit-level mappings from the shift chains into these fields are
	// implementation-specific and must be provided by decode helpers that
	// reference the relevant HRM tables.
	//
	// Architecturally, the existence and effect of these controls is consistent
	// with ASA Chapter 5, which describes caches, coherency, and write buffers,
	// even though the exact hardware fields are not architecturally visible.
// ============================================================================
	struct DecodedCSRs
	{
		// --------------------------------------------------------------------
		// Bcache geometry and timing
		// --------------------------------------------------------------------
		// These fields describe the effective Bcache configuration as seen
		// by the emulator. They should be derived from the write-once chain
		// and any run-time override fields in the write-many chain.
		//
		// ASA 1994, Chapter 5, Section "Caches and Write Buffers" describes
		// the architectural model of caches, including line sizes and cache
		// behavior with respect to memory operations.
		// --------------------------------------------------------------------

		quint32 bc_size_quads{};        // Bcache size in quadwords (derived)
		quint32 bc_size_bytes{};        // Bcache size in bytes (derived)
		quint32 bc_line_size_bytes{};   // Bcache line size (bytes)
		quint32 bc_assoc{};             // Bcache associativity (ways)
		quint32 bc_num_sets{};          // Number of Bcache sets

		// Original high level fields you already had:
		quint32 bc_size{};              // Raw encoded size field from CSR
		quint32 bc_clk_delay{};         // Encoded Bcache clock delay
		quint32 sys_clk_ratio{};        // Encoded system clock ratio field

		// Derived timing values:
		quint32 bc_clk_delay_cycles{};  // Bcache latency (cycles)
		quint32 cpu_to_bcache_clk_ratio{}; // CPU : Bcache clock ratio
		quint32 cpu_to_mem_clk_ratio{};    // CPU : external memory clock ratio

		// --------------------------------------------------------------------
		// Bcache enable / mode flags
		// --------------------------------------------------------------------
		// These flags control whether the Bcache participates in normal
		// memory traffic, whether duplicate tags are enabled, and how
		// write traffic is handled (write-back vs write-through, gathering,
		// victim behavior).
		// --------------------------------------------------------------------

		bool bc_enable{};              // Bcache enabled (true if active)
		bool dup_tag_enable{};         // Duplicate tag mode enable
		bool bc_write_back_mode{};     // true if Bcache is write-back
		bool bc_write_allocate{};      // true if write allocate is enabled
		bool bc_parity_enable{};       // true if parity checking is enabled
		bool bc_ecc_enable{};          // true if ECC is enabled (if impl.)
		bool bc_tag_test_mode{};       // true if tag test / diagnostic mode
		bool bc_data_test_mode{};      // true if data test / diagnostic mode

		// --------------------------------------------------------------------
		// Write buffer / victim buffer controls
		// --------------------------------------------------------------------
		// ASA Chapter 5 discusses write buffers and their relationship to
		// memory ordering and coherency. The specific control fields are
		// implementation-defined in the CBOX CSRs.
		// --------------------------------------------------------------------

		bool write_buffer_enable{};     // Global write buffer enable
		bool write_gather_enable{};     // Enable write gathering
		bool victim_buffer_enable{};    // Enable victim buffer
		quint32 write_buffer_depth{};   // Number of write buffer entries
		quint32 victim_buffer_depth{};  // Number of victim buffer entries

		// --------------------------------------------------------------------
		// Error reporting and correction controls
		// --------------------------------------------------------------------
		// These reflect CBOX-side control of error detection and reporting
		// for Bcache and memory interface.
		// --------------------------------------------------------------------

		bool correctable_error_int_enable{};   // Enable corrected error interrupt
		bool uncorrectable_error_int_enable{}; // Enable uncorrectable error interrupt
		bool bc_snoop_parity_enable{};         // Enable parity checking on snoops
		bool bc_syndrome_latch_enable{};       // Enable error-syndrome latching

		// --------------------------------------------------------------------
		// Miscellaneous CBOX controls
		// --------------------------------------------------------------------
		// These fields group additional CSR bits that may influence global
		// system behavior, but which the emulator may treat in a simplified
		// way.
		// --------------------------------------------------------------------

		bool cbox_diag_mode{};          // General CBOX diagnostic mode
		bool cbox_force_idle{};         // Force CBOX / Bcache to idle
		bool cbox_perf_mon_enable{};    // Enable CBOX-specific perf counters

		quint32 reserved_impl0{};       // Implementation-specific decoded value
		quint32 reserved_impl1{};       // Implementation-specific decoded value

		// --------------------------------------------------------------------
		// Maintenance helpers
		// --------------------------------------------------------------------
		// clear() is used to reset decoded state when the chains are reset,
		// or when a CPU is initialized.
		// --------------------------------------------------------------------
		inline void clear() noexcept
		{
			std::memset(this, 0, sizeof(DecodedCSRs));
		}
	} decoded;

	// ------------------------------------------------------------------------
	// Reset CBOX CSR state
	// ------------------------------------------------------------------------
	// This helper clears both raw shift-register chains and the decoded
	// hot-path fields. It should be called on CPU reset and whenever PALcode
	// performs a full CBOX reinitialization sequence.
	// ------------------------------------------------------------------------
	inline void reset() noexcept
	{
		std::memset(writeOnceChain, 0, sizeof(writeOnceChain));
		std::memset(writeManyChain, 0, sizeof(writeManyChain));
		decoded.clear();
	}

	// ------------------------------------------------------------------------
	// Decoding interface
	// ------------------------------------------------------------------------
	// These helpers are intended to be called by the IPR / PALcode emulation
	// when the CBOX CSR chains are updated via the C_DATA / C_SHFT IPRs.
	//
	// Note:
	//   The *exact* implementation of these functions depends on the bit-level
	//   definition of the write-once and write-many chains in the specific
	//   CPU hardware reference manual and is therefore left as a set of
	//   detailed TODO items.
	// ------------------------------------------------------------------------

	// Decode all known CSRs from both chains.
	inline void decodeAll() noexcept
	{
		decodeWriteOnce();
		decodeWriteMany();
	}

	// Decode fields that are sourced from the write-once chain.
	inline void decodeWriteOnce() noexcept
	{
		// TODO: Map implementation-specific write-once chain bits into:
		//   - decoded.bc_size
		//   - decoded.bc_size_quads
		//   - decoded.bc_size_bytes
		//   - decoded.bc_line_size_bytes
		//   - decoded.bc_assoc
		//   - decoded.bc_num_sets
		//   - decoded.bc_clk_delay
		//   - decoded.bc_clk_delay_cycles
		//   - decoded.sys_clk_ratio
		//   - decoded.cpu_to_bcache_clk_ratio
		//   - decoded.cpu_to_mem_clk_ratio
		//   - decoded.bc_enable
		//   - decoded.dup_tag_enable
		//
		// Use the target CPU's HRM (CBOX / Bcache configuration chapter) as
		// the authoritative source for these mappings. For example, for a
		// 21164-based system, consult the 21164 HRM tables that describe the
		// write-once configuration chain and the fields which select:
		//   - Bcache size (Kbytes)
		//   - Bcache organization (ways, sets, line size)
		//   - Bcache enable / strap options
		//   - Processor clock to Bcache / memory clock ratios.
		//
		// For now, we leave the decoded fields cleared; the emulator can still
		// operate if it uses default model parameters until the mapping is
		// implemented.
	}

	// Decode fields that are sourced from the write-many chain.
	inline void decodeWriteMany() noexcept
	{
		// TODO: Map implementation-specific write-many chain bits into:
		//   - decoded.bc_write_back_mode
		//   - decoded.bc_write_allocate
		//   - decoded.bc_parity_enable
		//   - decoded.bc_ecc_enable
		//   - decoded.bc_tag_test_mode
		//   - decoded.bc_data_test_mode
		//   - decoded.write_buffer_enable
		//   - decoded.write_gather_enable
		//   - decoded.victim_buffer_enable
		//   - decoded.write_buffer_depth
		//   - decoded.victim_buffer_depth
		//   - decoded.correctable_error_int_enable
		//   - decoded.uncorrectable_error_int_enable
		//   - decoded.bc_snoop_parity_enable
		//   - decoded.bc_syndrome_latch_enable
		//   - decoded.cbox_diag_mode
		//   - decoded.cbox_force_idle
		//   - decoded.cbox_perf_mon_enable
		//   - decoded.reserved_impl0 / reserved_impl1
		//
		// Reference:
		//   Implementation-specific CPU HRM (CBOX / CSR chapter) which defines
		//   the write-many chain bitfields for:
		//     - Bcache error reporting and handling
		//     - Write buffer and victim buffer controls
		//     - CBOX diagnostics and performance monitoring.
		//
		// Until this mapping is implemented, these decoded fields will remain
		// cleared and the emulator may treat them as "disabled" or default.
	}
};

// ============================================================================
// CboxCSRBank - Per-CPU CBOX CSR state
// ============================================================================
// This provides a per-CPU bank of CboxCSRState, similar to IPRBank, so that
// each logical CPU core has its own CBOX configuration and decoded state.
//
// The size of the bank is determined at construction time using getCPUCount(),
// which should return the configured number of CPUs for the emulator.
// ============================================================================

struct CboxCSRBank
{
	QVector<CboxCSRState> perCpuState;

	explicit CboxCSRBank(int cpuCount)
		: perCpuState(cpuCount)
	{
	}

	CboxCSRState& operator[](CPUIdType cpuId)
	{
		return perCpuState[cpuId];
	}

	const CboxCSRState& operator[](CPUIdType cpuId) const
	{
		return perCpuState[cpuId];
	}

	inline int cpuCount() const noexcept
	{
		return perCpuState.size();
	}
	inline int getCPUCount() noexcept{ return perCpuState.count(); }
};

// ---------------------------------------------------------------------------
// Global accessor for the CBOX CSR bank
// ---------------------------------------------------------------------------
// This uses a function-local static to lazily construct the per-CPU bank.
// The helper getCPUCount() is expected to be defined in your core library
// (for example, alongside your EmulatorManager / CPU configuration helpers).
// ---------------------------------------------------------------------------
inline CboxCSRBank& global_CboxCSRs(CPUIdType cpuId)
{
	static CboxCSRBank bank(cpuId);
	return bank;
}

#endif // CBOXCSRSTATE_H
