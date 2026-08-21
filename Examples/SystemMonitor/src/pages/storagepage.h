#pragma once

#include "systemprovider.h"

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QProgressBar;

class StoragePage final : public QWidget
{
    Q_OBJECT

public:
    explicit StoragePage(QWidget *parent = nullptr);

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

private:
    void setupUi();
    void rebuildDiskList(const QList<DiskInfo> &disks);

    QLabel *m_totalSpaceSummaryLabel = nullptr;
    QProgressBar *m_totalDiskBar = nullptr;
    QWidget *m_disksListContainer = nullptr;
    QVBoxLayout *m_disksListLayout = nullptr;

    QStringList m_lastDiskPaths;
};
