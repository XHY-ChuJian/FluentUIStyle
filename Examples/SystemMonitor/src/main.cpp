#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include "systemmonitorwindow.h"

#ifndef WIN32
#include "fluentui3style.h"
#endif

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    qputenv("QT_SCREEN_SCALE_FACTORS", "1.25");
#endif
#endif

    QApplication application(argc, argv);
    QApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/../plugins");

    application.setApplicationName(QStringLiteral("FluentSysMon"));
    application.setOrganizationName(QStringLiteral("Window11Style"));

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(13);
    font.setHintingPreference(QFont::PreferNoHinting);
    application.setFont(font);

    application.setProperty("_q_scrollHint_center", false);
    application.setProperty("_q_colorscheme", 1); // 默认深色主题
    application.setProperty("_q_themestyle", 0);

#ifdef WIN32
    application.setStyle(QStringLiteral("FluentUI3"));
#else
    application.setStyle(new FluentUI3Style);
#endif

    SystemMonitorWindow window;
    window.show();

    return application.exec();
}
