include(../../common.pri)

QT += gui-private core-private widgets-private svg
greaterThan(QT_MAJOR_VERSION, 5): QT += multimedia network

TEMPLATE = app
TARGET = Gallery
CONFIG(debug, debug|release): TARGET = $${TARGET}d
CONFIG += object_parallel_to_source no_batch

INCLUDEPATH += $$PWD/../../ExWidgets
LIBS += -L$$DESTDIR_LIB -lExWidgets

INCLUDEPATH += $$PWD/../../fluentui3style
INCLUDEPATH += $$PWD/../../FluentUI3Colors

win32 {
    DEFINES += FLUENT_USE_QT_STYLE GALLERY_ENABLE_I18N
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
           $$PWD/rangeslidershowcasewidget.cpp \
           $$PWD/liquidgaugeshowcasewidget.cpp \
           $$PWD/radialgaugeshowcasewidget.cpp \
           $$PWD/progressringshowcasewidget.cpp
HEADERS += $$PWD/mainwindow.h \
           $$PWD/installedsoftwaretablewidget.h \
           $$PWD/tabshowcasewidget.h \
           $$PWD/dialogshowcasewidget.h \
           $$PWD/font-icon/fonticon.h \
           $$PWD/aboutprojectwidget.h \
           $$PWD/segoeicongallerywidget.h \
           $$PWD/colorshowcasewidget.h \
           $$PWD/rangeslidershowcasewidget.h \
           $$PWD/liquidgaugeshowcasewidget.h \
           $$PWD/radialgaugeshowcasewidget.h \
           $$PWD/progressringshowcasewidget.h
FORMS   += $$PWD/mainwindow.ui

win32 {
    SOURCES += $$PWD/applanguage.cpp
    HEADERS += $$PWD/applanguage.h

    # Translations: edit tools/fill_en_ts.py then:
    #   lupdate Gallery.pro && python tools/fill_en_ts.py && lrelease translations/Gallery_en_US.ts -qm translations/Gallery_en_US.qm
    TRANSLATIONS += translations/Gallery_en_US.ts

    LRELEASE = $$clean_path($$[QT_INSTALL_BINS]/lrelease.exe)

    gallery_qm.target = $$shell_path($$PWD/translations/Gallery_en_US.qm)
    gallery_qm.commands = $$quote($$LRELEASE) $$quote($$PWD/translations/Gallery_en_US.ts) -qm $$quote($$PWD/translations/Gallery_en_US.qm)
    gallery_qm.depends = $$PWD/translations/Gallery_en_US.ts
    QMAKE_EXTRA_TARGETS += gallery_qm
    PRE_TARGETDEPS += $$gallery_qm.target

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
