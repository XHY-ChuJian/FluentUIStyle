set(AUDIOMATIC_MINI_WIDGET_SOURCES
    audiomaticplayerwidget.cpp
    audiomaticplayerwidget.h
    audiomaticplayerwidget.ui
    audiospectrumanalyzer.cpp
    audiospectrumanalyzer.h
    spectrumwidget.cpp
    spectrumwidget.h
    sinewavegenerator.cpp
    sinewavegenerator.h
)

add_library(AudiomaticMiniWidgets STATIC ${AUDIOMATIC_MINI_WIDGET_SOURCES})

target_include_directories(AudiomaticMiniWidgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../fluentui3style
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rd/kissfft
)

target_link_libraries(AudiomaticMiniWidgets PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    kissfft
    ${FLUENT_QT_PRIVATE_LIBS}
)

if(BUILD_AUDIOMATIC_MINI)
    set(AUDIOMATIC_MINI_APP_SOURCES
        main.cpp
        playerwindow.cpp
        playerwindow.h
        spectrumdemowindow.cpp
        spectrumdemowindow.h
        ../Example/frameless/fluenttitlebar.cpp
        ../Example/frameless/fluenttitlebar.h
        ../Example/frameless/fluentwindowframe.cpp
        ../Example/frameless/fluentwindowframe.h
    )

    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        qt_add_executable(AudiomaticMini
            MANUAL_FINALIZATION
            ${AUDIOMATIC_MINI_APP_SOURCES}
        )
    else()
        add_executable(AudiomaticMini
            ${AUDIOMATIC_MINI_APP_SOURCES}
        )
    endif()

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
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Multimedia
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

    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        qt_finalize_executable(AudiomaticMini)
    endif()
endif()

# 独立频谱演示程序（正弦波模拟 PCM，无需音频文件）
option(BUILD_SPECTRUM_DEMO "Build standalone SpectrumWidget demo" ON)
if(BUILD_SPECTRUM_DEMO)
    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        qt_add_executable(SpectrumDemo
            MANUAL_FINALIZATION
            spectrumdemomain.cpp
            spectrumdemowindow.cpp
            spectrumdemowindow.h
        )
    else()
        add_executable(SpectrumDemo
            spectrumdemomain.cpp
            spectrumdemowindow.cpp
            spectrumdemowindow.h
        )
    endif()

    target_link_libraries(SpectrumDemo PRIVATE
        AudiomaticMiniWidgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
    )

    set_target_properties(SpectrumDemo PROPERTIES
        WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
        DEBUG_POSTFIX d
    )

    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        qt_finalize_executable(SpectrumDemo)
    endif()
endif()
