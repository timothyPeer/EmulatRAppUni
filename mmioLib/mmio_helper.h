#pragma once
#include <QString>
#include "../CoreLib/mmio_core.h"



inline QString toString(mmio_AllocationResult r)
{
	switch (r) {
	case mmio_AllocationResult::SUCCESS:            return "SUCCESS";
	case mmio_AllocationResult::MMIO_EXHAUSTED:     return "MMIO_EXHAUSTED";
	case mmio_AllocationResult::IRQ_EXHAUSTED:      return "IRQ_EXHAUSTED";
	case mmio_AllocationResult::TEMPLATE_NOT_FOUND: return "TEMPLATE_NOT_FOUND";
	case mmio_AllocationResult::CRITICAL_FAILURE:   return "CRITICAL_FAILURE";
	case mmio_AllocationResult::FATAL_BOOT_ABORT:   return "FATAL_BOOT_ABORT";
	case mmio_AllocationResult::DEGRADED:           return "DEGRADED";
	}
	return "UNKNOWN";
}
inline mmio_AllocationResult allocationResultFromString(const QString& s)
{
	if (s == "SUCCESS")            return mmio_AllocationResult::SUCCESS;
	if (s == "MMIO_EXHAUSTED")     return mmio_AllocationResult::MMIO_EXHAUSTED;
	if (s == "IRQ_EXHAUSTED")      return mmio_AllocationResult::IRQ_EXHAUSTED;
	if (s == "TEMPLATE_NOT_FOUND") return mmio_AllocationResult::TEMPLATE_NOT_FOUND;
	if (s == "CRITICAL_FAILURE")   return mmio_AllocationResult::CRITICAL_FAILURE;
	if (s == "FATAL_BOOT_ABORT")   return mmio_AllocationResult::FATAL_BOOT_ABORT;
	if (s == "DEGRADED")           return mmio_AllocationResult::DEGRADED;
	// Optionally, handle unknowns:
	return mmio_AllocationResult::CRITICAL_FAILURE; // or some safe default or error
}

