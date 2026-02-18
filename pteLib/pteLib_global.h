#ifndef PTELIB_GLOBAL_H
#define PTELIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PTELIB_LIBRARY)
#define PTELIB_EXPORT Q_DECL_EXPORT
#else
#define PTELIB_EXPORT Q_DECL_IMPORT
#endif

#endif // PTELIB_GLOBAL_H
