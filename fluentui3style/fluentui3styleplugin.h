#pragma once

#include <QStylePlugin>

class FluentUI3StylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "fluentui3styleplugin.json")

public:
    explicit FluentUI3StylePlugin(QObject *parent = nullptr);
    QStyle *create(const QString &key) override;
};