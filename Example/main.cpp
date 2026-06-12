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
#ifdef EXAMPLE_ENABLE_I18N
#include "applanguage.h"
#endif
#include "qstylefactory.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication a(argc, argv);
    //FLUENT_USE_QT_STYLE只是在本项目中为了“qApp->setStyle("FluentUI3");”
    //和“fluentUIAppearance.initialize();”方式的切换，其他项目中可以根据需要选择使用哪种方式

#ifdef FLUENT_USE_QT_STYLE
    qDebug() << QStyleFactory::keys();
    qApp->setProperty("_q_scrollHint_center", false); //控制QComboBox弹出位置，默认false，true则在QComboBox中心位置弹出
    qApp->setProperty("_q_themestyle", 0);            //控制配色方案，默认0-Fluent, 1-Teams
    qApp->setStyle("FluentUI3");
#else
    fluentUIAppearance.initialize();
#endif

#ifdef EXAMPLE_ENABLE_I18N
    AppLanguage::applyTranslator(AppLanguage::effectiveUiLanguage());
#endif

    QFont font = a.font();
    font.setPixelSize(13);
    font.setFamily("微软雅黑");
    font.setHintingPreference(QFont::PreferNoHinting);
    a.setFont(font);

    MainWindow w;
    w.show();

#ifndef FLUENT_USE_QT_STYLE
    fluentUIAppearance.setMainWindow(&w); //动态切换标题栏颜色建议重启软件
#endif

    return a.exec();
}
