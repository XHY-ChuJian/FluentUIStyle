#pragma once

#include "systemprovider.h"

#include <QWidget>

class ExTimeline;
class QLabel;

class HardwareSpecsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit HardwareSpecsPage(QWidget *parent = nullptr);

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

private:
    void setupUi();
    void addTimelineEvent(const QString &title, const QString &description, int status);

    ExTimeline *m_timeline = nullptr;
    QLabel *m_osLabel = nullptr;
    QLabel *m_kernelLabel = nullptr;
    QLabel *m_cpuNameLabel = nullptr;
    QLabel *m_cpuArchLabel = nullptr;
    QLabel *m_coreCountLabel = nullptr;
    QLabel *m_bootTimeLabel = nullptr;
    QLabel *m_screenInfoLabel = nullptr;

    bool m_highCpuWarned = false;
    bool m_highMemWarned = false;
};
