#include <QApplication>
#include <QFont>
#include <QMenu>
#include <QTextEdit>
#include <QAction>
#include <QIcon>
#include <QStyle>
#include <QPainter>

#ifndef FLUENT_USE_QT_STYLE
#include "fluentui3style.h"
#include "fluentuiappearance.h"
#endif

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
#include "qstylefactory.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication a(argc, argv);

    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily("Microsoft YaHei");
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);

#ifdef FLUENT_USE_QT_STYLE
    qDebug()<< QStyleFactory::keys();

#if QT_VERSION > QT_VERSION_CHECK(6, 8, 0)
    qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#else
    qApp->setProperty("_q_colorscheme", 1);
#endif
    qApp->setProperty("_q_scrollHint_center", true);
    qApp->setProperty("_q_themestyle", 0);
    qApp->setStyle("FluentUI3");
#else
    fluentUIAppearance.initialize();
#endif

    MainWindow w;
    //动态切换标题栏颜色建议重启软件
    // fluentUIAppearance.setMainWindow(&w);

    w.show();

    return a.exec();
}

