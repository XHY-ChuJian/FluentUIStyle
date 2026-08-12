# Qt 6 Gallery reuses AudiomaticMiniWidgets, so AudiomaticMini must be added
# before Gallery. Qt 5 skips the examples that require Qt 6 Multimedia.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Multimedia Network)
    add_subdirectory(AudiomaticMini)
else()
    message(STATUS "AudiomaticMini and SpectrumDemo skipped: Qt 6 is required.")
endif()

add_subdirectory(Gallery)
add_subdirectory(Win11Clock)
