#pragma once
#include <QtGlobal>
#include "CPUStateIPRInterface.h"


// Barrier VTable (memory ordering semantics)
struct BarrierVTable {
	void (*mb)(CPUStateIPRInterface*);     // Full memory barrier
	void (*wmb)(CPUStateIPRInterface*);    // Write memory barrier
	void (*rmb)(CPUStateIPRInterface*);    // Read memory barrier
	void (*scPublish)(CPUStateIPRInterface*);      // Store-Conditional publish
	void (*clearReservation)(CPUStateIPRInterface*);
};