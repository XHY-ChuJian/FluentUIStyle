cmake_minimum_required(VERSION 3.16)

project(fluentw3uistyle VERSION 0.1 LANGUAGES CXX)

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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)

option(BUILD_LIBRARY "Build as library" ON)
option(BUILD_PLUGIN "Build as plugin" ON)
option(BUILD_EXAMPLE "Build example application" ON)

# Local runs: keep all configs in the same output dir (Windows/MSVC convenience).
# Note: Debug/Release will overwrite each other if built to the same tree.
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

add_subdirectory(ExWidgets)

if(BUILD_LIBRARY)
    add_subdirectory(fluentui3style)
endif()

if(BUILD_PLUGIN)
    add_subdirectory(fluentui3style/plugin)
endif()

if(BUILD_EXAMPLE)
    add_subdirectory(Example)
endif()

# --- Install (DLL / import lib / headers / CMake packages) ---
install(TARGETS ExWidgets
    EXPORT ExWidgetsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES $<TARGET_PDB_FILE:ExWidgets>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    OPTIONAL
)

if(BUILD_LIBRARY)
    install(TARGETS FluentUI3Style
        EXPORT FluentUI3StyleTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(FILES $<TARGET_PDB_FILE:FluentUI3Style>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ExWidgets/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ExWidgets
    FILES_MATCHING PATTERN "*.h"
)

if(BUILD_LIBRARY)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fluentui3style/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fluentui3style
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
