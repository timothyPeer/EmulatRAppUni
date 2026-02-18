#ifndef GLOBALCPUCOUNT_H
#define GLOBALCPUCOUNT_H


// ===============================================================
// Global CPU Count Provider (Meyers Singleton)
// ===============================================================
#include "types_core.h"
class GlobalCPUCount final
{
public:
    static inline void initialize(int count = MAX_CPUS) noexcept {
        cpuCount_ = count;
    }

    static inline int get() noexcept {
        return cpuCount_;
    }

private:
    inline static int cpuCount_ = 1; // safe default, updated at startup
};

#endif // GLOBALCPUCOUNT_H
