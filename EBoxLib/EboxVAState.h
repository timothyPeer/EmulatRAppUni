#pragma once
#include <QtCore/QtGlobal>
#include "../coreLib/types_core.h"
#include "VA_core.h"
#include "../coreLib/globalIPR_hot_cold_new.h"
#include "../coreLib/IPRStorage_HotExt.h"

// -----------------------------------------------------------------
// Address space classification helpers
//
// IMPORTANT:
//   - The Alpha architecture (ASA, "Virtual Addressing" chapter)
//     defines the VA width (43-bit vs 48-bit) and VA_FORM layout,
//     but it does NOT hard-code which ranges are "kernel" vs "user".
//
//   - User/kernel segmentation is chosen by the operating system
//     (OpenVMS, Tru64, etc.). Therefore, any isKernelAddress() or
//     isUserAddress() helper must be based on the OS's chosen VA
//     map, not on pure architectural rules.
//
//   - The constants below are placeholders. You should set them
//     to the ranges that your target OS uses for user and kernel
//     space (for example, OpenVMS P0/P1 vs Sx/K space, or Tru64
//     region layout).
//
// Reference: Alpha AXP System Reference Manual, "Virtual Addressing"
//            and OS-specific memory management documentation.
// -----------------------------------------------------------------
// 
// 
// OS-specific region bounds for 43-bit VA mode.
// TODO: set these to match your OpenVMS/Tru64 layout.
// static constexpr quint64 USER_MIN_43 = 0x0000000000000000ULL;
// static constexpr quint64 USER_MAX_43 = 0x000003FFFFFFFFFFULL; // placeholder
// static constexpr quint64 KERNEL_MIN_43 = 0xFFFFFC0000000000ULL; // placeholder
// static constexpr quint64 KERNEL_MAX_43 = 0xFFFFFFFFFFFFFFFFULL;
// // 
// // OS-specific region bounds for 48-bit VA mode.
// // TODO: set these to match your OS's 48-bit layout (if used).
// static constexpr quint64 USER_MIN_48 = 0x0000000000000000ULL;
// static constexpr quint64 USER_MAX_48 = 0x0000FFFFFFFFFFFFULL; // placeholder
// static constexpr quint64 KERNEL_MIN_48 = 0xFFFF000000000000ULL; // placeholder
// static constexpr quint64 KERNEL_MAX_48 = 0xFFFFFFFFFFFFFFFFULL;
// 
// enum class AddressClass : quint8 {
// 	Unknown = 0,
// 	User = 1,
// 	Kernel = 2
// };

// Convenience: check against a default superpage size you choose.
// TODO: set DEFAULT_SUPERPAGE_SHIFT to the page size you treat as
//       "superpage" in your OS (for example, 64KB -> 16).
static constexpr quint8 DEFAULT_SUPERPAGE_SHIFT = 16; // placeholder

class EBoxVAState {

	CPUIdType m_cpuId;
	IPRStorage_Hot64* m_iprs;

public:
	// VA_CTL bit masks
	static constexpr quint64 VA_CTL_FORM_MASK = 0x4; // [2]
	static constexpr quint64 VA_CTL_VA_48_MASK = 0x2; // [1]
	static constexpr quint64 VA_CTL_BENDIAN_MASK = 0x1; // [0]

	// Fault types
	enum class FaultType {
		None,
		AccessControlViolation,
		TranslationNotValid,
		FaultOnWrite,
		FaultOnRead, 
		FaultOnExecute, 
		PageFault, 
		TNV,
	};

	// Construction/initialization
	EBoxVAState(CPUIdType cpuId_) : m_cpuId(cpuId_)
		, m_iprs(&globalIPRHot64(cpuId_)) {
	}

	// VA accessors
	void setVA(VAType v) {
		m_iprs->va = v;
	}
	quint64 getVA() const {
		return m_iprs->va;
	}

	// VA_CTL accessors
	void setVA_CTL(VAType c) {
		m_iprs->va_ctl = c & 0x7;
	}
	quint64 getVA_CTL() const {
		return m_iprs->va_ctl;
	}

	// VPTB accessors
	void setVPTB(quint64 b) {
		m_iprs->vptb = b & 0xFFFFFFFFFFF80000ULL;
	}
	quint64 getVPTB() const {
		return m_iprs->vptb;
	}

