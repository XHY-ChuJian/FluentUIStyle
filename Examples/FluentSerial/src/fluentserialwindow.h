#pragma once

#include <QMainWindow>

class SerialControlBar;
class MonitorPage;
class QuickCommandsPage;
class WaveformPage;
class ToolboxPage;
class SettingsPage;
class ExStackedWidget;
class ExWinUINavigationView;

#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
class FluentWindowFrame;
#endif

class FluentSerialWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit FluentSerialWindow(QWidget *parent = nullptr);
    ~FluentSerialWindow() override = default;

    void refreshAppearance();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum PageIndex {
        MonitorPageIndex = 0,
        QuickCmdPageIndex,
        WaveformPageIndex,
        ToolboxPageIndex,
        SettingsPageIndex
    };

    void setupUi();
    void setupNavigation();

    SerialControlBar *m_controlBar{nullptr};
    ExWinUINavigationView *m_navigation{nullptr};
    ExStackedWidget *m_pages{nullptr};

    MonitorPage *m_monitorPage{nullptr};
    QuickCommandsPage *m_quickCmdPage{nullptr};
    WaveformPage *m_waveformPage{nullptr};
    ToolboxPage *m_toolboxPage{nullptr};
    SettingsPage *m_settingsPage{nullptr};

#ifdef FLUENTSERIAL_ENABLE_FRAMELESS
    FluentWindowFrame *m_windowFrame{nullptr};
#endif
};
