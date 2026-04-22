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

    qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
    qputenv("QSG_INFO", "1");
    //qputenv("QT_WIDGETS_HIGHDPI_DOWNSCALE", "1");
    //qputenv("QT_WIDGETS_RHI", "1");
    //qputenv("QSG_RHI_BACKEND", "d3d12");
    //qputenv("QSG_RHI_HDR", "scrgb");
    //qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");

    QApplication a(argc, argv);

    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily("Microsoft YaHei");
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);

    MainWindow w;
    w.show();

//FLUENT_USE_QT_STYLE只是在本项目中为了“qApp->setStyle("FluentUI3");”
//和“fluentUIAppearance.initialize();”方式的切换，其他项目中可以根据需要选择使用哪种方式

#ifdef FLUENT_USE_QT_STYLE
    qDebug()<< QStyleFactory::keys();

#if QT_VERSION > QT_VERSION_CHECK(6, 8, 0)
    qApp->styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#else
    qApp->setProperty("_q_colorscheme", 0);             //控制主题，默认0-Light, 1-Dark, 2-System
#endif
    qApp->setProperty("_q_scrollHint_center", false);   //控制QComboBox弹出位置，默认false，true则在QComboBox中心位置弹出
    qApp->setProperty("_q_themestyle", 0);              //控制配色方案，默认0-Fluent, 1-Teams
    qApp->setStyle("FluentUI3");
#else
    fluentUIAppearance.initialize();
#endif

#ifndef FLUENT_USE_QT_STYLE
    fluentUIAppearance.setMainWindow(&w);            //动态切换标题栏颜色建议重启软件
#endif


    return a.exec();
}

