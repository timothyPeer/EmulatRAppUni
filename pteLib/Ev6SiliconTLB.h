#ifndef Ev6SiliconTLB_h__
#define Ev6SiliconTLB_h__

#include <QtCore/QtGlobal>

// This header uses Ev6SPAMShardManager as a by-value member.
// Therefore we must include the header that defines Ev6SPAMShardManager.
#include "Ev6SiliconTypes.h"

// ============================================================================
// Ev6SiliconTLB - Silicon-level TLB container
// ============================================================================

class Ev6SiliconTLB
{
public:
	explicit Ev6SiliconTLB(int cpuCount)
		: m_spam(cpuCount) // Requires Ev6SPAMShardManager ctor(int)
	{
		// Silicon-level init
	}

	AXP_HOT inline Ev6SPAMShardManager& spam() noexcept { return m_spam; }
	AXP_HOT inline const Ev6SPAMShardManager& spam() const noexcept { return m_spam; }

private:
	Ev6SPAMShardManager m_spam;
};

#endif // Ev6SiliconTLB_h__

