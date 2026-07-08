include(../common.pri)

TEMPLATE = lib
CONFIG += shared
TARGET = FluentUI3Style

CONFIG += use_qt_style
use_qt_style {
    DEFINES += FLUENT_USE_QT_STYLE
}

include(fluentui3style.pri)

DESTDIR = $$DESTDIR_LIB
DLLDESTDIR = $$DESTDIR_BIN

target.path = $$PREFIX/lib
INSTALLS += target

headers.files = $$HEADERS
headers.path = $$PREFIX/include/fluentui3style
INSTALLS += headers
