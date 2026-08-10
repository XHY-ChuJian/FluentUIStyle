include($$PWD/../3rd/kissfft/kissfft.pri)

SOURCES += \
    $$PWD/exborderbeam.cpp \
    $$PWD/exborderbeambutton.cpp \
    $$PWD/exaudiolevelmeter.cpp \
    $$PWD/colorgradientslider.cpp \
    $$PWD/exrangeslider.cpp \
    $$PWD/excolorpicker.cpp \
    $$PWD/excolorpickerbutton.cpp \
    $$PWD/excombobox.cpp \
    $$PWD/exmessagebox.cpp \
    $$PWD/extimerdial.cpp \
    $$PWD/extimeline.cpp \
    $$PWD/exliquidgauge.cpp \
    $$PWD/exmultiprogressring.cpp \
    $$PWD/exmultiradialgauge.cpp \
    $$PWD/exprogressring.cpp \
    $$PWD/exradialgauge.cpp \
    $$PWD/exnavtreewidget.cpp \
    $$PWD/exwinuinavigationview.cpp \
    $$PWD/exstackedwidget.cpp \
    $$PWD/extabwidget.cpp \
    $$PWD/excontentdialog.cpp \
    $$PWD/exspectrumwidget.cpp

HEADERS += \
    $$PWD/exwidgetsmacros.h \
    $$PWD/exborderbeam.h \
    $$PWD/exborderbeambutton.h \
    $$PWD/exaudiolevelmeter.h \
    $$PWD/colorgradientslider.h \
    $$PWD/exrangeslider.h \
    $$PWD/excolorpicker.h \
    $$PWD/excolorpickerbutton.h \
    $$PWD/excombobox.h \
    $$PWD/exmessagebox.h \
    $$PWD/extimerdial.h \
    $$PWD/extimeline.h \
    $$PWD/exliquidgauge.h \
    $$PWD/exmultiprogressring.h \
    $$PWD/exmultiradialgauge.h \
    $$PWD/exprogressring.h \
    $$PWD/exradialgauge.h \
    $$PWD/exnavtreewidget.h \
    $$PWD/exwinuinavigationview.h \
    $$PWD/exstackedwidget.h \
    $$PWD/exwidgets_global.h \
    $$PWD/extabwidget.h \
    $$PWD/excontentdialog.h \
    $$PWD/exspectrumwidget.h

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../FluentUI3Colors
INCLUDEPATH += $$PWD/../fluentui3style
