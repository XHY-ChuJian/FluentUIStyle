#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QStyleFactory>

#include "playerwindow.h"
#include "spectrumdemowindow.h"

namespace {

void setupBundledQtPlugins()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QCoreApplication::addLibraryPath(appDir);
    QCoreApplication::addLibraryPath(QDir(appDir).filePath(QStringLiteral("plugins")));
    QCoreApplication::addLibraryPath(QDir(appDir).filePath(QStringLiteral("../plugins")));
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Window11Style"));
    QCoreApplication::setApplicationName(QStringLiteral("AudiomaticMini"));

    setupBundledQtPlugins();

    const bool demoMode = app.arguments().contains(QStringLiteral("--demo"));

    qDebug() << QStyleFactory::keys();
    app.setProperty("_q_scrollHint_center", false);
    app.setProperty("_q_themestyle", 0);
    app.setProperty("_q_colorscheme", 1);
    if (!demoMode)
    {
        app.setStyle(QStringLiteral("FluentUI3"));
    }

    QFont font = app.font();
    font.setPixelSize(13);
    font.setFamily(QStringLiteral("微软雅黑"));
    font.setHintingPreference(QFont::PreferNoHinting);
    app.setFont(font);

    if (demoMode)
    {
        SpectrumDemoWindow window;
        window.show();
        return app.exec();
    }

    PlayerWindow window;
    window.show();

    return app.exec();
}
