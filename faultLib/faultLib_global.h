#ifndef FAULTLIB_GLOBAL_H
#define FAULTLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FAULTLIB_LIBRARY)
#define FAULTLIB_EXPORT Q_DECL_EXPORT
#else
#define FAULTLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // FAULTLIB_GLOBAL_H
