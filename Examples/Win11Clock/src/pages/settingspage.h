#pragma once

#include <QWidget>

class QComboBox;

class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    void setDarkTheme(bool dark);

signals:
    void appearanceChanged();

private:
    QComboBox* m_themeCombo{nullptr};
};
