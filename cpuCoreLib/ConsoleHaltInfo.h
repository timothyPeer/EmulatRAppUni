#ifndef CONSOLEHALTINFO_H
#define CONSOLEHALTINFO_H

#include "types_core.h"
#include <QtGlobal>

struct ConsoleHaltInfo
{
    CPUIdType cpuId = 0;
    quint64 haltPC = 0;
    quint64 haltPS = 0;
    quint8  haltCode = 0;   // 0 = HALT instruction, others = RESET/MCHK/etc.
};
#endif // CONSOLEHALTINFO_H
