cmake_minimum_required(VERSION 3.16)

project(FluentUI3CreatorPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 寻找基础 Qt 组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# 自动探测或使用用户指定的 Qt Creator 路径
if(NOT QTCREATOR_DIR)
    set(_possible_qtcreator_dirs
        "D:/Qt/Qt6.10/Tools/QtCreator"
        "C:/Qt/Tools/QtCreator"
        "D:/Qt/Tools/QtCreator"
        "C:/Qt/Qt6.10/Tools/QtCreator"
    )
    foreach(_dir IN LISTS _possible_qtcreator_dirs)
        if(EXISTS "${_dir}")
            set(QTCREATOR_DIR "${_dir}" CACHE PATH "Path to Qt Creator installation" FORCE)
            message(STATUS "Auto-detected Qt Creator installation at: ${QTCREATOR_DIR}")
            break()
        endif()
    endforeach()
endif()

if(NOT QTCREATOR_DIR)
    message(FATAL_ERROR "Please specify -DQTCREATOR_DIR=<path/to/QtCreator> (e.g. D:/Qt/Qt6.10/Tools/QtCreator)")
endif()

# 插件源文件
set(PLUGIN_SOURCES
    fluentui3creatorplugin.h
    fluentui3creatorplugin.cpp
    FluentUI3CreatorPlugin.json
)

add_library(FluentUI3CreatorPlugin SHARED
    ${PLUGIN_SOURCES}
)

# 包含 Qt Creator 官方 SDK 头文件及本项目样式库头文件
target_include_directories(FluentUI3CreatorPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Style
    ${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Colors
    "${QTCREATOR_DIR}/include/qtcreator/src/libs"
    "${QTCREATOR_DIR}/include/qtcreator/src/plugins"
    "${QTCREATOR_DIR}/include/qtcreator"
    "${QTCREATOR_DIR}/include"
)

# 查找并链接 Qt Creator 官方 ExtensionSystem 库
find_library(QTC_EXTENSIONSYSTEM_LIB
    NAMES ExtensionSystem ExtensionSystem4 ExtensionSystem5 ExtensionSystem6 ExtensionSystem10 ExtensionSystem11 ExtensionSystem12 ExtensionSystem13 ExtensionSystem14 ExtensionSystem15
    PATHS
        "${QTCREATOR_DIR}/lib/qtcreator"
        "${QTCREATOR_DIR}/bin"
        "${QTCREATOR_DIR}/lib"
    NO_DEFAULT_PATH
)

if(NOT QTC_EXTENSIONSYSTEM_LIB)
    message(FATAL_ERROR "Could not find ExtensionSystem library under ${QTCREATOR_DIR}. Please check the path.")
else()
    message(STATUS "Found QtCreator ExtensionSystem library: ${QTC_EXTENSIONSYSTEM_LIB}")
endif()

# 链接核心样式库、Qt Creator ExtensionSystem 官方库及 Qt 库
target_link_libraries(FluentUI3CreatorPlugin PRIVATE
    FluentUI3Style
    ${QTC_EXTENSIONSYSTEM_LIB}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

# 设置插件输出路径
set_target_properties(FluentUI3CreatorPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtcreator_plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtcreator_plugins"
)

# 自动推导插件安装部署目录
if(NOT QTCREATOR_PLUGINS_INSTALL_DIR)
    if(EXISTS "${QTCREATOR_DIR}/lib/qtcreator/plugins")
        set(QTCREATOR_PLUGINS_INSTALL_DIR "${QTCREATOR_DIR}/lib/qtcreator/plugins")
    elseif(EXISTS "${QTCREATOR_DIR}/bin/plugins")
        set(QTCREATOR_PLUGINS_INSTALL_DIR "${QTCREATOR_DIR}/bin/plugins")
    elseif(EXISTS "${QTCREATOR_DIR}/plugins")
        set(QTCREATOR_PLUGINS_INSTALL_DIR "${QTCREATOR_DIR}/plugins")
    else()
        set(QTCREATOR_PLUGINS_INSTALL_DIR "${QTCREATOR_DIR}/lib/qtcreator/plugins")
    endif()
endif()

# 构建完成后自动部署插件及依赖的 FluentUI3Style.dll 到 Qt Creator 目录
if(QTCREATOR_PLUGINS_INSTALL_DIR)
    add_custom_command(TARGET FluentUI3CreatorPlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QTCREATOR_PLUGINS_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo "Deploying FluentUI3CreatorPlugin to ${QTCREATOR_PLUGINS_INSTALL_DIR}..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:FluentUI3CreatorPlugin>" "${QTCREATOR_PLUGINS_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:FluentUI3Style>" "${QTCREATOR_PLUGINS_INSTALL_DIR}"
    )
    message(STATUS "QtCreator plugin deployment target: ${QTCREATOR_PLUGINS_INSTALL_DIR}")
endif()
