# Qt Designer custom-widget plugin. It links ExWidgets and is loaded from
# <Qt>/plugins/designer. UiPlugin is headers only; do not link Qt Designer.

find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS UiPlugin)
if(NOT TARGET Qt${QT_VERSION_MAJOR}::UiPlugin)
    message(STATUS "ExWidgets Designer plugin skipped: Qt${QT_VERSION_MAJOR}::UiPlugin not found")
    return()
endif()

set(_ex_designer_sources exwidgetsdesignerplugin.cpp)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_plugin(ExWidgetsDesignerPlugin
        SHARED
        ${_ex_designer_sources}
    )
else()
    add_library(ExWidgetsDesignerPlugin SHARED
        ${_ex_designer_sources}
    )
endif()

set_target_properties(ExWidgetsDesignerPlugin PROPERTIES
    AUTOMOC ON
    PREFIX ""
    DEBUG_POSTFIX d
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/designer
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/designer
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins/designer
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/plugins/designer
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins/designer
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/plugins/designer
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins/designer
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/plugins/designer
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins/designer
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/plugins/designer
)

target_link_libraries(ExWidgetsDesignerPlugin PRIVATE
    ExWidgets::ExWidgets
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::UiPlugin
)

# Copy into the Qt selected by this Kit after every successful build.
# Designer looks in plugins/designer. ExWidgets.dll must be next to the plugin
# and next to designer.exe, because Qt 5 does not search the plugin directory
# for dependent DLLs.
option(EXWIDGETS_COPY_DESIGNER_PLUGIN_TO_QT_DIR
    "Copy ExWidgetsDesignerPlugin into Qt plugins/designer after build"
    ON)

if(EXWIDGETS_COPY_DESIGNER_PLUGIN_TO_QT_DIR)
    set(_ex_qt_plugins_dir "")
    set(_ex_qt_bins_dir "")
    set(_ex_qmake_target "Qt${QT_VERSION_MAJOR}::qmake")
    if(TARGET ${_ex_qmake_target})
        get_target_property(_ex_qmake_executable ${_ex_qmake_target} IMPORTED_LOCATION)
        if(_ex_qmake_executable AND EXISTS "${_ex_qmake_executable}")
            execute_process(
                COMMAND "${_ex_qmake_executable}" -query QT_INSTALL_PLUGINS
                OUTPUT_VARIABLE _ex_qt_plugins_dir
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            execute_process(
                COMMAND "${_ex_qmake_executable}" -query QT_INSTALL_BINS
                OUTPUT_VARIABLE _ex_qt_bins_dir
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
    endif()

    if(_ex_qt_plugins_dir)
        set(_ex_qt_designer_dir "${_ex_qt_plugins_dir}/designer")
        message(STATUS "ExWidgets Designer plugin copy: ${_ex_qt_designer_dir}")

        add_custom_command(TARGET ExWidgetsDesignerPlugin POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_ex_qt_designer_dir}"
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ExWidgetsDesignerPlugin to ${_ex_qt_designer_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:ExWidgetsDesignerPlugin>"
                    "${_ex_qt_designer_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:ExWidgets>"
                    "${_ex_qt_designer_dir}"
            VERBATIM
        )

        if(_ex_qt_bins_dir)
            add_custom_command(TARGET ExWidgetsDesignerPlugin POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying ExWidgets runtime to ${_ex_qt_bins_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:ExWidgets>"
                        "${_ex_qt_bins_dir}"
                VERBATIM
            )
        endif()

        if(MSVC)
            add_custom_command(TARGET ExWidgetsDesignerPlugin POST_BUILD
                COMMAND cmd /c "if exist \"$<TARGET_PDB_FILE:ExWidgetsDesignerPlugin>\" \"${CMAKE_COMMAND}\" -E copy_if_different \"$<TARGET_PDB_FILE:ExWidgetsDesignerPlugin>\" \"${_ex_qt_designer_dir}\""
            )
        endif()
    else()
        message(WARNING "ExWidgets Designer plugin: QT_INSTALL_PLUGINS is unknown; the plugin will not be copied")
    endif()
endif()
