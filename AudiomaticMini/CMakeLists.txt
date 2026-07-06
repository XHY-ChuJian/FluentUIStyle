if(QT_VERSION_MAJOR LESS 6)
    message(STATUS "AudiomaticMini: skipped (requires Qt 6 for multimedia playback)")
    return()
endif()

set(AUDIOMATIC_MINI_WIDGET_SOURCES
    audiomaticplayerwidget.cpp
    audiomaticplayerwidget.h
    audiomaticplayerwidget.ui
    audiospectrumanalyzer.cpp
    audiospectrumanalyzer.h
    jamendoclient.cpp
    jamendoclient.h
)

add_library(AudiomaticMiniWidgets STATIC ${AUDIOMATIC_MINI_WIDGET_SOURCES})

target_include_directories(AudiomaticMiniWidgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style
)

target_link_libraries(AudiomaticMiniWidgets PUBLIC
    ExWidgets
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    ${FLUENT_QT_PRIVATE_LIBS}
)

if(BUILD_AUDIOMATIC_MINI)
    set(AUDIOMATIC_MINI_APP_SOURCES
        main.cpp
        playerwindow.cpp
        playerwindow.h
        spectrumdemowindow.cpp
        spectrumdemowindow.h
        sinewavegenerator.cpp
        sinewavegenerator.h
        ../Example/frameless/fluenttitlebar.cpp
        ../Example/frameless/fluenttitlebar.h
        ../Example/frameless/fluentwindowframe.cpp
        ../Example/frameless/fluentwindowframe.h
    )

    qt_add_executable(AudiomaticMini
        MANUAL_FINALIZATION
        ${AUDIOMATIC_MINI_APP_SOURCES}
    )

    target_compile_definitions(AudiomaticMini PRIVATE FLUENT_USE_QT_STYLE)

    target_include_directories(AudiomaticMini PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../Example/frameless
        ${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style
        ${CMAKE_CURRENT_SOURCE_DIR}/../FluentUI3Colors
    )

    target_link_libraries(AudiomaticMini PRIVATE
        AudiomaticMiniWidgets
        ExWidgets
        QWindowKit::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::Multimedia
        Qt6::Network
        ${FLUENT_QT_PRIVATE_LIBS}
    )

    set_target_properties(AudiomaticMini PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
        DEBUG_POSTFIX d
    )

    if(TARGET QWKCore AND TARGET QWKWidgets)
        add_dependencies(AudiomaticMini QWKCore QWKWidgets)
        add_custom_command(TARGET AudiomaticMini POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:QWKCore>"
                "$<TARGET_FILE_DIR:AudiomaticMini>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:QWKWidgets>"
                "$<TARGET_FILE_DIR:AudiomaticMini>"
            VERBATIM)
    endif()

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

option(BUILD_SPECTRUM_DEMO "Build standalone ExSpectrumWidget demo (Qt 6 only)" ON)
if(BUILD_SPECTRUM_DEMO)
    qt_add_executable(SpectrumDemo
        MANUAL_FINALIZATION
        spectrumdemomain.cpp
        spectrumdemowindow.cpp
        spectrumdemowindow.h
        sinewavegenerator.cpp
        sinewavegenerator.h
    )

    target_link_libraries(SpectrumDemo PRIVATE
        ExWidgets
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
