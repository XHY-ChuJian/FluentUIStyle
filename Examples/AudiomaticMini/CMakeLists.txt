if(QT_VERSION_MAJOR LESS 6)
    message(STATUS "AudiomaticMini: skipped (requires Qt 6 for multimedia playback)")
    return()
endif()

set(AUDIOMATIC_MINI_WIDGET_SOURCES
    audiomaticimmersiveview.cpp
    audiomaticimmersiveview.h
    audiomaticplayerwidget.cpp
    audiomaticplayerwidget.h
    audiomaticplayerwidget.ui
    audiospectrumanalyzer.cpp
    audiospectrumanalyzer.h
    jamendoclient.cpp
    jamendoclient.h
    lyricsmanager.cpp
    lyricsmanager.h
    systemaudiocapture.cpp
    systemaudiocapture.h
    systemmediawatcher.cpp
    systemmediawatcher.h
)

add_library(AudiomaticMiniWidgets STATIC ${AUDIOMATIC_MINI_WIDGET_SOURCES})

target_include_directories(AudiomaticMiniWidgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../fluentui3style
)

target_link_libraries(AudiomaticMiniWidgets PUBLIC
    ExWidgets::ExWidgets
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    ${FLUENT_QT_PRIVATE_LIBS}
)

if(WIN32)
    target_link_libraries(AudiomaticMiniWidgets PUBLIC ole32 windowsapp)
endif()

if(BUILD_EXAMPLES)
    set(AUDIOMATIC_MINI_APP_SOURCES
        main.cpp
        playerwindow.cpp
        playerwindow.h
        spectrumdemowindow.cpp
        spectrumdemowindow.h
        sinewavegenerator.cpp
        sinewavegenerator.h
    )

    set(app_icon_res "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
    qt_add_executable(AudiomaticMini
        MANUAL_FINALIZATION
        ${AUDIOMATIC_MINI_APP_SOURCES}
        ${app_icon_res}
    )

    qt_add_resources(AudiomaticMini "audiomatic_resources"
        PREFIX "/"
        FILES
            music.ico
    )

    target_include_directories(AudiomaticMini PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../fluentui3style
        ${CMAKE_CURRENT_SOURCE_DIR}/../../FluentUI3Colors
    )
    target_link_libraries(AudiomaticMini PRIVATE
        AudiomaticMiniWidgets
        ExWidgets::ExWidgets
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::Multimedia
        Qt6::Network
        ${FLUENT_QT_PRIVATE_LIBS}
    )
    if(NOT WIN32)
        if(TARGET FluentUI3Style)
            target_link_libraries(AudiomaticMini PRIVATE FluentUI3Style)
            target_compile_definitions(AudiomaticMini PRIVATE HAS_FLUENTUI3_STYLE_LIB)
        endif()
        set_target_properties(AudiomaticMini PROPERTIES
            BUILD_RPATH "$ORIGIN/../lib"
            INSTALL_RPATH "$ORIGIN/../lib"
        )
    endif()
    if(TARGET Frameless::Frameless)
        target_compile_definitions(AudiomaticMini PRIVATE AUDIOMATIC_ENABLE_FRAMELESS)
        target_link_libraries(AudiomaticMini PRIVATE Frameless::Frameless)
    endif()

    set_target_properties(AudiomaticMini PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
        DEBUG_POSTFIX d
    )

    if(TARGET FluentUI3StylePlugin)
        add_dependencies(AudiomaticMini FluentUI3StylePlugin)
        add_custom_command(TARGET AudiomaticMini POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:AudiomaticMini>/plugins/styles"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:FluentUI3StylePlugin>"
                "$<TARGET_FILE_DIR:AudiomaticMini>/plugins/styles"
            VERBATIM)
    endif()

    qt_finalize_executable(AudiomaticMini)
endif()

if(BUILD_EXAMPLES)
    qt_add_executable(SpectrumDemo
        MANUAL_FINALIZATION
        spectrumdemomain.cpp
        spectrumdemowindow.cpp
        spectrumdemowindow.h
        sinewavegenerator.cpp
        sinewavegenerator.h
    )

    target_link_libraries(SpectrumDemo PRIVATE
        ExWidgets::ExWidgets
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )

    set_target_properties(SpectrumDemo PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
        DEBUG_POSTFIX d
    )

    qt_finalize_executable(SpectrumDemo)
endif()
