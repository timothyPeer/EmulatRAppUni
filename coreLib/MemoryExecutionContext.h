#ifndef MEMORYEXECUTIONCONTEXT_H
#define MEMORYEXECUTIONCONTEXT_H

#include <QtGlobal>
#include "../pteLib/AlphaPTE_Core.h"

// forward Declares

struct PipelineSlot;
class AlphaProcessorContext;
struct AlphaPTE;

struct alignas(16) MemoryExecutionContext {
    // What you already pass everywhere:
    PipelineSlot& slot;
    AlphaProcessorContext* ctx;

    // What you already compute:
    quint64 va;        // Effective address
    quint64 pa;        // Physical address
    AlphaPTE pte;      // Translation result

    // What you already check:
    bool success;      // Did operation succeed?

    // Constructor for convenience
    MemoryExecutionContext(PipelineSlot& s, AlphaProcessorContext* c)
        : slot(s), ctx(c), va(0), pa(0), success(false) {}
};
#endif // MEMORYEXECUTIONCONTEXT_H
