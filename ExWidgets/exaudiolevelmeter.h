#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QColor>
#include <QElapsedTimer>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

// 只读音频电平表。输入每个声道的 dBFS 或线性幅度，控件负责衰减、
// 峰值保持和绘制。setLevel/setLevels/setLinearLevels 可从普通工作线程调用，
// 跨线程调用会自动投递到控件所在线程；硬实时音频回调应先写入无锁状态。
class EXWIDGETS_EXPORT ExAudioLevelMeter final : public QWidget
{
    Q_OBJECT

public:
    enum ScalePosition
    {
        NoScale,
        LeftScale,
        RightScale,
        CenterScale
    };
    Q_ENUM( ScalePosition )

    enum ScaleMode
    {
        IntervalScale,
        FixedTickCount,
        CustomScale
    };
    Q_ENUM( ScaleMode )

    enum ColorMode
    {
        SingleColor,
        ThresholdColors,
        GradientColors
    };
    Q_ENUM( ColorMode )

    EXWIDGETS_DECLARE_PROPERTY( int, channelCount, channelCount, setChannelCount, 2 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimumDecibels, minimumDecibels, setMinimumDecibels, -60.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximumDecibels, maximumDecibels, setMaximumDecibels, 0.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, warningDecibels, warningDecibels, setWarningDecibels, -12.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, clipDecibels, clipDecibels, setClipDecibels, -3.0 )
    EXWIDGETS_DECLARE_PROPERTY( int, segmentCount, segmentCount, setSegmentCount, 30 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, segmentSpacing, segmentSpacing, setSegmentSpacing, 3.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, segmentRadius, segmentRadius, setSegmentRadius, 2.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, channelSpacing, channelSpacing, setChannelSpacing, 8.0 )
    EXWIDGETS_DECLARE_PROPERTY( ScalePosition, scalePosition, scalePosition, setScalePosition, RightScale )
    EXWIDGETS_DECLARE_PROPERTY( ScaleMode, scaleMode, scaleMode, setScaleMode, IntervalScale )
    EXWIDGETS_DECLARE_PROPERTY( qreal, scaleInterval, scaleInterval, setScaleInterval, 10.0 )
    EXWIDGETS_DECLARE_PROPERTY( int, scaleTickCount, scaleTickCount, setScaleTickCount, 7 )
    EXWIDGETS_DECLARE_PROPERTY( QString, scaleUnit, scaleUnit, setScaleUnit, QStringLiteral( "dB" ) )
    EXWIDGETS_DECLARE_PROPERTY( bool, scaleUnitVisible, isScaleUnitVisible, setScaleUnitVisible, false )
    EXWIDGETS_DECLARE_PROPERTY( int, scalePrecision, scalePrecision, setScalePrecision, 0 )
    EXWIDGETS_DECLARE_PROPERTY( bool, scaleTickMarksVisible, areScaleTickMarksVisible, setScaleTickMarksVisible, false )
    EXWIDGETS_DECLARE_PROPERTY( qreal, scaleTickLength, scaleTickLength, setScaleTickLength, 4.0 )
    EXWIDGETS_DECLARE_PROPERTY( bool, channelLabelsVisible, areChannelLabelsVisible, setChannelLabelsVisible, true )
    EXWIDGETS_DECLARE_PROPERTY( bool, peakHoldEnabled, isPeakHoldEnabled, setPeakHoldEnabled, true )
    EXWIDGETS_DECLARE_PROPERTY( int, peakHoldDuration, peakHoldDuration, setPeakHoldDuration, 1000 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, decayRate, decayRate, setDecayRate, 36.0 )
    EXWIDGETS_DECLARE_PROPERTY( qreal, peakDecayRate, peakDecayRate, setPeakDecayRate, 18.0 )
    EXWIDGETS_DECLARE_PROPERTY( int, inputTimeout, inputTimeout, setInputTimeout, 120 )
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )
    EXWIDGETS_DECLARE_PROPERTY( ColorMode, colorMode, colorMode, setColorMode, SingleColor )
    EXWIDGETS_DECLARE_PROPERTY( QColor, backgroundColor, backgroundColor, setBackgroundColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, activeColor, activeColor, setActiveColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, inactiveColor, inactiveColor, setInactiveColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, warningColor, warningColor, setWarningColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, clipColor, clipColor, setClipColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, peakColor, peakColor, setPeakColor, QColor() )
    EXWIDGETS_DECLARE_PROPERTY( QColor, scaleColor, scaleColor, setScaleColor, QColor() )

    Q_PROPERTY( QVector<qreal> customScaleValues READ customScaleValues WRITE setCustomScaleValues NOTIFY customScaleValuesChanged )
    Q_PROPERTY( QStringList channelLabels READ channelLabels WRITE setChannelLabels NOTIFY channelLabelsChanged )
    Q_PROPERTY( bool running READ isRunning NOTIFY runningChanged )

    explicit ExAudioLevelMeter( QWidget* parent = nullptr );

    [[nodiscard]] QStringList channelLabels() const;
    void setChannelLabels( const QStringList& labels );
    [[nodiscard]] QVector<qreal> customScaleValues() const;
    void setCustomScaleValues( const QVector<qreal>& values );

    [[nodiscard]] QVector<qreal> levels() const;
    [[nodiscard]] QVector<qreal> displayedLevels() const;
    [[nodiscard]] QVector<qreal> peakLevels() const;
    [[nodiscard]] qreal level( int channel ) const;
    [[nodiscard]] qreal displayedLevel( int channel ) const;
    [[nodiscard]] qreal peakLevel( int channel ) const;
    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

public Q_SLOTS:
    void setLevel( qreal decibels );
    void setStereoLevels( qreal leftDecibels, qreal rightDecibels );
    void setLevels( const QVector<qreal>& decibels );
    void setLinearLevel( qreal amplitude );
    void setLinearLevels( const QVector<qreal>& amplitudes );
    void resetPeaks();
    void clear();

Q_SIGNALS:
    void channelLabelsChanged( const QStringList& labels );
    void customScaleValuesChanged( const QVector<qreal>& values );
    void levelsChanged( const QVector<qreal>& levels );
    void peakLevelsChanged( const QVector<qreal>& levels );
    void runningChanged( bool running );

protected:
    void paintEvent( QPaintEvent* event ) override;
    void showEvent( QShowEvent* event ) override;
    void hideEvent( QHideEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    void applyLevels( const QVector<qreal>& decibels );
    void updateAnimationState();
    void updateAnimationFrame();
    void resizeLevelStorage();
    [[nodiscard]] qreal boundedLevel( qreal decibels ) const;
    [[nodiscard]] qreal levelRatio( qreal decibels ) const;
    [[nodiscard]] QColor resolvedInactiveColor() const;
    [[nodiscard]] QColor resolvedScaleColor() const;
    [[nodiscard]] QColor colorForLevel( qreal decibels ) const;
    [[nodiscard]] QString resolvedChannelLabel( int channel ) const;
    [[nodiscard]] QVector<qreal> scaleValues() const;
    [[nodiscard]] QString scaleLabel( qreal decibels ) const;

    QTimer* m_animationTimer = nullptr;
    QElapsedTimer m_frameElapsed;
    QElapsedTimer m_inputElapsed;
    QVector<qreal> m_levels;
    QVector<qreal> m_displayedLevels;
    QVector<qreal> m_peakLevels;
    QVector<int> m_peakHoldRemaining;
    QStringList m_channelLabels;
    QVector<qreal> m_customScaleValues;
};
