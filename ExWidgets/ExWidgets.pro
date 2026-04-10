include(../common.pri)

DEFINES += EXWIDGETS_LIBRARY
TEMPLATE = lib
CONFIG += shared
TARGET = ExWidgets

include(ExWidgets.pri)

DESTDIR = $$DESTDIR_LIB
DLLDESTDIR = $$DESTDIR_BIN

target.path = $$PREFIX/lib
INSTALLS += target

headers.files = $$HEADERS
headers.path = $$PREFIX/include/ExWidgets
INSTALLS += headers
