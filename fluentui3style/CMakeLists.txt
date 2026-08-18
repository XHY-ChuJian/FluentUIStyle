file(GLOB LIBRARY_SOURCES
    "*.cpp"
    "*.h"
    "*.qrc"
)

add_library(FluentUI3Style SHARED
    ${LIBRARY_SOURCES}
)

target_compile_definitions(FluentUI3Style PRIVATE FLUENTUI3STYLE_LIBRARY)

target_include_directories(FluentUI3Style PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Colors>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Style>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Colors>
)

target_link_libraries(FluentUI3Style PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

if(WIN32)
    target_link_libraries(FluentUI3Style PRIVATE dwmapi)
endif()



set_target_properties(FluentUI3Style PROPERTIES
    DEBUG_POSTFIX d
)
