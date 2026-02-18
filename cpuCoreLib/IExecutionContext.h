#ifndef IEXECUTIONCONTEXT_H
#define IEXECUTIONCONTEXT_H
#include "types_core.h"
#include <QtGlobal>

class IExecutionContext {
public:
    virtual ~IExecutionContext() = default;

    virtual CPUIdType cpuId() const noexcept = 0;

    // Architectural state
    virtual IPRBank& ipr() noexcept = 0;
    virtual RegisterBank& regs() noexcept = 0;

    // Faults & traps
    virtual IFaultSink& faultSink() noexcept = 0;

    // Pipeline / control
    virtual void requestPipelineFlush(quint64 pc = 0) noexcept = 0;
};

#endif // IEXECUTIONCONTEXT_H
