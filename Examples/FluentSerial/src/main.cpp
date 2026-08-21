#include "fluentserialwindow.h"

#include <QApplication>
#include <QFontDatabase>
#include <QStyle>

#ifndef WIN32
#include "fluentui3style.h"
#endif

int main(int argc, char *argv[]) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)) &&                               \
    (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  QApplication application(argc, argv);
  application.setApplicationName(QStringLiteral("FluentSerial"));
  application.setOrganizationName(QStringLiteral("Window11Style"));
  application.setApplicationVersion(QStringLiteral("1.0.0"));

  // 字体设置
  QFont defaultFont = application.font();
  defaultFont.setFamily(QStringLiteral("Microsoft YaHei UI"));
  defaultFont.setHintingPreference(QFont::PreferNoHinting);
  defaultFont.setPixelSize(13);
  application.setFont(defaultFont);

  // 加载 FluentUI3 样式与默认深色主题
  application.setProperty("_q_colorscheme", 1); // 1: Dark, 0: Light
#ifdef WIN32
  application.setStyle(QStringLiteral("FluentUI3"));
#else
  application.setStyle(new FluentUI3Style);
#endif

  FluentSerialWindow window;
  window.show();

  return application.exec();
}
