#pragma once

#include "systemprovider.h"

#include <QWidget>

class ExRadialGauge;
class ExLiquidGauge;
class ExMultiProgressRing;
class QLabel;
class QPushButton;
class QComboBox;
class QProgressBar;

class OverviewPage final : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);

signals:
    void openMiniCapsuleRequested();

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

private:
    void setupUi();

    // 控件引用
    ExRadialGauge *m_cpuGauge = nullptr;
    QLabel *m_cpuSummaryLabel = nullptr;

    ExLiquidGauge *m_memoryLiquidGauge = nullptr;
    QLabel *m_memorySummaryLabel = nullptr;

    ExMultiProgressRing *m_diskMultiRing = nullptr;
    QLabel *m_diskSummaryLabel = nullptr;

    QLabel *m_downloadSpeedLabel = nullptr;
    QLabel *m_uploadSpeedLabel = nullptr;
    QLabel *m_netSummaryLabel = nullptr;
    QProgressBar *m_downSpeedBar = nullptr;
    QProgressBar *m_upSpeedBar = nullptr;

    QLabel *m_osInfoLabel = nullptr;
    QLabel *m_cpuModelLabel = nullptr;
    QLabel *m_uptimeLabel = nullptr;
    QLabel *m_hostUserLabel = nullptr;

    QComboBox *m_intervalCombo = nullptr;
    QPushButton *m_miniCapsuleBtn = nullptr;
};
