#include "androidgallerywindow.h"

#include "fluentui3style.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Window11Style"));
    QCoreApplication::setApplicationName(QStringLiteral("AndroidGallery"));

    application.setProperty("_q_scrollHint_center", false);
    application.setProperty("_q_themestyle", 0);
    application.setProperty("_q_colorscheme", 0);
    application.setStyle(new FluentUI3Style);

    QFont font = application.font();
    font.setPixelSize(15);
    application.setFont(font);

    AndroidGalleryWindow window;
    window.showMaximized();

    return application.exec();
}
