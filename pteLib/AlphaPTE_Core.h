#ifndef _EMULATRAPPUNI_PTELIB_ALPHAPTE_CORE_H
#define _EMULATRAPPUNI_PTELIB_ALPHAPTE_CORE_H

#include "../coreLib/types_core.h"
#include <QtGlobal>
#include "alpha_pte_core.h"
#include "../coreLib/Axp_Attributes_core.h" 


static constexpr unsigned MAX_SIZE_CLASSES = 4;


// AlphaPTE.H
// Alpha AXP PTE layout based on Alpha Architecture Reference Manual
// and Linux Alpha case-study (see e.g. "Alpha AXP Page Table Entry",
// Figure 13.3). Bits 0..9 are architectural; PFN lives in 63..32.

// AlphaPTE.H
// Canonical Alpha AXP Page Table Entry definition.
// ------------------------------------------------
//
// This class represents the *architectural (memory)* page table entry as
// defined by the Alpha Architecture Reference Manual.
//
// IMPORTANT:
//   This is NOT the same bit layout used by EV6's ITB_PTE, DTB_PTE,
//   ITB_PTE_TEMP, or DTB_PTE_TEMP registers. Those formats differ for
//   hardware refill convenience and PALcode compatibility.
//
//   The canonical PTE in memory uses:
//     - low bits 0..15 for V/FOE/FOW/FOR/ASM/GH and per-mode access bits
//     - PFN located in bits 51..32 (20 bits architecturally,
//       up to 28 bits for EV6 implementations)
//       This implementation uses: 28 bits.
//
//   All ITB/DTB IPR formats (read vs write) must be encoded/decoded into
//   this canonical format by adapter traits (see alpha_pte_traits_ev6_*).
//
// Bit layout of canonical (memory) PTE:
//
//   bit  0  : V     (Valid)
//   bit  1  : FOE   (Fault on Execute)
//   bit  2  : FOW   (Fault on Write)
//   bit  3  : FOR   (Fault on Read)
//   bit  4  : ASM   (Address Space Match)
//   bit  5  : GH    (Granularity Hint)
//   bit  6  : UWE   (User Write Enable)
//   bit  7  : KWE   (Kernel Write Enable)
//   bit  8  : URE   (User Read Enable)
//   bit  9  : KRE   (Kernel Read Enable)
//   bits 10..15 : OS-specific (dirty, accessed, etc.)
//
//   PFN (Page Frame Number) is held in bits 51..32.
//   EV6 implementations may use up to 28 PFN bits (for >4GB physical RAM),
//   but PFN always starts at bit 32.
//
// Summary:
//   - AlphaPTE represents the canonical memory PTE.
//   - EV6-specific TEMP and IPR formats are *adapters*, not canonical.
//   - Accessors V(), asm(), gh(), read/write bits, and pfn() operate
//     strictly on the canonical architectural layout.
//
// References:
//   - Alpha Architecture Reference Manual, 3rd Ed., "Memory Management"
//   - DEC 21264 (EV6) Hardware Reference Manual, ITB/DTB IPR definition
//   - Linux/Alpha: arch/alpha/include/asm/pgtable.h

struct alignas(16) AlphaPTE {
	quint64 raw;
	bool cow;               // COW status (emulator-only extension)


/*	static constexpr quint64 VALID_BIT = 1ULL << 0;   // if you use bit 0 for valid*/
	
	// ---------------------------------------------------------------------
	// Architectural bit positions (low 16 bits)
	// ---------------------------------------------------------------------
	// Use the architectural bit positions defined in AlphaN_S.
	// Source: Alpha AXP Architecture Reference Manual, Memory Management
	//         (EV6 PTE layout: V, FOR, FOW, ASM, GH, access enables, PFN).
	//


    static constexpr unsigned PTE_BIT_V = AlphaN_S::PTE_BIT_V;
    static constexpr quint64 PTE_MASK_V = AlphaN_S::PTE_MASK_V;
	static constexpr unsigned PTE_BIT_FOR = AlphaN_S::PTE_BIT_FOR;
	static constexpr unsigned PTE_BIT_FOW = AlphaN_S::PTE_BIT_FOW;
    static constexpr unsigned PTE_BIT_FOE = AlphaN_S::PTE_BIT_FOE;
	static constexpr unsigned PTE_BIT_ASM = AlphaN_S::PTE_BIT_ASM;


	static constexpr unsigned PTE_BIT_KRE = AlphaN_S::PTE_BIT_KRE;
	static constexpr unsigned PTE_BIT_ERE = AlphaN_S::PTE_BIT_ERE;
	static constexpr unsigned PTE_BIT_SRE = AlphaN_S::PTE_BIT_SRE;
	static constexpr unsigned PTE_BIT_URE = AlphaN_S::PTE_BIT_URE;

