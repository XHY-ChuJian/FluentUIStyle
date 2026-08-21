#pragma once

#include "systemprovider.h"

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QProgressBar;

class NetworkPage final : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkPage(QWidget *parent = nullptr);

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

private:
    void setupUi();
    void rebuildAdaptersList(const QList<NetworkAdapterInfo> &adapters);

    QLabel *m_downSpeedLabel = nullptr;
    QLabel *m_upSpeedLabel = nullptr;
    QProgressBar *m_downSpeedBar = nullptr;
    QProgressBar *m_upSpeedBar = nullptr;

    QLabel *m_totalRecvLabel = nullptr;
    QLabel *m_totalSentLabel = nullptr;

    QWidget *m_adaptersContainer = nullptr;
    QVBoxLayout *m_adaptersLayout = nullptr;
    int m_lastAdapterCount = -1;
};
