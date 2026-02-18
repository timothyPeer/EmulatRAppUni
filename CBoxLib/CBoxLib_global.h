#ifndef CBOXLIB_GLOBAL_H
#define CBOXLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CBOXLIB_LIBRARY)
#define CBOXLIB_EXPORT Q_DECL_EXPORT
#else
#define CBOXLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // CBOXLIB_GLOBAL_H
