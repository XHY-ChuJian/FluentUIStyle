set(EXAMPLE_SOURCES
    main.cpp
    applanguage.cpp
    applanguage.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    tabshowcasewidget.cpp
    tabshowcasewidget.h
    aboutprojectwidget.cpp
    aboutprojectwidget.h
    segoeicongallerywidget.cpp
    segoeicongallerywidget.h
    font-icon/fonticon.cpp
    font-icon/fonticon.h
    font-icon/resource.qrc
    resources.qrc
    translations/i18n_embed.qrc
    appicon.rc
    appicon.ico
)

# --- translations: lrelease Example_en_US.ts -> Example_en_US.qm (same dir as i18n_embed.qrc) ---
set(_ex_ts "${CMAKE_CURRENT_SOURCE_DIR}/translations/Example_en_US.ts")
set(_ex_qm "${CMAKE_CURRENT_SOURCE_DIR}/translations/Example_en_US.qm")
set(_ex_i18n_qrc "${CMAKE_CURRENT_SOURCE_DIR}/translations/i18n_embed.qrc")

get_target_property(_ex_qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_ex_qt_bin "${_ex_qmake}" DIRECTORY)
find_program(EXAMPLE_LRELEASE NAMES lrelease lrelease-qt6 lrelease-qt5 HINTS "${_ex_qt_bin}" "${_ex_qt_bin}/..")
find_program(EXAMPLE_LUPDATE NAMES lupdate lupdate-qt6 lupdate-qt5 HINTS "${_ex_qt_bin}" "${_ex_qt_bin}/..")

if(EXAMPLE_LRELEASE)
    add_custom_command(
        OUTPUT "${_ex_qm}"
        COMMAND "${EXAMPLE_LRELEASE}" "${_ex_ts}" -qm "${_ex_qm}"
        DEPENDS "${_ex_ts}"
        COMMENT "lrelease: Example_en_US.ts -> Example_en_US.qm"
        VERBATIM)
    set_source_files_properties("${_ex_i18n_qrc}" PROPERTIES OBJECT_DEPENDS "${_ex_qm}")
else()
    if(NOT EXISTS "${_ex_qm}")
        message(FATAL_ERROR "Example: 'lrelease' not found and translations/Example_en_US.qm is missing. Install Qt Linguist tools.")
    endif()
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Example
        MANUAL_FINALIZATION
        ${EXAMPLE_SOURCES}
    )
else()
    add_executable(Example
        ${EXAMPLE_SOURCES}
    )
endif()

# target_include_directories(Example PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style
# )

target_compile_definitions(Example PRIVATE FLUENT_USE_QT_STYLE)

# Translations workflow:
#   1) Update .ts from sources: build target `example-update-translations` (or run lupdate manually).
#   2) Fill English in tools/fill_en_ts.py (dictionary), then: python Example/tools/fill_en_ts.py
#   3) Every build runs lrelease (.ts -> .qm); i18n_embed.qrc embeds translations/Example_en_US.qm.
target_link_libraries(Example PRIVATE
    # FluentUI3Style
    ExWidgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::WidgetsPrivate
    Qt${QT_VERSION_MAJOR}::CorePrivate
)

# Windows 平台链接 DWM API（Mica/Acrylic 效果所需）
if(WIN32)
    target_link_libraries(Example PRIVATE dwmapi)
endif()

set_target_properties(Example PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    DEBUG_POSTFIX d
)

if(EXAMPLE_LUPDATE)
    add_custom_target(
        example-update-translations
        COMMAND
            "${EXAMPLE_LUPDATE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui"
            "${CMAKE_CURRENT_SOURCE_DIR}/tabshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/applanguage.cpp"
            -ts
            "${_ex_ts}"
            -locations
            none
            -no-obsolete
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "lupdate: refresh Example/translations/Example_en_US.ts"
        VERBATIM)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_custom_target(
            example-fill-translations
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/fill_en_ts.py"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "fill_en_ts.py: apply English table to Example_en_US.ts"
            VERBATIM)
        add_dependencies(example-fill-translations example-update-translations)
    endif()
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(Example)
endif()