#include <QApplication>
#include <QFont>
#include <QMenu>
#include <QTextEdit>
#include "fluentuiappearance.h"
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
#include "qtoolbar.h"

int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication a(argc, argv);

    // qApp->setEffectEnabled(Qt::UI_AnimateCombo, false);

    QFont font = qApp->font();
    font.setPixelSize(13);
    font.setFamily("Microsoft YaHei");
    font.setHintingPreference(QFont::PreferNoHinting);
    qApp->setFont(font);

    fluentUIAppearance.initialize();

    MainWindow w;
    //动态切换标题栏颜色建议重启软件
    // fluentUIAppearance.setMainWindow(&w);

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
