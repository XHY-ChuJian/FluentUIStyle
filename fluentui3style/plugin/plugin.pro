include(../../common.pri)

TEMPLATE = lib
CONFIG += plugin
TARGET = FluentUI3StylePlugin
CONFIG(debug, debug|release): TARGET = $${TARGET}d

# 编译插件默认开启  宏
CONFIG += use_qt_style
use_qt_style {
    DEFINES += 
}

include(../fluentui3style.pri)

SOURCES += ../fluentui3styleplugin.cpp
HEADERS += ../fluentui3styleplugin.h
OTHER_FILES += ../fluentui3styleplugin.json

DESTDIR = $$DESTDIR_BIN/plugins/styles

# 安装插件目录
target.path = $$PREFIX/bin/plugins/styles
INSTALLS += target

# --- Auto copy to Qt plugins dir ---
TARGET_STYLE_DIR = $$[QT_INSTALL_PLUGINS]/styles
TARGET_HEADER_DIR = $$[QT_INSTALL_HEADERS]/FluentUI3Style

DLL_PATH = $${DESTDIR}/$${TARGET}.dll
STYLE_PROPS_HEADER = $$PWD/../fluentui3styleproperties.h

win32-g++ {
    # MinGW环境使用cp命令
    QMAKE_POST_LINK += $$quote(test -d "$$TARGET_STYLE_DIR" || mkdir -p "$$TARGET_STYLE_DIR") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(cp -f "$$DLL_PATH" "$$TARGET_STYLE_DIR/") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(test -d "$$TARGET_HEADER_DIR" || mkdir -p "$$TARGET_HEADER_DIR") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(cp -f "$$STYLE_PROPS_HEADER" "$$TARGET_HEADER_DIR/") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(echo "Success! Plugin copied to Qt dir.")
    CONFIG(release, debug|release) {
        # 在 MinGW 下的 Release 也生成调试信息，便于分析崩溃堆栈
        QMAKE_CXXFLAGS_RELEASE += -g
    }
}

win32-msvc {
    # MSVC环境使用CMD命令
    TARGET_STYLE_DIR_WIN = $$replace(TARGET_STYLE_DIR, /, \\)
    TARGET_HEADER_DIR_WIN = $$replace(TARGET_HEADER_DIR, /, \\)
    DLL_PATH_WIN = $$replace(DLL_PATH, /, \\)
    STYLE_PROPS_HEADER_WIN = $$replace(STYLE_PROPS_HEADER, /, \\)
    QMAKE_POST_LINK += $$quote(if not exist "$$TARGET_STYLE_DIR_WIN" mkdir "$$TARGET_STYLE_DIR_WIN") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(copy /y "$$DLL_PATH_WIN" "$$TARGET_STYLE_DIR_WIN" > nul) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(if not exist "$$TARGET_HEADER_DIR_WIN" mkdir "$$TARGET_HEADER_DIR_WIN") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(copy /y "$$STYLE_PROPS_HEADER_WIN" "$$TARGET_HEADER_DIR_WIN" > nul) $$escape_expand(\\n\\t)

    CONFIG(debug, debug|release) {
        PDB_PATH = $${DESTDIR}/$${TARGET}.pdb
        PDB_PATH_WIN = $$replace(PDB_PATH, /, \\)
        QMAKE_POST_LINK += $$quote(if exist "$$PDB_PATH_WIN" copy /y "$$PDB_PATH_WIN" "$$TARGET_STYLE_DIR_WIN" > nul) $$escape_expand(\\n\\t)
    }
    CONFIG(release, debug|release) {
        PDB_PATH = $${DESTDIR}/$${TARGET}.pdb
        PDB_PATH_WIN = $$replace(PDB_PATH, /, \\)
        QMAKE_POST_LINK += $$quote(if exist "$$PDB_PATH_WIN" copy /y "$$PDB_PATH_WIN" "$$TARGET_STYLE_DIR_WIN" > nul) $$escape_expand(\\n\\t)
    }
    QMAKE_POST_LINK += $$quote(echo Success! Plugin copied to Qt dir.)

    # 在 MSVC 的 Release 下生成 PDB（/Zi + 链接器 /DEBUG）
    CONFIG(release, debug|release) {
        QMAKE_CFLAGS_RELEASE += /Zi
        QMAKE_CXXFLAGS_RELEASE += /Zi
        QMAKE_LFLAGS_RELEASE += /DEBUG
    }
}
