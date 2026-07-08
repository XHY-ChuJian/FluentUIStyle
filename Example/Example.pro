include(../common.pri)

QT += gui-private core-private widgets-private svg
greaterThan(QT_MAJOR_VERSION, 5): QT += multimedia network

TEMPLATE = app
TARGET = Example
CONFIG(debug, debug|release): TARGET = $${TARGET}d
CONFIG += object_parallel_to_source no_batch

INCLUDEPATH += $$PWD/../ExWidgets
LIBS += -L$$DESTDIR_LIB -lExWidgets

INCLUDEPATH += $$PWD/../fluentui3style
INCLUDEPATH += $$PWD/../FluentUI3Colors

win32 {
    DEFINES += FLUENT_USE_QT_STYLE EXAMPLE_ENABLE_I18N
} else {
    LIBS += -L$$DESTDIR_LIB -lFluentUI3Style
    QMAKE_RPATHDIR += \$$ORIGIN/../lib
}

greaterThan(QT_MAJOR_VERSION, 5) {
    include($$PWD/../AudiomaticMini/AudiomaticMiniWidgets.pri)
} else {
    SOURCES += $$PWD/spectrumshowcasewidget.cpp \
               $$PWD/sinewavegenerator.cpp
    HEADERS += $$PWD/spectrumshowcasewidget.h \
               $$PWD/sinewavegenerator.h
}

SOURCES += $$PWD/main.cpp \
           $$PWD/mainwindow.cpp \
           $$PWD/installedsoftwaretablewidget.cpp \
           $$PWD/tabshowcasewidget.cpp \
           $$PWD/dialogshowcasewidget.cpp \
           $$PWD/font-icon/fonticon.cpp \
           $$PWD/aboutprojectwidget.cpp \
           $$PWD/segoeicongallerywidget.cpp \
           $$PWD/colorshowcasewidget.cpp \
           $$PWD/rangeslidershowcasewidget.cpp
HEADERS += $$PWD/mainwindow.h \
           $$PWD/installedsoftwaretablewidget.h \
           $$PWD/tabshowcasewidget.h \
           $$PWD/dialogshowcasewidget.h \
           $$PWD/font-icon/fonticon.h \
           $$PWD/aboutprojectwidget.h \
           $$PWD/segoeicongallerywidget.h \
           $$PWD/colorshowcasewidget.h \
           $$PWD/rangeslidershowcasewidget.h
FORMS   += $$PWD/mainwindow.ui

win32 {
    SOURCES += $$PWD/applanguage.cpp
    HEADERS += $$PWD/applanguage.h

    # Translations: edit tools/fill_en_ts.py then:
    #   lupdate Example.pro && python tools/fill_en_ts.py && lrelease translations/Example_en_US.ts -qm translations/Example_en_US.qm
    TRANSLATIONS += translations/Example_en_US.ts

    LRELEASE = $$clean_path($$[QT_INSTALL_BINS]/lrelease.exe)

    example_qm.target = $$shell_path($$PWD/translations/Example_en_US.qm)
    example_qm.commands = $$quote($$LRELEASE) $$quote($$PWD/translations/Example_en_US.ts) -qm $$quote($$PWD/translations/Example_en_US.qm)
    example_qm.depends = $$PWD/translations/Example_en_US.ts
    QMAKE_EXTRA_TARGETS += example_qm
    PRE_TARGETDEPS += $$example_qm.target

    RC_FILE = appicon.rc
}

RESOURCES += resources.qrc \
             font-icon/resource.qrc
win32 {
    RESOURCES += translations/i18n_embed.qrc
}

CONFIG(release, debug|release) {
    CONFIG -= console
}

DESTDIR = $$DESTDIR_BIN