	static constexpr unsigned PTE_BIT_KWE = AlphaN_S::PTE_BIT_KWE;
	static constexpr unsigned PTE_BIT_EWE = AlphaN_S::PTE_BIT_EWE;
	static constexpr unsigned PTE_BIT_SWE = AlphaN_S::PTE_BIT_SWE;
	static constexpr unsigned PTE_BIT_UWE = AlphaN_S::PTE_BIT_UWE;

	// PFN geometry comes from types_core.h (PFN_SHIFT, PFN_WIDTH).
	// See DEC 21264 Hardware Reference Manual, Memory Management:
	// PFN is stored in bits 52:32 for EV6-class implementations.

    static constexpr unsigned PTE_BIT_PFN_LSB = 32;   // bit 32
    static constexpr unsigned PTE_BIT_PFN_MSB = 52;   // bit 52
    static constexpr unsigned PTE_PFN_WIDTH =
        (PTE_BIT_PFN_MSB - PTE_BIT_PFN_LSB + 1);   // 21 bits
    static constexpr quint64 PTE_MASK_PFN =
        ((quint64(1) << PTE_PFN_WIDTH) - 1) << PTE_BIT_PFN_LSB;

    static inline quint64 extractPFN(PFNType pte) noexcept
    {
        return (pte >> PTE_BIT_PFN_LSB) & ((quint64(1) << PTE_PFN_WIDTH) - 1);
    }


	// ---------------------------------------------------------------------------
	// PTE GH (Granularity Hint) bits
	//
	// ASA/SRM (Alpha AXP Architecture Reference Manual / Alpha System Architecture)
	// - PTE low-bits: GH is PTE<6:5>, a 2-bit field.
	// - GH encodes the "TB block" size: 8**GH base pages in a block.
	// - If GH is inconsistent across the block, behavior is UNPREDICTABLE.
	//
	// Source ref: ASA/SRM, Memory Management, "Page Table Entry (PTE) format",
	// and "Granularity Hint (GH) / TB block hint" description.
	// ---------------------------------------------------------------------------
	static constexpr unsigned PTE_BIT_GH0 = 5; // GH[0] at PTE<5>
	static constexpr unsigned PTE_BIT_GH1 = 6; // GH[1] at PTE<6>

	// Mask for the GH field at its architectural position.
	static constexpr quint64 PTE_GH_MASK = 0x60ULL; // bits 6:5

	// Extract GH from a raw PTE value (2 bits).
	static inline quint8 pteGH(PTEType raw) noexcept
	{
		// GH is PTE<6:5>
		return static_cast<quint8>((raw >> PTE_BIT_GH0) & 0x3U);
	}

	// Set GH inside a raw PTE value (2 bits).
	static inline void setPteGH(PTEType& raw, quint8 gh) noexcept
	{
		// Keep only 2 bits; store into PTE<6:5>
		raw = (raw & ~PTE_GH_MASK) | ((static_cast<quint64>(gh) & 0x3ULL) << PTE_BIT_GH0);
	}

	AXP_FLATTEN bool hasGH() const noexcept
	{
		// GH is 2 bits; treat nonzero GH as "has block hint"
		return pteGH(raw) != 0;
	}

	AXP_HOT AXP_FLATTEN quint8 gh() const noexcept
	{
		return pteGH(raw);
	}

	// Optional convenience setter on the object
	inline void setGH(quint8 gh) noexcept
	{
		setPteGH(raw, gh);
	}



	// OS specific software bits (example mapping)
	static constexpr unsigned PTE_BIT_PAGE_DIRTY = 14; // OS defined
	static constexpr unsigned PTE_BIT_PAGE_ACCESSED = 15; // OS defined

	static constexpr quint64 PERM_MASK_BITS =
		(1ULL << PTE_BIT_KRE)
		| (1ULL << PTE_BIT_ERE)
		| (1ULL << PTE_BIT_SRE)
		| (1ULL << PTE_BIT_URE)
		| (1ULL << PTE_BIT_KWE)
		| (1ULL << PTE_BIT_EWE)
		| (1ULL << PTE_BIT_SWE)
		| (1ULL << PTE_BIT_UWE)
		| (1ULL << PTE_BIT_FOR)
		| (1ULL << PTE_BIT_FOW)	;

