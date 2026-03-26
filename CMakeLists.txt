cmake_minimum_required(VERSION 3.16)

project(fluentw3uistyle VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
    fluentui3style/fluentui3style.cpp
    fluentui3style/fluentui3style.h
    fluentui3style/fluentui3colors.cpp
    fluentui3style/fluentui3colors.h
    fluentui3style/fluentuiappearance.cpp
    fluentui3style/fluentuiappearance.h
    fluentui3style/palettemanager.cpp
    fluentui3style/palettemanager.h
    fluentui3style/qhexstring_p.h
    fluentui3style/qstyleanimation.cpp
    fluentui3style/qstyleanimation_p.h
    fluentui3style/qstylehelper.cpp
    fluentui3style/qstylehelper_p.h
    fluentui3style/resource.qrc
)

if(WIN32)
    list(APPEND PROJECT_SOURCES
        appicon.rc
        appicon.ico
    )
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_standard_project_setup()

    qt_add_executable(fluentw3uistyle
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(fluentw3uistyle WIN32
        ${PROJECT_SOURCES}
    )
endif()

target_include_directories(fluentw3uistyle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fluentui3style
)

target_link_libraries(fluentw3uistyle PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

set_target_properties(fluentw3uistyle PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(fluentw3uistyle)
endif()
