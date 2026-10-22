file(GLOB_RECURSE GALLERY_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    "*.ui"
    "*.qrc"
)

# Remove conditional sources so they aren't compiled unless their conditions are met
set(GALLERY_LOCAL_PLAYER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/audiomaticplayerwidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/audiomaticplayerwidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/audiomaticplayerwidget.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/audiospectrumanalyzer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/audiospectrumanalyzer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/jamendoclient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer/jamendoclient.h"
)

list(REMOVE_ITEM GALLERY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/applanguage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/applanguage.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/i18n_embed.qrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/pagespectrum.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/pagespectrum.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/sinewavegenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pages/media/sinewavegenerator.h"
    ${GALLERY_LOCAL_PLAYER_SOURCES}
)

if(WIN32)
    list(APPEND GALLERY_SOURCES
        core/applanguage.cpp
        core/applanguage.h
        translations/i18n_embed.qrc
        resources/appicon.rc
        resources/appicon.ico
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

target_include_directories(Gallery PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/core/systemresources
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/basic
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/gauges
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/media
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/media/localplayer
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/timeline
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/exwidgets
    ${CMAKE_CURRENT_SOURCE_DIR}/pages/info
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/font-icon
)

if(TARGET FluentUI3Style)
    target_link_libraries(Gallery PRIVATE FluentUI3Style)
endif()

if(WIN32)
    target_compile_definitions(Gallery PRIVATE GALLERY_ENABLE_I18N)
    target_link_libraries(Gallery PRIVATE dbghelp iphlpapi pdh)
else()
    if(TARGET FluentUI3Style)
        target_compile_definitions(Gallery PRIVATE HAS_FLUENTUI3_STYLE_LIB)
    endif()
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

if(NOT ANDROID AND TARGET Frameless::Frameless)
    target_compile_definitions(Gallery PRIVATE GALLERY_ENABLE_FRAMELESS)
    target_link_libraries(Gallery PRIVATE Frameless::Frameless)
endif()

if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND NOT ANDROID)
    target_sources(Gallery PRIVATE ${GALLERY_LOCAL_PLAYER_SOURCES})
    target_link_libraries(Gallery PRIVATE
        Qt6::Multimedia
        Qt6::Network
    )
else()
    target_sources(Gallery PRIVATE
        pages/media/pagespectrum.cpp
        pages/media/pagespectrum.h
        pages/media/sinewavegenerator.cpp
        pages/media/sinewavegenerator.h
    )
endif()

if(WIN32)
    set_target_properties(Gallery PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
endif()

set_target_properties(Gallery PROPERTIES
    DEBUG_POSTFIX d
)

if(ANDROID)
    set_property(TARGET Gallery PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../android")
endif()

if(WIN32 AND GALLERY_LUPDATE)
    add_custom_target(
        gallery-update-translations
        COMMAND
            "${GALLERY_LUPDATE}"
            ${GALLERY_SOURCES}
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
