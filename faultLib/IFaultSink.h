// IFaultSink.h  
// TODO (transitional interface - to deprecate later)
#ifndef IFAULTSINK_H
#define IFAULTSINK_H

#include "../coreLib/types_core.h"

// Forward declarations
struct PendingEvent;

class IFaultSink {
public:
	virtual ~IFaultSink() = default;

	// Core interface
	virtual void raiseFault(const PendingEvent& ev) noexcept = 0;
	virtual bool hasPendingFault() const noexcept = 0;
	virtual bool eventPending() const noexcept = 0;
	virtual void clearPendingEvents() noexcept = 0;

	// Optional extensions (can be removed if not used)
	virtual void flushPendingTraps() noexcept {}
	virtual PendingEvent getPendingEvents() const noexcept = 0;
};

#endif // IFAULTSINK_H
