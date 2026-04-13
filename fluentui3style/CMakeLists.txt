set(LIBRARY_SOURCES
    fluentui3style_global.h
    fluentui3style.cpp
    fluentui3style.h
    fluentui3colors.cpp
    fluentui3colors.h
    fluentuiappearance.cpp
    fluentuiappearance.h
    palettemanager.cpp
    palettemanager.h
    qhexstring_p.h
    qstyleanimation.cpp
    qstyleanimation_p.h
    qstylehelper.cpp
    qstylehelper_p.h
    resource.qrc
)

add_library(FluentUI3Style SHARED
    ${LIBRARY_SOURCES}
)

target_compile_definitions(FluentUI3Style PRIVATE FLUENTUI3STYLE_LIBRARY)

target_include_directories(FluentUI3Style PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Style>
)

target_link_libraries(FluentUI3Style PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)



set_target_properties(FluentUI3Style PROPERTIES
    DEBUG_POSTFIX d
)
