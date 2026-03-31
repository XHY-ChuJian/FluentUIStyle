QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

win32-msvc {
    QMAKE_CXXFLAGS += /utf-8
}

RC_ICONS = appicon.ico

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

INCLUDEPATH += $$PWD/fluentui3style
include($$PWD/fluentui3style/fluentui3style.pri)

INCLUDEPATH += $$PWD/customcontrol/SlidingStackedWidget
include($$PWD/customcontrol/SlidingStackedWidget/SlidingStackedWidget.pri)





