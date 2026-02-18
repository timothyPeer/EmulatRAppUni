#ifndef PALBOXLIB_GLOBAL_H
#define PALBOXLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PALBOXLIB_LIBRARY)
#  define PALBOXLIB_EXPORT Q_DECL_EXPORT
#else
#  define PALBOXLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // PALBOXLIB_GLOBAL_H
