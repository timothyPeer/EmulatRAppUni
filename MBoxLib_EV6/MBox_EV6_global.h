#ifndef MBOX_EV6_GLOBAL_H
#define MBOX_EV6_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MBOX_EV6_LIBRARY)
#define MBOX_EV6_EXPORT Q_DECL_EXPORT
#else
#define MBOX_EV6_EXPORT Q_DECL_IMPORT
#endif

#endif // MBOX_EV6_GLOBAL_H
