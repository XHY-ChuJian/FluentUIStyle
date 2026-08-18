#include <QApplication>
#include <QFont>
#include <QMenu>
#include <QTextEdit>
#include <QAction>
#include <QIcon>
#include <QStyle>
#include <QPainter>

#include "qabstractitemview.h"
#include "qboxlayout.h"
#include "qcombobox.h"
#include "qdebug.h"
#include "qevent.h"
#include "qlineedit.h"
#include <QStyleHints>
#include <QApplication>
#include <QPalette>
#include <QDebug>
#include <QPropertyAnimation>

#include "mainwindow.h"
#include "diagnostics/crashdump.h"
#ifdef GALLERY_ENABLE_I18N
#include "applanguage.h"
#endif
#include "qstylefactory.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    //改成屏幕实际缩放的
    qputenv("QT_SCREEN_SCALE_FACTORS", "1.25");
#endif
#endif

    QApplication a(argc, argv);
    const QString crashDumpDirectory = GalleryCrashDump::install();
    if (!crashDumpDirectory.isEmpty())
    {
        qInfo() << "Crash dumps:" << crashDumpDirectory;
    }

    QApplication::addLibraryPath(QCoreApplication::applicationDirPath() + "/../plugins");

    qDebug() << QStyleFactory::keys();
    qApp->setProperty("_q_scrollHint_center", false); //控制QComboBox弹出位置，默认false，true则在QComboBox中心位置弹出
    qApp->setProperty("_q_themestyle", 0);            //控制配色方案，默认0-Fluent, 1-Teams
    qApp->setStyle("FluentUI3");

#ifdef GALLERY_ENABLE_I18N
    AppLanguage::applyTranslator(AppLanguage::effectiveUiLanguage());
#endif

    QFont font = a.font();
    font.setPixelSize(13);
    font.setFamily("微软雅黑");
    font.setHintingPreference(QFont::PreferNoHinting);
    a.setFont(font);

    MainWindow w;
    w.show();

    return a.exec();
}
