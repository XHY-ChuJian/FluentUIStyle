cmake_minimum_required(VERSION 3.16)

project(FluentSerial VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/wd4273 /utf-8)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets SerialPort)

if(NOT TARGET ExWidgets::ExWidgets)
    get_filename_component(WINDOW11STYLE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    add_subdirectory("${WINDOW11STYLE_ROOT}/ExWidgets"
                     "${CMAKE_CURRENT_BINARY_DIR}/dependencies/ExWidgets")
endif()

file(GLOB_RECURSE FLUENTSERIAL_SOURCES
    "src/*.cpp"
    "src/*.h"
    "*.rc"
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/res.qrc")
    list(APPEND FLUENTSERIAL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/res.qrc")
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(FluentSerial MANUAL_FINALIZATION ${FLUENTSERIAL_SOURCES})
else()
    add_executable(FluentSerial ${FLUENTSERIAL_SOURCES})
endif()

target_include_directories(FluentSerial PRIVATE
    src
    src/core
    src/widgets
    src/pages
)

target_link_libraries(FluentSerial PRIVATE
    ExWidgets::ExWidgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
)

if(NOT WIN32)
    target_link_libraries(FluentSerial PRIVATE FluentUI3Style)
endif()

if(TARGET ExWidgets::Frameless)
    target_compile_definitions(FluentSerial PRIVATE FLUENTSERIAL_ENABLE_FRAMELESS)
    target_link_libraries(FluentSerial PRIVATE ExWidgets::Frameless)
endif()

set_target_properties(FluentSerial PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    DEBUG_POSTFIX d
)

if(NOT WIN32)
    set_target_properties(FluentSerial PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(FluentSerial)
endif()
