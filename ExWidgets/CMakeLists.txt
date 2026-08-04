set(EXWIDGETS_SOURCES
    colorgradientslider.cpp
    colorgradientslider.h
    exrangeslider.cpp
    exrangeslider.h
    excolorpicker.cpp
    excolorpicker.h
    excolorpickerbutton.cpp
    excolorpickerbutton.h
    excombobox.cpp
    excombobox.h
    exmessagebox.cpp
    exmessagebox.h
    exspectrumwidget.cpp
    exspectrumwidget.h
    excontentdialog.cpp
    excontentdialog.h
    exnavtreewidget.cpp
    exnavtreewidget.h
    exwinuinavigationview.cpp
    exwinuinavigationview.h
    exstackedwidget.cpp
    exstackedwidget.h
    extabwidget.cpp
    extabwidget.h
)

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

add_library(ExWidgets SHARED
    ${EXWIDGETS_SOURCES}
)

target_compile_definitions(ExWidgets PRIVATE EXWIDGETS_LIBRARY)

target_include_directories(ExWidgets PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Colors>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Colors>
)

target_include_directories(ExWidgets PRIVATE
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
