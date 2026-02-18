#ifndef PALLIB_GLOBAL_H
#define PALLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PALLIB_EV6_LIBRARY)
#define PALLIB_EXPORT Q_DECL_EXPORT
#else
#define PALLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // PALLIB_GLOBAL_H
