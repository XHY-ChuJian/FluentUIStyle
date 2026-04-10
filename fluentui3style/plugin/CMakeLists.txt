set(PLUGIN_SOURCES
    ../fluentui3style.cpp
    ../fluentui3style.h
    ../fluentui3colors.cpp
    ../fluentui3colors.h
    ../fluentuiappearance.cpp
    ../fluentuiappearance.h
    ../palettemanager.cpp
    ../palettemanager.h
    ../qhexstring_p.h
    ../qstyleanimation.cpp
    ../qstyleanimation_p.h
    ../qstylehelper.cpp
    ../qstylehelper_p.h
    ../resource.qrc
    ../fluentui3styleplugin.cpp
    ../fluentui3styleplugin.h
    ../fluentui3styleplugin.json
)


if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_plugin(FluentUI3StylePlugin
        SHARED
        ${PLUGIN_SOURCES}
    )
else()
    add_library(FluentUI3StylePlugin SHARED
        ${PLUGIN_SOURCES}
    )
endif()

target_compile_definitions(FluentUI3StylePlugin PRIVATE FLUENT_USE_QT_STYLE FLUENTUI3STYLE_LIBRARY)

target_include_directories(FluentUI3StylePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(FluentUI3StylePlugin PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

set_target_properties(FluentUI3StylePlugin PROPERTIES
    DEBUG_POSTFIX d
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/styles
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/styles
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins/styles
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins/styles
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins/styles
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins/styles
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins/styles
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins/styles
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins/styles
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins/styles
)





# --- Auto copy to Qt plugins dir ---
get_target_property(QMAKE_EXEC Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
execute_process(COMMAND ${QMAKE_EXEC} -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_PLUGINS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(QT_STYLE_PLUGINS_DIR "${QT_PLUGINS_DIR}/styles")

add_custom_command(TARGET FluentUI3StylePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_STYLE_PLUGINS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying FluentUI3StylePlugin to ${QT_STYLE_PLUGINS_DIR}..."
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:FluentUI3StylePlugin>" "${QT_STYLE_PLUGINS_DIR}"
)
if(MSVC)
    add_custom_command(TARGET FluentUI3StylePlugin POST_BUILD
        COMMAND cmd /c "if exist \"$<TARGET_PDB_FILE:FluentUI3StylePlugin>\" \"${CMAKE_COMMAND}\" -E copy \"$<TARGET_PDB_FILE:FluentUI3StylePlugin>\" \"${QT_STYLE_PLUGINS_DIR}\""
    )
endif()
add_custom_command(TARGET FluentUI3StylePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Success! Plugin copied to Qt dir."
)