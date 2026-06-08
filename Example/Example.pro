include(../common.pri)

QT += gui-private core-private widgets-private
TEMPLATE = app
TARGET = Example

INCLUDEPATH += $$PWD/../ExWidgets
LIBS += -L$$DESTDIR_LIB -lExWidgets

INCLUDEPATH += $$PWD/../fluentui3style

win32 {
    DEFINES += FLUENT_USE_QT_STYLE EXAMPLE_ENABLE_I18N
} else {
    LIBS += -L$$DESTDIR_LIB -lFluentUI3Style
    QMAKE_RPATHDIR += \$$ORIGIN/../lib
}

SOURCES += main.cpp \
           mainwindow.cpp \
           tabshowcasewidget.cpp \
           dialogshowcasewidget.cpp \
           font-icon/fonticon.cpp \
           aboutprojectwidget.cpp \
           segoeicongallerywidget.cpp \
           colorshowcasewidget.cpp
HEADERS += mainwindow.h \
           tabshowcasewidget.h \
           dialogshowcasewidget.h \
           font-icon/fonticon.h \
           aboutprojectwidget.h \
           segoeicongallerywidget.h \
           colorshowcasewidget.h
FORMS   += mainwindow.ui

win32 {
    SOURCES += applanguage.cpp
    HEADERS += applanguage.h

    # Translations: edit tools/fill_en_ts.py then:
    #   lupdate Example.pro && python tools/fill_en_ts.py && lrelease translations/Example_en_US.ts -qm translations/Example_en_US.qm
    TRANSLATIONS += translations/Example_en_US.ts

    LRELEASE = $$clean_path($$[QT_INSTALL_BINS]/lrelease.exe)

    example_qm.target = $$shell_path($$PWD/translations/Example_en_US.qm)
    example_qm.commands = $$quote($$LRELEASE) $$quote($$PWD/translations/Example_en_US.ts) -qm $$quote($$PWD/translations/Example_en_US.qm)
    example_qm.depends = $$PWD/translations/Example_en_US.ts
    QMAKE_EXTRA_TARGETS += example_qm
    PRE_TARGETDEPS += $$example_qm.target
}

RESOURCES += resources.qrc \
             font-icon/resource.qrc
win32 {
    RESOURCES += translations/i18n_embed.qrc
    RC_ICONS = appicon.ico
}

DESTDIR = $$DESTDIR_BIN
