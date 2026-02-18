#ifndef EBOXLIB_GLOBAL_H
#define EBOXLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(EBOXLIB_LIBRARY)
#define EBOXLIB_EXPORT Q_DECL_EXPORT
#else
#define EBOXLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // EBOXLIB_GLOBAL_H
