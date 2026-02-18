#ifndef TRACE_CORE_INL_H
#define TRACE_CORE_INL_H

#include "Axp_Attributes_core.h"
#include <QtGlobal>
#include <QString>

AXP_FLATTEN void axp_trace_side_effect(bool condition, const QString& message) noexcept
{
#if AXP_TRACE_ENABLED
    if (condition)
        axp_trace_real(message);
#else
    Q_UNUSED(condition)
    Q_UNUSED(message)
#endif
}

#endif // TRACE_CORE_INL_H
