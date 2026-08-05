# Qt 6 Gallery reuses AudiomaticMiniWidgets, so AudiomaticMini must be added
# before Gallery even when only BUILD_GALLERY is enabled.
if(BUILD_GALLERY OR BUILD_AUDIOMATIC_MINI)
    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        find_package(Qt6 REQUIRED COMPONENTS Multimedia Network)
        add_subdirectory(AudiomaticMini)
    elseif(BUILD_AUDIOMATIC_MINI)
        message(WARNING "AudiomaticMini requires Qt 6; BUILD_AUDIOMATIC_MINI ignored.")
    endif()
endif()

if(BUILD_GALLERY)
    add_subdirectory(Gallery)
endif()

if(BUILD_WIN11_CLOCK)
    add_subdirectory(Win11Clock)
endif()
