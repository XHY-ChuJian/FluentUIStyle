#pragma once

#include <QWidget>

class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

signals:
    void appearanceChanged();
};
