set(GALLERY_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    installedsoftwaretablewidget.cpp
    installedsoftwaretablewidget.h
    tabshowcasewidget.cpp
    tabshowcasewidget.h
    dialogshowcasewidget.cpp
    dialogshowcasewidget.h
    feedbackshowcasewidget.cpp
    feedbackshowcasewidget.h
    colorshowcasewidget.cpp
    colorshowcasewidget.h
    borderbeamshowcasewidget.cpp
    borderbeamshowcasewidget.h
    audiolevelmetershowcasewidget.cpp
    audiolevelmetershowcasewidget.h
    rangeslidershowcasewidget.cpp
    rangeslidershowcasewidget.h
    liquidgaugeshowcasewidget.cpp
    liquidgaugeshowcasewidget.h
    radialgaugeshowcasewidget.cpp
    radialgaugeshowcasewidget.h
    progressringshowcasewidget.cpp
    progressringshowcasewidget.h
    timelineshowcasewidget.cpp
    timelineshowcasewidget.h
    changelogtimelinewidget.cpp
    changelogtimelinewidget.h
    systemresourceswidget.cpp
    systemresourceswidget.h
    systemresources/systemresourceprovider.cpp
    systemresources/systemresourceprovider.h
    systemresources/systemresourcesnapshot.h
    diagnostics/crashdump.cpp
    diagnostics/crashdump.h
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
    list(APPEND GALLERY_SOURCES
        applanguage.cpp
        applanguage.h
        translations/i18n_embed.qrc
        appicon.rc
        appicon.ico
    )
endif()

if(WIN32)
    set(_gallery_ts "${CMAKE_CURRENT_SOURCE_DIR}/translations/Gallery_en_US.ts")
    set(_gallery_qm "${CMAKE_CURRENT_SOURCE_DIR}/translations/Gallery_en_US.qm")
    set(_gallery_i18n_qrc "${CMAKE_CURRENT_SOURCE_DIR}/translations/i18n_embed.qrc")

    get_target_property(_gallery_qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_gallery_qt_bin "${_gallery_qmake}" DIRECTORY)
    find_program(GALLERY_LRELEASE NAMES lrelease lrelease-qt6 lrelease-qt5 HINTS "${_gallery_qt_bin}" "${_gallery_qt_bin}/..")
    find_program(GALLERY_LUPDATE NAMES lupdate lupdate-qt6 lupdate-qt5 HINTS "${_gallery_qt_bin}" "${_gallery_qt_bin}/..")

    if(GALLERY_LRELEASE)
        add_custom_command(
            OUTPUT "${_gallery_qm}"
            COMMAND "${GALLERY_LRELEASE}" "${_gallery_ts}" -qm "${_gallery_qm}"
            DEPENDS "${_gallery_ts}"
            COMMENT "lrelease: Gallery_en_US.ts -> Gallery_en_US.qm"
            VERBATIM)
        set_source_files_properties("${_gallery_i18n_qrc}" PROPERTIES OBJECT_DEPENDS "${_gallery_qm}")
    else()
        if(NOT EXISTS "${_gallery_qm}")
            message(FATAL_ERROR "Gallery: 'lrelease' not found and translations/Gallery_en_US.qm is missing. Install Qt Linguist tools.")
        endif()
    endif()
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Gallery
        MANUAL_FINALIZATION
        ${GALLERY_SOURCES}
    )
else()
    add_executable(Gallery
        ${GALLERY_SOURCES}
    )
endif()

if(WIN32)
    target_compile_definitions(Gallery PRIVATE FLUENT_USE_QT_STYLE GALLERY_ENABLE_I18N)
    target_link_libraries(Gallery PRIVATE dbghelp iphlpapi pdh)
else()
    target_link_libraries(Gallery PRIVATE FluentUI3Style)
    set_target_properties(Gallery PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()

target_link_libraries(Gallery PRIVATE
    ExWidgets::ExWidgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    ${FLUENT_QT_PRIVATE_LIBS}
)

if(TARGET ExWidgets::Frameless)
    target_compile_definitions(Gallery PRIVATE GALLERY_ENABLE_FRAMELESS)
    target_link_libraries(Gallery PRIVATE ExWidgets::Frameless)
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_link_libraries(Gallery PRIVATE
        AudiomaticMiniWidgets
        Qt6::Multimedia
    )
else()
    target_sources(Gallery PRIVATE
        spectrumshowcasewidget.cpp
        spectrumshowcasewidget.h
        sinewavegenerator.cpp
        sinewavegenerator.h
    )
endif()

set_target_properties(Gallery PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    DEBUG_POSTFIX d
)

if(WIN32 AND GALLERY_LUPDATE)
    add_custom_target(
        gallery-update-translations
        COMMAND
            "${GALLERY_LUPDATE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui"
            "${CMAKE_CURRENT_SOURCE_DIR}/tabshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/dialogshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/feedbackshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/colorshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/borderbeamshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/audiolevelmetershowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/rangeslidershowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/liquidgaugeshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/radialgaugeshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/progressringshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/timelineshowcasewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/changelogtimelinewidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/systemresourceswidget.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/systemresources/systemresourceprovider.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/applanguage.cpp"
            -ts
            "${_gallery_ts}"
            -locations
            none
            -no-obsolete
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "lupdate: refresh examples/Gallery/translations/Gallery_en_US.ts"
        VERBATIM)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_custom_target(
            gallery-fill-translations
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/fill_en_ts.py"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "fill_en_ts.py: apply English table to Gallery_en_US.ts"
            VERBATIM)
        add_dependencies(gallery-fill-translations gallery-update-translations)
    endif()
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(Gallery)
endif()
