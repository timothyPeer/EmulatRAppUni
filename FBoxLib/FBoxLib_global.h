#ifndef FBOXLIB_GLOBAL_H
#define FBOXLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FBOXLIB_LIBRARY)
#define FBOXLIB_EXPORT Q_DECL_EXPORT
#else
#define FBOXLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // FBOXLIB_GLOBAL_H
