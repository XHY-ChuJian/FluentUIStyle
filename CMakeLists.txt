cmake_minimum_required(VERSION 3.16)

project(fluentw3uistyle VERSION 0.1 LANGUAGES CXX C)

include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MSVC)
    # /utf-8: avoid C4819 when sources contain UTF-8 (e.g. Chinese comments) under CP936
    add_compile_options(/wd4273 /utf-8)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
)

# Qt 6.10+ private CMake packages are required when using <private/...> headers.
# Older Qt 6.x installs (e.g. 6.6) often omit them; do not require globally.
set(FLUENT_QT_PRIVATE_LIBS "")
if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate WidgetsPrivate)
    list(APPEND FLUENT_QT_PRIVATE_LIBS
        Qt6::CorePrivate
        Qt6::GuiPrivate
        Qt6::WidgetsPrivate
    )
endif()

# Local runs: keep all configs in the same output dir (Windows/MSVC convenience).
# Optional components such as ExWidgetsFrameless also inherit these paths.
set(_out_bin "${CMAKE_BINARY_DIR}/bin")
set(_out_lib "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_out_bin}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_out_lib}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${_out_lib}")

foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_cfg} "${_out_bin}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_cfg} "${_out_lib}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_cfg} "${_out_lib}")
endforeach()

option(BUILD_LIBRARY "Build as library" ON)
option(BUILD_PLUGIN "Build as plugin (Windows only)" ON)
option(BUILD_GALLERY "Build the Gallery application" ON)
option(BUILD_AUDIOMATIC_MINI "Build Audiomatic mini player" ON)
option(BUILD_WIN11_CLOCK "Build the Win11 Clock sample application" ON)

if(NOT WIN32)
    set(BUILD_PLUGIN OFF)
endif()

add_subdirectory(ExWidgets)

if(BUILD_LIBRARY)
    add_subdirectory(fluentui3style)
endif()

if(BUILD_PLUGIN)
    add_subdirectory(fluentui3style/plugin)
endif()

if(BUILD_GALLERY OR BUILD_AUDIOMATIC_MINI OR BUILD_WIN11_CLOCK)
    add_subdirectory(Examples)
endif()

# --- Install (DLL / import lib / headers / CMake packages) ---
install(TARGETS ExWidgets
    EXPORT ExWidgetsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(TARGET ExWidgetsFrameless)
    install(TARGETS ExWidgetsFrameless
        EXPORT ExWidgetsTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:ExWidgets>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
    if(TARGET ExWidgetsFrameless)
        install(FILES $<TARGET_PDB_FILE:ExWidgetsFrameless>
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            OPTIONAL
        )
    endif()
endif()

if(BUILD_LIBRARY)
    install(TARGETS FluentUI3Style
        EXPORT FluentUI3StyleTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    if(MSVC)
        install(FILES $<TARGET_PDB_FILE:FluentUI3Style>
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            OPTIONAL
        )
    endif()
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ExWidgets/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/FluentUI3Colors/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Colors
    FILES_MATCHING PATTERN "*.h"
)

if(BUILD_LIBRARY)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fluentui3style/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/FluentUI3Style
        FILES_MATCHING PATTERN "*.h"
    )
endif()

include(CMakePackageConfigHelpers)

set(_cfg_dir "${CMAKE_CURRENT_BINARY_DIR}/cmake")
file(MAKE_DIRECTORY "${_cfg_dir}")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ExWidgetsConfig.cmake.in"
    "${_cfg_dir}/ExWidgetsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ExWidgets
)

write_basic_package_version_file(
    "${_cfg_dir}/ExWidgetsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(EXPORT ExWidgetsTargets
    FILE ExWidgetsTargets.cmake
    NAMESPACE ExWidgets::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ExWidgets
)

install(FILES
    "${_cfg_dir}/ExWidgetsConfig.cmake"
    "${_cfg_dir}/ExWidgetsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ExWidgets
)

if(BUILD_LIBRARY)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FluentUI3StyleConfig.cmake.in"
        "${_cfg_dir}/FluentUI3StyleConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FluentUI3Style
    )

    write_basic_package_version_file(
        "${_cfg_dir}/FluentUI3StyleConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion
    )

    install(EXPORT FluentUI3StyleTargets
        FILE FluentUI3StyleTargets.cmake
        NAMESPACE FluentUI3Style::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FluentUI3Style
    )

    install(FILES
        "${_cfg_dir}/FluentUI3StyleConfig.cmake"
        "${_cfg_dir}/FluentUI3StyleConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FluentUI3Style
    )
endif()

if(BUILD_PLUGIN)
    install(TARGETS FluentUI3StylePlugin
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/plugins/styles
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugins/styles
    )
endif()
