#ifndef _EMULATRAPPUNI_FAULTLIB_BUILDARITHMETICTRAP_INL_H
#define _EMULATRAPPUNI_FAULTLIB_BUILDARITHMETICTRAP_INL_H

#pragma once
#include <QtGlobal>
#include "fault_core.h"

// Build arithmetic trap kind from multiple exceptions
inline ArithmeticTrapKind buildArithmeticTrap(bool inv, bool dze, bool ovf, bool unf, bool ine, bool iov = false) {
	quint16 result = 0;
	if (inv) result |= static_cast<quint16>(ArithmeticTrapKind::INVALID_OPERATION);
	if (dze) result |= static_cast<quint16>(ArithmeticTrapKind::DIVIDE_BY_ZERO);
	if (ovf) result |= static_cast<quint16>(ArithmeticTrapKind::OVER_FLOW);
	if (unf) result |= static_cast<quint16>(ArithmeticTrapKind::UNDER_FLOW);
	if (ine) result |= static_cast<quint16>(ArithmeticTrapKind::INEXACT);
	if (iov) result |= static_cast<quint16>(ArithmeticTrapKind::INTEGER_OVERFLOW);
	return static_cast<ArithmeticTrapKind>(result);
}
#endif
