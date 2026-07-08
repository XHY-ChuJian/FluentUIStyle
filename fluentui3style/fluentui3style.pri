QT += svg

DEFINES += FLUENTUI3STYLE_LIBRARY

HEADERS += \
    $$PWD/fluentui3style.h \
    $$PWD/fluentui3styleproperties.h \
    $$PWD/fluentuiappearance.h \
    $$PWD/palettemanager.h \
    $$PWD/qhexstring_p.h \
    $$PWD/qstyleanimation_p.h \
    $$PWD/qstylehelper_p.h \
    $$PWD/fluentui3style_global.h

SOURCES += \
    $$PWD/fluentui3style.cpp \
    $$PWD/fluentuiappearance.cpp \
    $$PWD/palettemanager.cpp \
    $$PWD/qstyleanimation.cpp \
    $$PWD/qstylehelper.cpp

RESOURCES += \
    $$PWD/resource.qrc

INCLUDEPATH += $$PWD/../FluentUI3Colors

win32 {
    LIBS += -ldwmapi
}
