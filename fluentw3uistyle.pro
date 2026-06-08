TEMPLATE = subdirs
CONFIG += ordered

CONFIG += build_library
CONFIG += build_plugin
CONFIG += build_example

SUBDIRS += ExWidgets

build_library {
    fluentui3style.subdir = fluentui3style
    fluentui3style.file = FluentUI3Style.pro
    SUBDIRS += fluentui3style
}

build_plugin {
    win32 {
        fluentui3styleplugin.subdir = fluentui3style/plugin
        fluentui3styleplugin.file = plugin.pro
        SUBDIRS += fluentui3styleplugin
        fluentui3styleplugin.depends = fluentui3style
    }
}

build_example {
    SUBDIRS += Example
    Example.depends = ExWidgets fluentui3style
}
