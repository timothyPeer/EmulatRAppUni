#ifndef CBOX_CORE_H
#define CBOX_CORE_H
#include <QtGlobal>

enum class SerializationType {
    Barrier_MB,		// MB
	Barrier_TRAP ,  // TRAPB
	Barrier_WRITE,  // WMB  
	Barrier_EXC,     // EXCB
	Barrier_NONE	// None
};

#endif // CBOX_CORE_H
