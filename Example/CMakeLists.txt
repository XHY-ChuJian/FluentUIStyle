set(EXAMPLE_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    frameless/fluenttitlebar.cpp
    frameless/fluenttitlebar.h
    frameless/fluentwindowframe.cpp
    frameless/fluentwindowframe.h
    installedsoftwaretablewidget.cpp
    installedsoftwaretablewidget.h
    tabshowcasewidget.cpp
    tabshowcasewidget.h
    dialogshowcasewidget.cpp
    dialogshowcasewidget.h
    colorshowcasewidget.cpp
    colorshowcasewidget.h
    aboutprojectwidget.cpp
    aboutprojectwidget.h
    segoeicongallerywidget.cpp
    segoeicongallerywidget.h
    font-icon/fonticon.cpp
    font-icon/fonticon.h
    font-icon/resource.qrc
    resources.qrc
)

if(WIN32)
    list(APPEND EXAMPLE_SOURCES
        applanguage.cpp
        applanguage.h
        translations/i18n_embed.qrc
        appicon.rc
        appicon.ico
    )
endif()

if(WIN32)
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

if(WIN32)
    target_compile_definitions(Example PRIVATE FLUENT_USE_QT_STYLE EXAMPLE_ENABLE_I18N)
else()
    target_link_libraries(Example PRIVATE FluentUI3Style)
    set_target_properties(Example PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()

target_include_directories(Example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/frameless
)

target_link_libraries(Example PRIVATE
    ExWidgets
    QWindowKit::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    ${FLUENT_QT_PRIVATE_LIBS}
)

set_target_properties(Example PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    DEBUG_POSTFIX d
)

# $<TARGET_FILE:...> is configuration-aware (Debug -> QWKCored.dll, Release -> QWKCore.dll).
# QWK targets already output to ${CMAKE_BINARY_DIR}/bin; this POST_BUILD is a safety net.
if(TARGET QWKCore AND TARGET QWKWidgets)
    add_dependencies(Example QWKCore QWKWidgets)

    add_custom_command(TARGET Example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:QWKCore>"
            "$<TARGET_FILE_DIR:Example>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:QWKWidgets>"
            "$<TARGET_FILE_DIR:Example>"
        COMMENT "Copy QWindowKit ($<CONFIG>) runtime libraries next to Example"
        VERBATIM
    )
endif()

if(WIN32 AND EXAMPLE_LUPDATE)
    add_custom_target(
        example-update-translations
        COMMAND
            "${EXAMPLE_LUPDATE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui"
            "${CMAKE_CURRENT_SOURCE_DIR}/tabshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/dialogshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/colorshowcasewidget.cpp"
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
