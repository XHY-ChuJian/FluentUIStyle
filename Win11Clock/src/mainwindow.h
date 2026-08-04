#pragma once

#include <QMainWindow>

class ExStackedWidget;
class ExWinUINavigationView;
class SettingsPage;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    enum PageIndex
    {
        FocusPageIndex,
        TimerPageIndex,
        AlarmPageIndex,
        StopwatchPageIndex,
        WorldClockPageIndex,
        AccountPageIndex,
        SettingsPageIndex
    };

    void setupUi();
    void setupNavigation();
    void refreshNavigationIcons();

    ExWinUINavigationView* m_navigation{nullptr};
    ExStackedWidget* m_pages{nullptr};
};
