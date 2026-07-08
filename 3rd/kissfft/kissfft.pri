KISSFFT_ROOT = $$PWD

INCLUDEPATH += $$KISSFFT_ROOT

DEFINES += kiss_fft_scalar=float

SOURCES += \
    $$KISSFFT_ROOT/kiss_fft.c \
    $$KISSFFT_ROOT/kiss_fftr.c

win32-msvc {
    kiss_fft.c.kiss_fftr.c {
        QMAKE_CFLAGS += /W3
    }
} else {
    kiss_fft.c.kiss_fftr.c {
        QMAKE_CFLAGS += -w
    }
}
