#ifndef CONFIGLIB_GLOBAL_H
#define CONFIGLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CONFIGLIB_LIBRARY)
#define CONFIGLIB_EXPORT Q_DECL_EXPORT
#else
#define CONFIGLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // CONFIGLIB_GLOBAL_H
