#pragma once

#include <QMainWindow>

class QButtonGroup;
class QColor;
class QCloseEvent;
class ExInfoBar;
class ExProgressRing;
class ExRadialGauge;
class QKeyEvent;
class QLabel;
class QProgressBar;
class QShowEvent;
class QStackedWidget;
class QTimer;
class QToolButton;
class QVBoxLayout;
class QWidget;

class AndroidGalleryWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit AndroidGalleryWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QWidget *createShell();
    QWidget *createHomePage();
    QWidget *createControlsPage();
    QWidget *createSystemMonitorPage();
    QWidget *createAndroidPage();
    QWidget *createSettingsPage();
    QWidget *createScrollPage(QWidget *content) const;

    void setCurrentPage(int index);
    void applyColorScheme(int colorScheme);
    void applyAccentColor(const QColor &color);
    QString deviceSummary() const;
    void updateSystemMonitor();
    void updateSafeAreaMargins();

    QVBoxLayout *m_rootLayout = nullptr;
    QLabel *m_pageTitle = nullptr;
    QToolButton *m_themeButton = nullptr;
    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    QTimer *m_monitorTimer = nullptr;
    ExRadialGauge *m_memoryGauge = nullptr;
    ExProgressRing *m_batteryRing = nullptr;
    QProgressBar *m_storageProgress = nullptr;
    QProgressBar *m_downloadProgress = nullptr;
    QProgressBar *m_uploadProgress = nullptr;
    QLabel *m_memoryDetails = nullptr;
    QLabel *m_storageDetails = nullptr;
    QLabel *m_downloadDetails = nullptr;
    QLabel *m_uploadDetails = nullptr;
    QLabel *m_batteryDetails = nullptr;
    ExInfoBar *m_monitorStatus = nullptr;
    qint64 m_lastReceivedBytes = -1;
    qint64 m_lastTransmittedBytes = -1;
    qint64 m_lastMonitorSampleMs = 0;
    bool m_monitorAutoRefresh = true;
    bool m_safeAreaConnected = false;
    bool m_darkTheme = false;
};