	inline void clear() { raw = 0; }
	//constexpr AlphaPTE() noexcept : raw(0) {}
	constexpr AlphaPTE() noexcept : raw(0), cow(false) {}
	constexpr AlphaPTE(quint64 value) noexcept : raw(value), cow(false) {}


	// ---------------------------------------------------------------------
	// Static factory methods
	// ---------------------------------------------------------------------

	/// \brief Create AlphaPTE from raw 64-bit value
	/// \param rawValue The raw PTE value from memory or IPR
	/// \return AlphaPTE instance with the raw value
	static constexpr AlphaPTE fromRaw(quint64 rawValue) noexcept {
		return AlphaPTE(rawValue);
	}

	/// \brief Create an invalid (zero) PTE
	/// \return AlphaPTE instance with all bits cleared
	static constexpr AlphaPTE makeInvalid() noexcept {
		return AlphaPTE(0);
	}

	/// \brief Create a valid PTE from PFN and permission bits
	/// \param pfn Physical frame number
	/// \param kre Kernel read enable
	/// \param kwe Kernel write enable  
	/// \param ure User read enable
	/// \param uwe User write enable
	/// \param asmFlag Address space match (global) flag
	/// \return AlphaPTE instance with specified values
	static inline AlphaPTE makeValid(
		PFNType pfn,
		bool kre = true,
		bool kwe = false,
		bool ure = false,
		bool uwe = false,
		bool asmFlag = false
	) noexcept {
		AlphaPTE pte(0);
		pte.setPfn(pfn);
		pte.setValid(true);
		pte.setAsm(asmFlag);
		pte.setReadPermissions(kre, ure);
		pte.setWritePermissions(kwe, uwe);
		return pte;
	}
	// \brief Get the raw 64-bit representation
	/// \return Raw PTE value
	AXP_HOT AXP_FLATTEN quint64 toRaw() const noexcept {
		return raw;
	}



	// Fundamental bitwise helpers
	AXP_HOT AXP_FLATTEN bool valid() const noexcept {
		return extract<PTE_BIT_V, 1>() != 0;
	}

	template<unsigned Start, unsigned Len>
	AXP_HOT AXP_FLATTEN quint64 extract() const noexcept {
		static_assert(Start + Len <= 64, "Bit range exceeds 64-bit width");
		constexpr quint64 mask = (Len == 64) ? ~0ULL : ((1ULL << Len) - 1ULL);
		return (raw >> Start) & mask;
	}

	template<unsigned Start, unsigned Len>
	constexpr void insert(quint64 value) noexcept {
		static_assert(Start + Len <= 64, "Bit range exceeds 64-bit width");
		constexpr quint64 maskCore = (Len == 64) ? ~0ULL : ((1ULL << Len) - 1ULL);
		const quint64 mask = (maskCore << Start);
		raw = (raw & ~mask) | ((value << Start) & mask);
	}

	inline bool getASM() const noexcept { return asmBit(); }
	inline bool isAsm()  const noexcept { return asmBit(); }
	inline bool isGlobal() const noexcept
	{
		// ASM = 1 => matches all ASNs (global mapping)
		return asmBit();
	}
	inline bool matchesAllASNs() const noexcept
	{
		return asmBit();
	}




	inline void setPFN(PFNType pfnValue) noexcept
	{
		// EV6 PFN uses bits 59..32 (28 bits)
		constexpr quint64 PFN_MASK_inl = ((1ULL << 28) - 1) << 32;
		constexpr quint64 PFN_CLEAR = ~PFN_MASK_inl;

		// clear existing PFN
		raw &= PFN_CLEAR;

		// insert new PFN
		raw |= (static_cast<PFNType>(pfnValue & ((1ULL << 28) - 1)) << 32);
	}


	inline void setPermMask(AlphaN_S::PermMask perm) noexcept
	{
		raw = perm;
	}






	// ---------------------------------------------------------------------
	// Single-bit accessors for architectural fields
	// ---------------------------------------------------------------------


