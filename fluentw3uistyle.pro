TEMPLATE = subdirs
CONFIG += ordered

CONFIG += build_library
CONFIG += build_plugin
CONFIG += build_example

SUBDIRS += ExWidgets

build_library {
    fluentui3style.subdir = FluentUI3Style
    fluentui3style.file = FluentUI3Style.pro
    SUBDIRS += FluentUI3Style
}

build_plugin {
    win32 {
        fluentui3styleplugin.subdir = FluentUI3Style/plugin
        SUBDIRS += fluentui3styleplugin
        fluentui3styleplugin.depends = FluentUI3Style
    }
}

build_example {
    SUBDIRS += Example
    Example.depends = ExWidgets
    build_library: Example.depends += FluentUI3Style
}
