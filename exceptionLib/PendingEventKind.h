#ifndef PENDINGEVENTKIND_H
#define PENDINGEVENTKIND_H
#include <QtGlobal>

enum class PendingEventKind : quint8
{
    None = 0,
    Exception,      // Synchronous fault/trap
    Interrupt,      // Asynchronous interrupt
    Ast,            // Asynchronous System Trap
    MachineCheck,   // Machine check
    Reset,           // Reset/wakeup
    PalCall			// CALL_PAL Eventkind
};
#endif // PENDINGEVENTKIND_H
