// ============================================================================
// ExecTrace_Instrumentation.cpp
// ============================================================================
// Implementation of Tier 1-4 instrumentation record methods.
// Append to your existing ExecTrace.cpp or include as separate compilation unit.
//
// Output format: one text line per event, pipe-delimited fields, greppable tags.
// All lines start with the event tag for easy filtering:
//   grep "^PAL_ENTER" trace_cpu0.txt
//   grep "^FAULT_"    trace_cpu0.txt
//   grep "^IPR_"      trace_cpu0.txt
// ============================================================================

#include "ExecTrace.h"
// Assumes access to ExecTrace::Impl with:
//   static QTextStream* getStream(quint16 cpuId);
//   static bool isActive(quint16 cpuId);
// Adapt to match your Impl layout.

// ============================================================================
// HELPER: TrapCode_Class to string
// ============================================================================



