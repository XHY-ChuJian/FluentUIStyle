#pragma once

#include <QtCore/qobjectdefs.h>

// Declares a Qt property together with its inline getter, setter declaration,
// change signal and private m_<property> storage. Setter implementations remain
// in the source file.
#define EXWIDGETS_DECLARE_PROPERTY( Type, Property, Getter, Setter, DefaultValue ) \
    Q_PROPERTY( Type Property READ Getter WRITE Setter NOTIFY Property##Changed ) \
    [[nodiscard]] Type Getter() const { return m_##Property; } \
    void Setter( Type value ); \
    Q_SIGNAL void Property##Changed( Type value ); \
private: \
    Type m_##Property = DefaultValue; \
public:
