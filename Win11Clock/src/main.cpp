#include <QApplication>
#include <QFont>
#include <QGuiApplication>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Win11 Clock"));
    application.setOrganizationName(QStringLiteral("Window11Style"));

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(14);
    font.setHintingPreference(QFont::PreferNoHinting);
    application.setFont(font);

    application.setProperty("_q_scrollHint_center", false); //控制QComboBox弹出位置，默认false，true则在QComboBox中心位置弹出
    application.setProperty("_q_colorscheme", 1);
    application.setProperty("_q_themestyle", 0);
    application.setStyle(QStringLiteral("FluentUI3"));

    MainWindow window;
    window.show();

    return application.exec();
}
