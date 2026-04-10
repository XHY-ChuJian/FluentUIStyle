#include "fluentui3styleplugin.h"
#include "fluentui3style.h"

#include <QtPlugin>

FluentUI3StylePlugin::FluentUI3StylePlugin(QObject *parent)
    : QStylePlugin(parent)
{
}

QStyle *FluentUI3StylePlugin::create(const QString &key)
{
    if (key.compare(QLatin1String("FluentUI3"), Qt::CaseInsensitive) == 0)
        return new FluentUI3Style;
    return nullptr;
}
