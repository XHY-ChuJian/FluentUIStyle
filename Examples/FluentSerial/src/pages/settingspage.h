#pragma once

#include <QComboBox>
#include <QWidget>

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setDarkTheme(bool isDark);

signals:
    void appearanceChanged();

private slots:
    void onThemeChanged(int index);

private:
    void setupUi();

    QComboBox *m_themeCombo{nullptr};
};
