#pragma once

#include <QtGlobal>

#if defined(EXWIDGETS_LIBRARY)
#  define EXWIDGETS_EXPORT Q_DECL_EXPORT
#else
#  define EXWIDGETS_EXPORT Q_DECL_IMPORT
#endif
