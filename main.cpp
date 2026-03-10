#include <QApplication>
#include <QFont>
#include <QMenu>
#include <QTextEdit>
#include "qboxlayout.h"
#include "qcombobox.h"
#include "qdebug.h"
#include "qevent.h"
#include "qlineedit.h"
#include <QStyleHints>
#include <QApplication>
#include <QPalette>
#include <QDebug>

#include "mainwindow.h"
#include "fluentui3style.h"
#include "thememanager.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    // qputenv("QT_SCALE_FACTOR", "1.5");
#endif

    QApplication a(argc, argv);

    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily("Microsoft YaHei");
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);

//Qt6.8以下没有手动主题修改，手动修改调色板
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
    QObject::connect(&ThemeManager::instance(), &ThemeManager::sigThemeChanged, [&a]() {
        auto appPalette = a.palette();
        ThemeManager::instance().applyPalette(appPalette);
        a.setPalette(appPalette);
        qApp->setStyle(new FluentUI3Style());
    });
    ThemeManager::instance().setTheme(Theme::Dark);
#else
    a.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    qApp->setStyle(new FluentUI3Style());
    a.setStyle("windows11");
#endif

    MainWindow w;
    w.show();
    return a.exec();
}

//可能不是Qt6.0.0,实测修改
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
//menu popup会有阴影，如果不想要阴影，可以重写contextMenuEvent事件，直接调用exec()方法显示菜单，而不是popup()方法
//已知QLineEdit, QComboBox, QTextEdit
void QLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        // menu->popup(event->globalPos());
        menu->exec(event->globalPos());
    }
}

void QTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    if (QMenu *menu = createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);
        // menu->popup(event->globalPos());
        menu->exec(e->globalPos());
    }
}

void QComboBox::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QComboBox);
    if (lineEdit()) {
        if (QMenu *menu = lineEdit()->createStandardContextMenu()) {
            menu->setAttribute(Qt::WA_DeleteOnClose);
            // menu->popup(event->globalPos());
            menu->exec(e->globalPos());
        }
    }
}
#endif
