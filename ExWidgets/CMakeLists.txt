# ExWidgets owns the spectrum backend dependency.  Parent applications only
# need to link ExWidgets and should not have to create the kissfft target.
if(NOT TARGET kissfft)
    if(NOT CMAKE_C_COMPILER_LOADED)
        enable_language(C)
    endif()
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/../3rd/kissfft"
        "${CMAKE_CURRENT_BINARY_DIR}/kissfft"
    )
endif()

file(GLOB_RECURSE EXWIDGETS_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
)
# designer/ 是独立的 Qt Designer 插件，不能编进 ExWidgets 库。
list(FILTER EXWIDGETS_SOURCES EXCLUDE REGEX "[/\\\\]designer[/\\\\]")

add_library(ExWidgets SHARED
    ${EXWIDGETS_SOURCES}
)

add_library(ExWidgets::ExWidgets ALIAS ExWidgets)

target_compile_definitions(ExWidgets PRIVATE EXWIDGETS_LIBRARY)

target_include_directories(ExWidgets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/navigation>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dialogs>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gauges>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/color>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controls>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets>
)

target_include_directories(ExWidgets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Colors
    ${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/kissfft
)

target_link_libraries(ExWidgets PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_link_libraries(ExWidgets PRIVATE kissfft)

set_target_properties(ExWidgets PROPERTIES
    DEBUG_POSTFIX d
)

option(EXWIDGETS_BUILD_DESIGNER_PLUGIN
    "Build the Qt Designer plugin for ExWidgets"
    ON)
if(ANDROID OR IOS)
    set(EXWIDGETS_BUILD_DESIGNER_PLUGIN OFF CACHE BOOL
        "Build the Qt Designer plugin for ExWidgets" FORCE)
endif()
if(EXWIDGETS_BUILD_DESIGNER_PLUGIN)
    add_subdirectory(designer)
endif()
