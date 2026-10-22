#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QIcon>
#include <QStyleFactory>

#include "crashdump.h"
#include "playerwindow.h"
#include "spectrumdemowindow.h"

#ifdef HAS_FLUENTUI3_STYLE_LIB
#include "fluentui3style.h"
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {

void setupBundledQtPlugins()
{
    const QString appDir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_WIN)
    SetDllDirectoryW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(appDir).utf16()));
#endif
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

    const QString crashDumpDirectory = AudiomaticCrashDump::install();
    if (!crashDumpDirectory.isEmpty())
    {
        qInfo() << "Crash dumps:" << crashDumpDirectory;
    }
    app.setWindowIcon(QIcon(QStringLiteral(":/music.ico")));

    setupBundledQtPlugins();

    const bool demoMode = app.arguments().contains(QStringLiteral("--demo"));

    qDebug() << QStyleFactory::keys();
    app.setProperty("_q_scrollHint_center", false);
    app.setProperty("_q_themestyle", 0);
    app.setProperty("_q_colorscheme", 1);
    if (!demoMode)
    {
#ifdef Q_OS_WIN
        app.setStyle(QStringLiteral("FluentUI3"));
#elif defined(HAS_FLUENTUI3_STYLE_LIB)
        app.setStyle(new FluentUI3Style);
#endif
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
