#pragma once

#include "systemprovider.h"

#include <QList>
#include <QWidget>

class QLabel;
class QProgressBar;
class QGridLayout;
class QScrollArea;

struct CoreWidget
{
    QLabel *titleLabel = nullptr;
    QLabel *percentLabel = nullptr;
    QProgressBar *progressBar = nullptr;
};

class CpuMemoryPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CpuMemoryPage(QWidget *parent = nullptr);

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

private:
    void setupUi();

    // CPU
    QLabel *m_cpuTotalUsageLabel = nullptr;
    QProgressBar *m_cpuTotalBar = nullptr;
    QLabel *m_cpuSpecLabel = nullptr;
    QWidget *m_coresContainer = nullptr;
    QGridLayout *m_coresLayout = nullptr;
    QList<CoreWidget> m_coreWidgets;

    // 内存
    QLabel *m_physMemUsedLabel = nullptr;
    QLabel *m_physMemTotalLabel = nullptr;
    QProgressBar *m_physMemBar = nullptr;

    QLabel *m_virtMemUsedLabel = nullptr;
    QLabel *m_virtMemTotalLabel = nullptr;
    QProgressBar *m_virtMemBar = nullptr;
};
