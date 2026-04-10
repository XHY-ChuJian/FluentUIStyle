set(EXAMPLE_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
    appicon.rc
    appicon.ico
)

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

target_link_libraries(Example PRIVATE
    # FluentUI3Style
    ExWidgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

set_target_properties(Example PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    DEBUG_POSTFIX d
)



if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(Example)
endif()