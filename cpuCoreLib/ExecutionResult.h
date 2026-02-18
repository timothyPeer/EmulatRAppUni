#ifndef EXECUTIONRESULT_H
#define EXECUTIONRESULT_H
#include <QtGlobal>
#include "../faultLib/IFaultSink.h"
// Unified ExecuteResult - matches your unified StageStatus pattern
struct ExecuteResult
{
    enum Status {
        Success,
        Trap,
        Stall,
        Fault
    } status;

    quint64 result;          // Integer/address result
    double fpResult;         // FP result (when applicable)
    IFaultSink fault;        // Fault info (when status == Fault/Trap)

    // Factory methods for each unit
    static ExecuteResult success(quint64 value) {
        return {Success, value, 0.0, {}};
    }
    static ExecuteResult fpSuccess(double value) {
        return {Success, 0, value, {}};
    }
    static ExecuteResult trap(FaultDescriptor f) {
        return {Trap, 0, 0.0, f};
    }
};
#endif // EXECUTIONRESULT_H
