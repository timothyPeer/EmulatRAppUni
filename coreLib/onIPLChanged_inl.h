#ifndef ONIPLCHANGED_INL_H
#define ONIPLCHANGED_INL_H

#include <QtGlobal>
#include "types_core.h"
#include "global_IRQController.h"

inline void onIPLChanged(CPUIdType cpuId, quint8 oldIPL, quint8 newIPL) noexcept{
	auto& irq = global_IRQController();
	irq.setCPUIpl(static_cast<qint32>(cpuId), newIPL);
}
#endif // ONIPLCHANGED_INL_H
