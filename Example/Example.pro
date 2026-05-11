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
           mainwindow.cpp \
           tabshowcasewidget.cpp \
           font-icon/fonticon.cpp \
           aboutprojectwidget.cpp \
           segoeicongallerywidget.cpp
HEADERS += mainwindow.h \
           tabshowcasewidget.h \
           font-icon/fonticon.h \
           aboutprojectwidget.h \
           segoeicongallerywidget.h
FORMS   += mainwindow.ui
RESOURCES += resources.qrc \
             font-icon/resource.qrc
RC_ICONS = appicon.ico

DESTDIR = $$DESTDIR_BIN

