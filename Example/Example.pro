include(../common.pri)

QT += gui-private core-private widgets-private
TEMPLATE = app
TARGET = Example

INCLUDEPATH += $$PWD/../ExWidgets
LIBS += -L$$DESTDIR_LIB -lExWidgets

INCLUDEPATH += $$PWD/../FluentUI3Style
# LIBS += -L$$DESTDIR_LIB -lFluentUI3Style
# include($$PWD/../FluentUI3Style/FluentUI3Style.pri)

DEFINES += FLUENT_USE_QT_STYLE

SOURCES += main.cpp \
           mainwindow.cpp
HEADERS += mainwindow.h
FORMS   += mainwindow.ui
RESOURCES += resources.qrc
RC_ICONS = appicon.ico

win32-msvc {
    QMAKE_CXXFLAGS += /utf-8
}

DESTDIR = $$DESTDIR_BIN

