CONFIG += c++17
QT += core gui widgets svg

win32-msvc {
    # /utf-8: 源文件含 UTF-8（中文注释等）时避免 C4819，并减少预处理器误判
    QMAKE_CXXFLAGS += /wd4273 /utf-8
    QMAKE_CFLAGS += /utf-8
}

DESTDIR_BIN = $$OUT_PWD/../bin
DESTDIR_LIB = $$OUT_PWD/../lib

isEmpty(PREFIX) {
    PREFIX = $$PWD/install
}
