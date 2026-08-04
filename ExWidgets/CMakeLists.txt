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

set(_exwidgets_frameless_default ON)
if(QT_VERSION VERSION_LESS "5.15.2")
    set(_exwidgets_frameless_default OFF)
endif()

option(EXWIDGETS_BUILD_FRAMELESS
    "Build the optional QWindowKit-based frameless window component"
    ${_exwidgets_frameless_default}
)

if(EXWIDGETS_BUILD_FRAMELESS AND QT_VERSION VERSION_LESS "5.15.2")
    message(STATUS
        "ExWidgets frameless component disabled: Qt ${QT_VERSION} < 5.15.2."
    )
    set(EXWIDGETS_BUILD_FRAMELESS OFF CACHE BOOL
        "Build the optional QWindowKit-based frameless window component"
        FORCE
    )
endif()

if(EXWIDGETS_BUILD_FRAMELESS)
    if(CMAKE_VERSION VERSION_LESS "3.19")
        message(FATAL_ERROR
            "EXWIDGETS_BUILD_FRAMELESS requires CMake 3.19 or newer (found ${CMAKE_VERSION})."
        )
    endif()

    if(NOT TARGET QWindowKit::Widgets)
        set(_exwidgets_qwindowkit_dir "${CMAKE_CURRENT_SOURCE_DIR}/../3rd/qwindowkit")
        if(EXISTS "${_exwidgets_qwindowkit_dir}/CMakeLists.txt")
            set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(QWINDOWKIT_BUILD_QUICK OFF CACHE BOOL "" FORCE)
            set(QWINDOWKIT_BUILD_WIDGETS ON CACHE BOOL "" FORCE)
            set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
            set(QWINDOWKIT_INSTALL ON CACHE BOOL "" FORCE)
            add_subdirectory(
                "${_exwidgets_qwindowkit_dir}"
                "${CMAKE_CURRENT_BINARY_DIR}/qwindowkit"
            )
        else()
            find_package(QWindowKit CONFIG REQUIRED)
        endif()
    endif()

    if(NOT TARGET QWindowKit::Widgets)
        message(FATAL_ERROR
            "EXWIDGETS_BUILD_FRAMELESS is ON, but QWindowKit::Widgets is unavailable."
        )
    endif()

    # QWindowKit's helper may choose its own output folders. Keep its runtime
    # libraries next to the rest of this build when the parent defines them.
    foreach(_qwk_target IN ITEMS QWKCore QWKWidgets)
        if(TARGET ${_qwk_target} AND CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            set_target_properties(${_qwk_target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
            )
            foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
                if(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg})
                    set_target_properties(${_qwk_target} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg}}"
                        LIBRARY_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg}}"
                        ARCHIVE_OUTPUT_DIRECTORY_${_cfg} "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg}}"
                    )
                endif()
            endforeach()
        endif()
    endforeach()
endif()

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

add_library(ExWidgets::ExWidgets ALIAS ExWidgets)

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

if(EXWIDGETS_BUILD_FRAMELESS)
    add_subdirectory(frameless)
endif()
