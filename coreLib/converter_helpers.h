#pragma once
#include <QtGlobal>
#include <QString>
#include "mmio_core.h"

QString toString(mmio_AllocationResult allocationResult)
{

	switch (allocationResult) 
	{
	case mmio_AllocationResult::SUCCESS:
		return "Success";

	case mmio_AllocationResult::MMIO_EXHAUSTED:
		return "MMIO_EXHAUSTED";

	case mmio_AllocationResult::IRQ_EXHAUSTED:
		return "IRQ_EXHAUSTED";

	case mmio_AllocationResult::TEMPLATE_NOT_FOUND:
		return "TEMPLATE_NOT_FOUND";

	case mmio_AllocationResult::CRITICAL_FAILURE:
		return "CRITICAL_FAILURE";

	case mmio_AllocationResult::FATAL_BOOT_ABORT:
		return "FATAK_BOOT_ABORT";

	case mmio_AllocationResult::DMA_NOT_SUPPORTED:
		return "DMA_NOT_SUPPORTED";

	case mmio_AllocationResult::DEGRADED:
		return "DEGRADED";
	}
	
	return "MMIO_Allocation NOT KNOWN";
}