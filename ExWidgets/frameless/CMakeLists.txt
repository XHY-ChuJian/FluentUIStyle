file(GLOB FRAMELESS_SOURCES
    "*.cpp"
    "*.h"
)
add_library(ExWidgetsFrameless SHARED
    ${FRAMELESS_SOURCES}
)

add_library(ExWidgets::Frameless ALIAS ExWidgetsFrameless)

target_compile_definitions(ExWidgetsFrameless PRIVATE
    EXWIDGETS_FRAMELESS_LIBRARY
)

target_include_directories(ExWidgetsFrameless PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets/frameless>
)

target_link_libraries(ExWidgetsFrameless PUBLIC
    ExWidgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    QWindowKit::Widgets
)

set_target_properties(ExWidgetsFrameless PROPERTIES
    DEBUG_POSTFIX d
    EXPORT_NAME Frameless
)