	AXP_HOT AXP_FLATTEN bool bitV()  const noexcept { return extract<AlphaN_S::PTE_BIT_V, 1>() != 0; }
	//[[nodiscard]] constexpr bool bitFOE() const noexcept { return extract<PTE_BIT_FOE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitFOW() const noexcept { return extract<AlphaN_S::PTE_BIT_FOW, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitFOR() const noexcept { return extract<AlphaN_S::PTE_BIT_FOR, 1>() != 0; }
    AXP_HOT AXP_FLATTEN bool bitFOE() const noexcept { return extract<AlphaN_S::PTE_BIT_FOE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitASM() const noexcept { return extract<AlphaN_S::PTE_BIT_ASM, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitUWE() const noexcept { return extract<AlphaN_S::PTE_BIT_UWE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitKWE() const noexcept { return extract<AlphaN_S::PTE_BIT_KWE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitURE() const noexcept { return extract<AlphaN_S::PTE_BIT_URE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitKRE() const noexcept { return extract<AlphaN_S::PTE_BIT_KRE, 1>() != 0; }

	AXP_HOT AXP_FLATTEN bool bitSWE() const noexcept { return extract<AlphaN_S::PTE_BIT_SWE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitEWE() const noexcept { return extract<AlphaN_S::PTE_BIT_EWE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitSRE() const noexcept { return extract<AlphaN_S::PTE_BIT_SRE, 1>() != 0; }
	AXP_HOT AXP_FLATTEN bool bitERE() const noexcept { return extract<AlphaN_S::PTE_BIT_ERE, 1>() != 0; }

	AXP_FLATTEN  bool testBit(unsigned bit) const noexcept
	{
		return (raw >> bit) & 1ULL;
	}

	// ---------------------------------------------------------------------
	// PFN helper (assumes PFN is in bits 32..63 in this layout)
	// ---------------------------------------------------------------------
	AXP_HOT AXP_FLATTEN PFNType pfn() const noexcept {
		return static_cast<PFNType>(extract<32, 28>());
	}

	AXP_HOT AXP_FLATTEN  void setPfn(PFNType pfnValue) noexcept {
		insert<32, 28>(static_cast<PFNType>(pfnValue));
	}

	// ---------------------------------------------------------------------
	// Valid, ASM helpers
	// ---------------------------------------------------------------------

	// Setter for V bit (Valid)
	AXP_HOT AXP_FLATTEN  void setValid(bool v) noexcept
    {
        if (v)
            raw |= PTE_MASK_V;
        else
            raw &= ~PTE_MASK_V;
    }

	// Getter for ASM bit. Using name asmBit() to avoid confusion with
	// inline assembly keyword on some compilers. If you need an "asm()"
	// alias for compatibility, you can add it.
	AXP_HOT AXP_FLATTEN bool asmBit() const noexcept {
		return bitASM();
	}

	// Optional alias if existing code calls pte.asm()
	AXP_HOT AXP_FLATTEN bool getAsm() const noexcept {
		return bitASM();
	}

	// Setter for ASM bit
	AXP_HOT AXP_FLATTEN  void setAsm(bool asmFlag)  noexcept {
		insert<PTE_BIT_ASM, 1>(asmFlag ? 1ULL : 0ULL);
	}

	// ---------------------------------------------------------------------
	// Packed "protection" byte for your emulator
	//
	// Bit layout in returned byte:
	//   bit 0 : KRE
	//   bit 1 : KWE
	//   bit 2 : URE
	//   bit 3 : UWE
	// Higher bits are currently unused.
	// ---------------------------------------------------------------------
	AXP_HOT AXP_FLATTEN quint8 protection8() const noexcept {
		quint8 prot = 0;

		if (bitKRE()) prot |= (1u << 0);
		if (bitKWE()) prot |= (1u << 1);
		if (bitURE()) prot |= (1u << 2);
		if (bitUWE()) prot |= (1u << 3);

		return prot;
	}

	// Convenience: full low-16-bit architectural/OS protection field
	AXP_HOT AXP_FLATTEN quint16 lowProtField() const noexcept {
		return static_cast<quint16>(extract<0, 16>());
	}

	// ---------------------------------------------------------------------
	// Canonical read / write permission setters
	//
	// Canonical form only tracks Kernel and User. EV6 specific E/S/E
	// modes will be compressed into these in PTETraits.
	// ---------------------------------------------------------------------

	// Two-argument canonical form: K, U
	AXP_HOT AXP_FLATTEN  void setReadPermissions(bool kre, bool ure) noexcept {
		insert<PTE_BIT_KRE, 1>(kre ? 1ULL : 0ULL);
		insert<PTE_BIT_URE, 1>(ure ? 1ULL : 0ULL);
	}

	// Four-argument adapter form: K, E, S, U
	// E and S are compressed into the canonical K bit.
	AXP_HOT AXP_FLATTEN  void setReadPermissions(bool kre, bool ere, bool sre, bool ure) noexcept {
		const bool kreCanonical = kre || ere || sre;
		setReadPermissions(kreCanonical, ure);
	}

	AXP_HOT AXP_FLATTEN  void setWritePermissions(bool kwe, bool uwe) noexcept {
		insert<PTE_BIT_KWE, 1>(kwe ? 1ULL : 0ULL);
		insert<PTE_BIT_UWE, 1>(uwe ? 1ULL : 0ULL);
	}

	// Four-argument adapter form: K, E, S, U
	// E and S are compressed into the canonical K bit.
	AXP_HOT AXP_FLATTEN  void setWritePermissions(bool kwe, bool ewe, bool swe, bool uwe) noexcept {
		const bool kweCanonical = kwe || ewe || swe;
		setWritePermissions(kweCanonical, uwe);
	}

	// ---------------------------------------------------------------------
	// Canonical read / write permission getters
	//
	// These provide the four-argument form expected by EV6 traits,
	// but only K and U are actually stored. E and S are reported as
	// false in canonical form.
	// ---------------------------------------------------------------------
	AXP_HOT AXP_FLATTEN void getReadPermissions(bool& kre, bool& ere, bool& sre, bool& ure) const noexcept {
		kre = bitKRE();
		ere = false;
		sre = false;
		ure = bitURE();
	}

	AXP_HOT AXP_FLATTEN  void getWritePermissions(bool& kwe, bool& ewe, bool& swe, bool& uwe) const noexcept {
		kwe = bitKWE();
		ewe = false;
		swe = false;
		uwe = bitUWE();
	}

	// ---------------------------------------------------------------------
	// Fault-on-access helpers
	// ---------------------------------------------------------------------

	AXP_HOT AXP_FLATTEN bool faultOnWrite() const noexcept {
		return bitFOW();
	}

	AXP_HOT AXP_FLATTEN bool faultOnRead() const noexcept {
		return bitFOR();
	}

    AXP_HOT AXP_FLATTEN bool faultOnExec() const noexcept {
        return bitFOE();
    }

	/**
 * @brief Check if PTE allows read access for given mode.
 *
 * @param mode Privilege mode (0=Kernel, 1=Executive, 2=Supervisor, 3=User)
 * @return true if read allowed, false otherwise
 */
	AXP_HOT AXP_FLATTEN bool canRead(Mode_Privilege mode) const noexcept
	{
		// Check fault-on-read flag first
		if (bitFOR()) {
			return false;
		}

		// Check mode-specific read enable bits
		switch (mode)
		{
		case Mode_Privilege::Kernel:
			return bitKRE();
		case Mode_Privilege::Executive:
			return bitERE();
		case Mode_Privilege::Supervisor:
			return bitSRE();
		case Mode_Privilege::User:
			return bitURE();
		default:
			return false;
		}
	}

	/**
	 * @brief Check if PTE allows write access for given mode.
	 *
	 * @param mode Privilege mode (0=Kernel, 1=Executive, 2=Supervisor, 3=User)
	 * @return true if write allowed, false otherwise
	 */
	AXP_HOT AXP_FLATTEN bool canWrite(Mode_Privilege mode) const noexcept
	{
		// Check fault-on-write flag first
		if (bitFOW()) {
			return false;
		}

		// Check mode-specific write enable bits
		switch (mode)
		{
		case Mode_Privilege::Kernel:
			return bitKWE();
		case Mode_Privilege::Executive:
			return bitEWE();
		case Mode_Privilege::Supervisor:
			return bitSWE();
		case Mode_Privilege::User:
			return bitUWE();
		default:
			return false;
		}
	}

	/**
	 * @brief Check if PTE allows execute access.
	 *
	 * Execute permission is implicit if read is allowed and FOE is not set.
	 *
	 * @param mode Privilege mode
	 * @return true if execute allowed, false otherwise
	 */
	AXP_HOT AXP_FLATTEN bool canExecute(Mode_Privilege mode) const noexcept
	{
		// Check fault-on-execute flag first
		if (bitFOE()) {
			return false;
		}

		// Execute requires read permission
		return canRead(mode);
	}

	/**
	 * @brief Check if PTE is valid.
	 *
	 * Alias for bitV() - more semantic name.
	 */
	AXP_HOT AXP_FLATTEN bool isValid() const noexcept
	{
		return bitV();
	}

	/**
	 * @brief Simplified read check (kernel-only).
	 *
	 * For compatibility with old code that doesn't pass mode.
	 * Assumes kernel mode if no mode specified.
	 */
	AXP_HOT AXP_FLATTEN bool canRead() const noexcept
	{
		return canRead(Mode_Privilege::Kernel);
	}

	/**
	 * @brief Simplified write check (kernel-only).
	 *
	 * For compatibility with old code that doesn't pass mode.
	 * Assumes kernel mode if no mode specified.
	 */
	AXP_HOT AXP_FLATTEN bool canWrite() const noexcept
	{
		return canWrite(Mode_Privilege::Kernel);
	}


};


#endif
