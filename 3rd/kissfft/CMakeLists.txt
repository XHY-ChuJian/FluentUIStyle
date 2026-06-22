# KissFFT - minimal real-FFT build for SpectrumWidget
set(_kissfft_sources kiss_fft.c kiss_fftr.c)

add_library(kissfft STATIC ${_kissfft_sources})

set_source_files_properties(${_kissfft_sources} PROPERTIES
    LANGUAGE C
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTORCC ON
)

set_target_properties(kissfft PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    C_STANDARD 99
)

target_include_directories(kissfft PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(kissfft PRIVATE kiss_fft_scalar=float)

if(MSVC)
    target_compile_options(kissfft PRIVATE /W3)
else()
    target_compile_options(kissfft PRIVATE -w)
endif()
