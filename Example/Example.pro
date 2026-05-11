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
           applanguage.cpp \
           mainwindow.cpp \
           tabshowcasewidget.cpp \
           font-icon/fonticon.cpp \
           aboutprojectwidget.cpp \
           segoeicongallerywidget.cpp
HEADERS += mainwindow.h \
           applanguage.h \
           tabshowcasewidget.h \
           font-icon/fonticon.h \
           aboutprojectwidget.h \
           segoeicongallerywidget.h
FORMS   += mainwindow.ui

# Translations: edit tools/fill_en_ts.py then:
#   lupdate Example.pro && python tools/fill_en_ts.py && lrelease translations/Example_en_US.ts -qm translations/Example_en_US.qm
# Optional targets (Qt Creator / qmake): `example-update-translations` if you add a custom run step, or run lupdate manually.
TRANSLATIONS += translations/Example_en_US.ts

win32 {
    LRELEASE = $$clean_path($$[QT_INSTALL_BINS]/lrelease.exe)
} else {
    LRELEASE = $$clean_path($$[QT_INSTALL_BINS]/lrelease)
}

# Build .qm before link. PRE_TARGETDEPS must list the *output file* (or a real path), not the extra-target name;
# otherwise MSVC nmake reports: dependent 'example_qm' does not exist.
example_qm.target = $$shell_path($$PWD/translations/Example_en_US.qm)
example_qm.commands = $$quote($$LRELEASE) $$quote($$PWD/translations/Example_en_US.ts) -qm $$quote($$PWD/translations/Example_en_US.qm)
example_qm.depends = $$PWD/translations/Example_en_US.ts
QMAKE_EXTRA_TARGETS += example_qm
PRE_TARGETDEPS += $$example_qm.target

RESOURCES += resources.qrc \
             font-icon/resource.qrc \
             translations/i18n_embed.qrc
RC_ICONS = appicon.ico

DESTDIR = $$DESTDIR_BIN

