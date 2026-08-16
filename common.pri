CONFIG += c++17
QT += core gui widgets svg

win32-msvc {
    # /utf-8: 源文件含 UTF-8（中文注释等）时避免 C4819，并减少预处理器误判
    QMAKE_CXXFLAGS += /wd4273 /utf-8
    QMAKE_CFLAGS += /utf-8

    # Release 保持优化，同时生成可用于分析 Dump 的最终 PDB。
    CONFIG(release, debug|release) {
        QMAKE_CFLAGS_RELEASE += /Z7
        QMAKE_CXXFLAGS_RELEASE += /Z7
        QMAKE_LFLAGS_RELEASE += /DEBUG:FULL /OPT:REF /OPT:ICF
    }
}

# 由顶层 subdirs 工程写入的构建根目录；存在时保证嵌套子项目也输出到同一个 bin/lib
!isEmpty(FLUENT_BUILD_ROOT) {
    DESTDIR_BIN = $$FLUENT_BUILD_ROOT/bin
    DESTDIR_LIB = $$FLUENT_BUILD_ROOT/lib
} else {
    DESTDIR_BIN = $$OUT_PWD/../bin
    DESTDIR_LIB = $$OUT_PWD/../lib
}

isEmpty(PREFIX) {
    PREFIX = $$PWD/install
}
