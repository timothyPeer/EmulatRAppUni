#pragma once

#include "ISpamManager.h"
#include "InstructionDecoder.h"

class Ev6InstructionCache {
public:
	Ev6InstructionCache(int cpuCount)
		: m_ispam(cpuCount) {
	}

	// ========================================================================
	// Fetch and decode instruction
	// ========================================================================

	const InstructionGrain_iSpam* fetchInstruction(
		CPUIdType cpuId,
		quint64 pc,
		quint64 pa,  // From TLB translation
		quint8 asn
	) {
		// 1. Fast path: Check ISPAM cache
		if (auto* entry = m_ispam.lookup(cpuId, pc, pa, asn)) {
			return &entry->grain;
		}

		// 2. Slow path: Decode instruction
		const quint32 rawInstr = readPhysical(pa);
		InstructionGrain_iSpam grain = decodeInstruction(rawInstr);

		// 3. Insert into cache
		m_ispam.insert(cpuId, pc, pa, asn, grain);

		// 4. Return cached entry
		if (auto* entry = m_ispam.lookup(cpuId, pc, pa, asn)) {
			return &entry->grain;
		}

		return nullptr;  // Should never happen
	}

	// ========================================================================
	// Route grain to execution box
	// ========================================================================

	void dispatchToBox(const InstructionGrain_iSpam& grain) {
		switch (grain.targetBox) {
		case ExecutionBox::EBox:
			m_ebox.enqueue(grain);
			break;
		case ExecutionBox::FBox:
			m_fbox.enqueue(grain);
			break;
		case ExecutionBox::MBox:
			m_mbox.enqueue(grain);
			break;
		default:
			break;
		}
	}

	// ========================================================================
	// Invalidation on code modification
	// ========================================================================

	void onPageModified(CPUIdType cpuId, quint64 pa) {
		m_ispam.invalidateByPA(cpuId, pa);
	}

private:
	ISpamManager<> m_ispam;
	// Execution boxes (placeholder)
	EBox m_ebox;
	FBox m_fbox;
	MBox m_mbox;
};