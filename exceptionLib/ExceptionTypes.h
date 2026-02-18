#ifndef _EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONTYPES_H
#define _EMULATRAPPUNI_EXCEPTIONLIB_EXCEPTIONTYPES_H

#pragma once
#include <QtGlobal>

enum class ExceptionType : quint8 {

	UNKNOWN,
	FAULT,
	TRAP,
	ABORT,
	INTERRUPT
	
};
#endif
