#pragma once

#include <QMainWindow>

class ExStackedWidget;
class ExWinUINavigationView;
class SettingsPage;
#ifdef WIN11CLOCK_ENABLE_FRAMELESS
class FluentWindowFrame;
#endif

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
    void refreshAppearance();

    ExWinUINavigationView* m_navigation{nullptr};
    ExStackedWidget* m_pages{nullptr};
    SettingsPage* m_settingsPage{nullptr};
#ifdef WIN11CLOCK_ENABLE_FRAMELESS
    FluentWindowFrame* m_windowFrame{nullptr};
#endif
};
