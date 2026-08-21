#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QLabel;

class SettingsPage final : public QWidget
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

    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_intervalCombo = nullptr;
    QCheckBox *m_autoCapsuleCheck = nullptr;
};
