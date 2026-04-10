include(../../common.pri)

TEMPLATE = lib
CONFIG += plugin
TARGET = FluentUI3StylePlugin
CONFIG(debug, debug|release): TARGET = $${TARGET}d

# 编译插件默认开启 FLUENT_USE_QT_STYLE 宏
CONFIG += use_qt_style
use_qt_style {
    DEFINES += FLUENT_USE_QT_STYLE
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
TARGET_STYLE_DIR_WIN = $$replace(TARGET_STYLE_DIR, /, \\)

DLL_PATH = $${DESTDIR}/$${TARGET}.dll
DLL_PATH_WIN = $$replace(DLL_PATH, /, \\)

win32 {
    QMAKE_POST_LINK += $$quote(if not exist "$$TARGET_STYLE_DIR_WIN" mkdir "$$TARGET_STYLE_DIR_WIN") $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$quote(copy /y "$$DLL_PATH_WIN" "$$TARGET_STYLE_DIR_WIN" > nul) $$escape_expand(\\n\\t)
    
    win32-msvc:CONFIG(debug, debug|release) {
        PDB_PATH = $${DESTDIR}/$${TARGET}.pdb
        PDB_PATH_WIN = $$replace(PDB_PATH, /, \\)
        QMAKE_POST_LINK += $$quote(if exist "$$PDB_PATH_WIN" copy /y "$$PDB_PATH_WIN" "$$TARGET_STYLE_DIR_WIN" > nul) $$escape_expand(\\n\\t)
    }
    QMAKE_POST_LINK += $$quote(echo Success! Plugin copied to Qt dir.)
}
