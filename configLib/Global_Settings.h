#ifndef _EMULATRAPPUNI_CONFIGLIB_GLOBAL_SETTINGS_H
#define _EMULATRAPPUNI_CONFIGLIB_GLOBAL_SETTINGS_H

#pragma once
#include <QGlobalStatic>

#include "EmulatorSettingsInline.h"

Q_GLOBAL_STATIC(EmulatorSettingsInline, g_emuSettings)

inline EmulatorSettingsInline& Global_EmulatorSettings()
{
	return *g_emuSettings();
}

#endif
