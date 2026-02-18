#ifndef MMULIB_GLOBAL_H
#define MMULIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MMULIB_LIBRARY)
#define MMULIB_EXPORT Q_DECL_EXPORT
#else
#define MMULIB_EXPORT Q_DECL_IMPORT
#endif

#endif // MMULIB_GLOBAL_H