	// Helper: VA_48 mode
	bool is43BitVA() const {
		return !(m_iprs->va_ctl & VA_CTL_VA_48_MASK);
	}
	bool is48BitVA() const {
		return  (m_iprs->va_ctl & VA_CTL_VA_48_MASK);
	}
	bool isBitEndian() const {
		return  (m_iprs->va_ctl & VA_CTL_BENDIAN_MASK);
	}

	// Compute VA_FORM (per spec, handle all cases)
	inline quint64 extractOffset() const noexcept
	{
		// 8KB pages -> offset is lower 13 bits
		return m_iprs->va & 0x1FFFULL;
	}

	// Extract page number/offset helpers
	inline VPNType extractVPN() const noexcept
	{
		if (is48BitVA()) {
			// 48-bit mode: VPN = VA[47:13]
			return (m_iprs->va >> 13) & 0xFFFFFFFFFFFFULL;
		}
		else {
			// 43-bit mode (Alpha default): VPN = VA[42:13]
			return (m_iprs->va >> 13) & 0x7FFFFFULL;
		}
	}
	inline quint64 getVA_FORM() const noexcept
	{
		quint64 form = (m_iprs->vptb & 0xFFFFFFFFFFF80000ULL);  // VPTB upper bits

		if (is48BitVA()) {
			// 48-bit form encoding
			form |= ((m_iprs->va & 0x3F00000000000ULL) >> 6);
			form |= ((m_iprs->va & 0xFFFFE0000ULL) << 16);
			form |= (quint64(qint64(m_iprs->va) >> 58) << 38);
		}
		else {
			// 43-bit form encoding
			if (m_iprs->va_ctl & VA_CTL_FORM_MASK) {
				form |= (m_iprs->vptb & 0xC0000000ULL);
				form |= ((m_iprs->va & 0xFFFFE0000ULL) << 3);
			}
			else {
				form |= ((m_iprs->va & 0x20000000000ULL) >> 10);
				form |= ((m_iprs->va & 0xFFFFE0000ULL) << 3);
			}
		}

		// Add FORM bit and endianness flag
		if (m_iprs->va_ctl & VA_CTL_FORM_MASK)
			form |= (1ULL << 2);
		if (isBitEndian())
			form |= 0x1ULL;

		return form;
	}


	// Core classifier using OS-selected ranges.
	inline AddressClass classifyAddress() const noexcept
	{
		const VAType v = m_iprs->va;

		if (is48BitVA()) {
			if (v >= USER_MIN_48 && v <= USER_MAX_48)   return AddressClass::User;
			if (v >= KERNEL_MIN_48 && v <= KERNEL_MAX_48) return AddressClass::Kernel;
		}
		else {
			if (v >= USER_MIN_43 && v <= USER_MAX_43)   return AddressClass::User;
			if (v >= KERNEL_MIN_43 && v <= KERNEL_MAX_43) return AddressClass::Kernel;
		}

		return AddressClass::Unknown;
	}

	inline bool isUserAddress() const noexcept
	{
		return classifyAddress() == AddressClass::User;
	}

	inline bool isKernelAddress() const noexcept
	{
		return classifyAddress() == AddressClass::Kernel;
	}

	// -----------------------------------------------------------------
	// Superpage alignment helper
	//
	// Architecturally, "superpage" (large page) is indicated by the
	// TB/PTE information (translation granularity), not by the VA
	// alone. However, you can test whether a VA is suitably aligned
	// to be the *base* of a candidate superpage.
	//
	// For Alpha, the base page size is typically 8KB (2^13), and
	// larger pages are sized in powers of 2. For example:
	//   - 64KB superpage:  2^16 bytes  -> pageShift = 16
	//   - 512KB superpage: 2^19 bytes  -> pageShift = 19
	//
	// This helper only checks alignment. Whether the mapping is
	// actually a superpage is determined by the PTE and TB state.
	//
	// Reference: Alpha AXP System Reference Manual, "Translation
	//            Buffer" / "Page Size" discussion for large pages.
	// -----------------------------------------------------------------

	inline bool isSuperpageAligned(quint8 pageShift) const noexcept
	{
		// Guard against silly values.
		if (pageShift < 13 || pageShift >= 64) {
			return false;
		}
		const quint64 mask = (quint64(1) << pageShift) - 1ULL;
		return (m_iprs->va & mask) == 0ULL;
	}

	inline bool isSuperpageCandidate() const noexcept
	{
		return isSuperpageAligned(DEFAULT_SUPERPAGE_SHIFT);
	}
};