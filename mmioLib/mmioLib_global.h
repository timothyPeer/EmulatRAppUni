#ifndef MMIOLIB_GLOBAL_H
#define MMIOLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MMIOLIB_LIBRARY)
#define MMIOLIB_EXPORT Q_DECL_EXPORT
#else
#define MMIOLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // MMIOLIB_GLOBAL_H
