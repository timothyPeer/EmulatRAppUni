#ifndef IBOXLIB_GLOBAL_H
#define IBOXLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(IBOXLIB_LIBRARY)
#define IBOXLIB_EXPORT Q_DECL_EXPORT
#else
#define IBOXLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // IBOXLIB_GLOBAL_H
