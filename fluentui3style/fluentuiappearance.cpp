#include "fluentuiappearance.h"

#include "fluentui3style.h"
#include "qapplication.h"
#include "palettemanager.h"
#include "qdebug.h"
#include "qsettings.h"
#include <dwmapi.h>

#include <QStyleHints>
#include <QPalette>

#pragma comment(lib, "dwmapi.lib")

class FluentUIAppearancePrivate
{
public:
    void applyTheme();
    void updateTitleBar();
    bool isSystemDark();

    Theme theme{Theme::Dark};
    QWidget* mainWindow{nullptr};
};

FluentUIAppearance::FluentUIAppearance(QObject *parent)
    : QObject{parent}, d_ptr(new FluentUIAppearancePrivate)
{}

FluentUIAppearance &FluentUIAppearance::instance()
{
    static FluentUIAppearance inst;
    return inst;
}

void FluentUIAppearance::initialize()
{
    Q_D(FluentUIAppearance);
    setTheme(d->isSystemDark() ? Theme::Dark : Theme::Light);
}

void FluentUIAppearance::setTheme(Theme theme)
{
    Q_D(FluentUIAppearance);

    qDebug()<< "Setting theme to:" << (theme == Theme::Dark ? "Dark" : "Light");

    d->theme = theme;
    d->applyTheme();
}

Theme FluentUIAppearance::theme() const
{
    Q_D(const FluentUIAppearance);
    return d->theme;
}

void FluentUIAppearance::setMainWindow(QWidget *mainWindow)
{
    Q_D(FluentUIAppearance);
    d->mainWindow = mainWindow;
    d->updateTitleBar();
}

void FluentUIAppearancePrivate::applyTheme()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    auto appPalette = qApp->palette();
    PaletteManager::instance().applyPalette(appPalette, theme == Theme::Dark);
    qApp->setPalette(appPalette);
#else
    qApp->styleHints()->setColorScheme(theme == Theme::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
#endif
    qApp->setStyle(new FluentUI3Style());
    // qApp->setStyle("windows11");
    updateTitleBar();
}

void FluentUIAppearancePrivate::updateTitleBar()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
#ifdef _MSC_VER
    if (mainWindow == nullptr)
        return;

    HWND hwnd = reinterpret_cast<HWND>(mainWindow->winId());
    BOOL darkMode = FALSE;
    if (theme == Theme::Dark)
    {
        darkMode = TRUE;
    }
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
#endif
#endif
}

bool FluentUIAppearancePrivate::isSystemDark()
{
#ifdef Q_OS_WINDOWS
    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    int value = settings.value("AppsUseLightTheme", 1).toInt();

    return value == 0;

#else
    return false;
#endif
}
