#pragma once
#include <QtGlobal>

/// \brief Simulate pipeline-side preparation for a virtual address access. 
/// \param va The virtual address for which to adjust the pipeline.
/// \details
/// - Should enqueue a prefetch or instruction/data access event in the pipeline model.
/// - No effect for functional simulation.
/// - For detailed simulation, updates AlphaPipeline structures.
/// \see Alpha 21164/21264 Microarchitecture Guide
inline void adjustPipeline(VAType va) noexcept {
	// Enqueue prefetch...
	// PipelineSimulator::enqueuePrefetch(va);

	// Track potential hazard...  
	// PipelineSimulator::trackHazard(va);

	DEBUG_LOG(QString("PIPELINE_ADJUST VA: 0x%1").arg(0, 16, va));
}
