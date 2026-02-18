#pragma once
#include "CPUStateIPRInterface.h"
#include <QtGlobal>


/*
	The PS is changed when an interrupt or exception is
	initiated and by the rti, retsys, and swpipl instructions.
	PS<mode>	PS<IPL> Mode Use
	1			0		User User software
	0			0		Kernel System software
	0			1		Kernel System software
	0			2		Kernel System software
	0			3		Kernel Low priority device interrupts
	0			4		Kernel High priority device interrupts
	0			5		Kernel Clock, and interprocessor interrupts
	0			6		Kernel Real-time devices
	0			6		Kernel Correctable error reporting
	0			7		Kernel Machine checks
*/
inline void executePALSwpipl_iface(CPUStateIPRInterface* cpuState) {

}
