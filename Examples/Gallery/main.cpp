#include <QAction>
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QTextEdit>

#ifndef WIN32
#include "fluentui3style.h"
#endif

#include "qabstractitemview.h"
#include "qboxlayout.h"
#include "qcombobox.h"
#include "qdebug.h"
#include "qevent.h"
#include "qlineedit.h"
#include <QApplication>
#include <QDebug>
#include <QPalette>
#include <QPropertyAnimation>
#include <QStyleHints>


#include "diagnostics/crashdump.h"
#include "mainwindow.h"

#ifdef GALLERY_ENABLE_I18N
#include "applanguage.h"
#endif
#include "qstylefactory.h"

int main(int argc, char *argv[]) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
  // 改成屏幕实际缩放的
  qputenv("QT_SCREEN_SCALE_FACTORS", "1.25");
#endif
#endif

  QApplication a(argc, argv);
  const QString crashDumpDirectory = GalleryCrashDump::install();
  if (!crashDumpDirectory.isEmpty()) {
    qInfo() << "Crash dumps:" << crashDumpDirectory;
  }

  qDebug() << QStyleFactory::keys();
  // qApp->setProperty("secondLevelRoundingRadius", 0);
  qApp->setProperty(
      "_q_scrollHint_center",
      false); // 控制QComboBox弹出位置，默认false，true则在QComboBox中心位置弹出
  qApp->setProperty("_q_themestyle", 0); // 控制配色方案，默认0-Fluent, 1-Teams

#ifdef Q_OS_ANDROID
  // 全局禁用所有 QComboBox 的展开动画
  qApp->setProperty("comboBoxPopupDropDownAnimationEnabled", false);
#endif

#ifdef WIN32
  qApp->setStyle("FluentUI3");
#else
  qApp->setStyle(new FluentUI3Style);
#endif

#ifdef GALLERY_ENABLE_I18N
  AppLanguage::applyTranslator(AppLanguage::effectiveUiLanguage());
#endif

  QFont font = a.font();
  font.setPixelSize(13);
  font.setFamily("微软雅黑");
  font.setHintingPreference(QFont::PreferNoHinting);
  a.setFont(font);

  MainWindow w;
#ifdef Q_OS_ANDROID
  w.showMaximized();
#else
  w.show();
#endif

  return a.exec();
}
