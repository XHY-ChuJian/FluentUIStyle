#include "applanguage.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QTranslator>

namespace
{
QTranslator *s_appTranslator = nullptr;
constexpr char kOrg[] = "FluentUI3Style";
constexpr char kApp[] = "Example";
constexpr char kUiLangKey[] = "ui_language";

void removeTranslator()
{
    if (!s_appTranslator)
    {
        return;
    }
    qApp->removeTranslator(s_appTranslator);
    delete s_appTranslator;
    s_appTranslator = nullptr;
}

AppUiLanguage preferenceFromString(const QString &v)
{
    if (v == QStringLiteral("en_US"))
    {
        return AppUiLanguage::En_US;
    }
    if (v == QStringLiteral("system"))
    {
        return AppUiLanguage::FollowSystem;
    }
    return AppUiLanguage::Zh_CN;
}

QString stringFromPreference(AppUiLanguage lang)
{
    switch (lang)
    {
    case AppUiLanguage::En_US:
        return QStringLiteral("en_US");
    case AppUiLanguage::FollowSystem:
        return QStringLiteral("system");
    case AppUiLanguage::Zh_CN:
    default:
        return QStringLiteral("zh_CN");
    }
}

AppUiLanguage systemResolvedUiLanguage()
{
    switch (QLocale::system().language())
    {
    case QLocale::Chinese:
        return AppUiLanguage::Zh_CN;
    default:
        return AppUiLanguage::En_US;
    }
}
} // namespace

AppUiLanguage AppLanguage::savedUiLanguage()
{
    QSettings settings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    const QString v = settings.value(QString::fromLatin1(kUiLangKey), QStringLiteral("zh_CN")).toString();
    return preferenceFromString(v);
}

AppUiLanguage AppLanguage::effectiveUiLanguage()
{
    const AppUiLanguage pref = savedUiLanguage();
    if (pref == AppUiLanguage::FollowSystem)
    {
        return systemResolvedUiLanguage();
    }
    return pref;
}

void AppLanguage::saveUiLanguage(AppUiLanguage lang)
{
    QSettings settings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    settings.setValue(QString::fromLatin1(kUiLangKey), stringFromPreference(lang));
    settings.sync();
}

void AppLanguage::applyTranslator(AppUiLanguage lang)
{
    removeTranslator();
    if (lang != AppUiLanguage::En_US)
    {
        return;
    }
    s_appTranslator = new QTranslator(qApp);
    if (!s_appTranslator->load(QStringLiteral(":/translations/Example_en_US")))
    {
        delete s_appTranslator;
        s_appTranslator = nullptr;
        return;
    }
    qApp->installTranslator(s_appTranslator);
}

bool AppLanguage::restartApplication()
{
    QSettings settings(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    settings.sync();

    const QString program = QCoreApplication::applicationFilePath();
    const QStringList arguments = QCoreApplication::arguments().mid(1);
    if (!QProcess::startDetached(program, arguments))
    {
        return false;
    }
    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    return true;
}
