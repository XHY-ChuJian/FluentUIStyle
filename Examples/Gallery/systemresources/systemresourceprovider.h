#pragma once

#include "systemresourcesnapshot.h"

#include <QObject>

class QTimer;
struct SystemResourceNativeCounters;

class SystemResourceProvider final : public QObject
{
    Q_OBJECT

public:
    explicit SystemResourceProvider( QObject* parent = nullptr );
    ~SystemResourceProvider() override;

    void start( int intervalMilliseconds = 1000 );
    void stop();
    void setInterval( int intervalMilliseconds );
    [[nodiscard]] bool isRunning() const;

Q_SIGNALS:
    void snapshotReady( const SystemResourceSnapshot& snapshot );

private:
    void collect();
    void resetSamplingState();

    QTimer* m_timer = nullptr;
    quint64 m_previousIdleTime = 0;
    quint64 m_previousKernelTime = 0;
    quint64 m_previousUserTime = 0;
    quint64 m_previousReceivedBytes = 0;
    quint64 m_previousSentBytes = 0;
    qint64 m_previousNetworkTimestamp = 0;
    QString m_previousNetworkKey;
    SystemResourceNativeCounters* m_nativeCounters = nullptr;
};
