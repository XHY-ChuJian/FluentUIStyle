set(EXWIDGETS_SOURCES
    exstackedwidget.cpp
    exstackedwidget.h
    extabwidget.cpp
    extabwidget.h
)

add_library(ExWidgets SHARED
    ${EXWIDGETS_SOURCES}
)

target_compile_definitions(ExWidgets PRIVATE EXWIDGETS_LIBRARY)

target_include_directories(ExWidgets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets>
)

target_link_libraries(ExWidgets PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)



set_target_properties(ExWidgets PROPERTIES
    DEBUG_POSTFIX d
)