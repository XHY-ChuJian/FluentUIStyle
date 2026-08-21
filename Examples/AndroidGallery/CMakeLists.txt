qt_add_executable(AndroidGallery
    MANUAL_FINALIZATION
    main.cpp
    androidintegration.cpp
    androidintegration.h
    androidgallerywindow.cpp
    androidgallerywindow.h
)

target_link_libraries(AndroidGallery PRIVATE
    ExWidgets::ExWidgets
    FluentUI3Style
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    ${FLUENT_QT_PRIVATE_LIBS}
)

set_target_properties(AndroidGallery PROPERTIES
    QT_ANDROID_PACKAGE_NAME "io.github.window11style.androidgallery"
    QT_ANDROID_APP_NAME "Fluent Android Gallery"
    QT_ANDROID_MIN_SDK_VERSION 28
    QT_ANDROID_VERSION_NAME "1.0.0"
    QT_ANDROID_VERSION_CODE 1
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../android"
)

qt_finalize_executable(AndroidGallery)
