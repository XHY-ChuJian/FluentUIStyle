#include <QApplication>

#include "spectrumdemowindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setProperty("_q_colorscheme", 1);

    SpectrumDemoWindow window;
    window.show();

    return app.exec();
}
