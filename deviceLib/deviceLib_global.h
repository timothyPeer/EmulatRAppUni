#ifndef DEVICELIB_GLOBAL_H
#define DEVICELIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DEVICELIB_LIBRARY)
#define DEVICELIB_EXPORT Q_DECL_EXPORT
#else
#define DEVICELIB_EXPORT Q_DECL_IMPORT
#endif

#endif // DEVICELIB_GLOBAL_H
