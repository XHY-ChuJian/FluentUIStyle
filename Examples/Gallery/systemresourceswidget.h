#pragma once

#include <QFrame>
#include <QHash>
#include <QString>

class ExMultiProgressRing;
class ExMultiProgressRingItem;
class ExMultiRadialGauge;
class ExMultiRadialGaugeItem;
class ExLiquidGauge;
class ExProgressRing;
class ExRadialGauge;
class ExTimeline;
class QLabel;
class QPushButton;
class QComboBox;
class QHideEvent;
class QShowEvent;
class SystemResourceProvider;
struct SystemResourceSnapshot;

class SystemResourcesWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit SystemResourcesWidget( QWidget* parent = nullptr );

protected:
    void showEvent( QShowEvent* event ) override;
    void hideEvent( QHideEvent* event ) override;

private:
    void applySnapshot( const SystemResourceSnapshot& snapshot );
    void updateNetworkGauge( quint64 receivedBytesPerSecond, quint64 sentBytesPerSecond );
    void synchronizeDiskItems( const SystemResourceSnapshot& snapshot );
    void updateAlert( const QString& key,
                      bool active,
                      const QString& title,
                      const QString& description );
    void appendTimelineEvent( const QString& title,
                              const QString& description,
                              int status );
    [[nodiscard]] int samplingInterval() const;

    SystemResourceProvider* m_provider = nullptr;
    ExRadialGauge* m_cpuGauge = nullptr;
    ExLiquidGauge* m_memoryGauge = nullptr;
    ExMultiRadialGauge* m_networkGauge = nullptr;
    ExMultiRadialGaugeItem* m_receiveItem = nullptr;
    ExMultiRadialGaugeItem* m_sendItem = nullptr;
    ExMultiProgressRing* m_diskRing = nullptr;
    ExProgressRing* m_gpuRing = nullptr;
    ExTimeline* m_timeline = nullptr;
    QLabel* m_cpuDetails = nullptr;
    QLabel* m_memoryDetails = nullptr;
    QLabel* m_networkDetails = nullptr;
    QLabel* m_diskDetails = nullptr;
    QLabel* m_gpuDetails = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QComboBox* m_intervalCombo = nullptr;
    QHash<QString, ExMultiProgressRingItem*> m_diskItems;
    QHash<QString, bool> m_alertStates;
    quint64 m_networkScaleBytesPerSecond = 32 * 1024;
    int m_networkLowTrafficSamples = 0;
    bool m_paused = false;
};
