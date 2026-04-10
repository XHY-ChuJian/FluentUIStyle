#pragma once

#include <QtGlobal>

#if defined(FLUENTUI3STYLE_LIBRARY)
#  define FLUENTUI3STYLE_EXPORT Q_DECL_EXPORT
#else
#  define FLUENTUI3STYLE_EXPORT Q_DECL_IMPORT
#endif
