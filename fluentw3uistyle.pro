TEMPLATE = subdirs
CONFIG += ordered

CONFIG += build_library
CONFIG += build_plugin
CONFIG += build_example

SUBDIRS += ExWidgets

build_library {
    SUBDIRS += FluentUI3Style
}

build_plugin {
    SUBDIRS += FluentUI3StylePlugin
    FluentUI3StylePlugin.subdir = FluentUI3Style/plugin
    FluentUI3StylePlugin.depends = FluentUI3Style
}

build_example {
    SUBDIRS += Example
    Example.depends = ExWidgets FluentUI3Style
}
