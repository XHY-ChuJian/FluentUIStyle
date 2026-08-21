#pragma once

#include <QMainWindow>

class ExStackedWidget;
class ExWinUINavigationView;
class OverviewPage;
class CpuMemoryPage;
class StoragePage;
class NetworkPage;
class ProcessesPage;
class HardwareSpecsPage;
class SettingsPage;
class MiniCapsuleWindow;

#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
class FluentWindowFrame;
#endif

class SystemMonitorWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit SystemMonitorWindow(QWidget *parent = nullptr);
    ~SystemMonitorWindow() override;

public slots:
    void switchToMiniCapsule();
    void restoreFromMiniCapsule();

private:
    enum PageIndex
    {
        OverviewPageIndex = 0,
        CpuMemoryPageIndex,
        StoragePageIndex,
        NetworkPageIndex,
        ProcessesPageIndex,
        HardwareSpecsPageIndex,
        SettingsPageIndex
    };

    void setupUi();
    void setupNavigation();
    void refreshAppearance();

    ExWinUINavigationView *m_navigation = nullptr;
    ExStackedWidget *m_pages = nullptr;

    OverviewPage *m_overviewPage = nullptr;
    CpuMemoryPage *m_cpuMemoryPage = nullptr;
    StoragePage *m_storagePage = nullptr;
    NetworkPage *m_networkPage = nullptr;
    ProcessesPage *m_processesPage = nullptr;
    HardwareSpecsPage *m_hardwareSpecsPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;

    MiniCapsuleWindow *m_miniCapsule = nullptr;

#ifdef SYSTEMMONITOR_ENABLE_FRAMELESS
    FluentWindowFrame *m_windowFrame = nullptr;
#endif
};
