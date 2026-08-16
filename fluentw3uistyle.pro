FLUENT_BUILD_ROOT = $$OUT_PWD
cache(FLUENT_BUILD_ROOT, set)

TEMPLATE = subdirs
CONFIG += ordered

CONFIG += build_library
CONFIG += build_plugin
CONFIG += build_gallery

SUBDIRS += ExWidgets

build_library {
    FluentUI3Style.file = fluentui3style/FluentUI3Style.pro
    SUBDIRS += FluentUI3Style
}

build_plugin {
    win32 {
        FluentUI3StylePlugin.file = fluentui3style/plugin/plugin.pro
        SUBDIRS += FluentUI3StylePlugin
        FluentUI3StylePlugin.depends = FluentUI3Style
    }
}

build_gallery {
    Gallery.file = Examples/Gallery/Gallery.pro
    SUBDIRS += Gallery
    Gallery.depends = ExWidgets
    build_library: Gallery.depends += FluentUI3Style
}
