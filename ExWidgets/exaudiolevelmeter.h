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

/**
 * \brief 支持单声道、立体声和多声道的只读音频电平表。
 *
 * 控件接收每个声道的 dBFS 或线性峰值幅度，负责分段绘制、回落衰减、
 * 峰值保持、输入超时和刻度显示，不用于调节音量。
 *
 * setLevel()、setStereoLevels()、setLevels()、setLinearLevel() 和
 * setLinearLevels() 可从普通工作线程调用，跨线程输入会自动排队到 GUI
 * 线程。硬实时音频回调不应直接调用 QWidget，也不应在其中分配 QVector；
 * 请先把结果写入原子或无锁状态，再由普通线程提交给本控件。
 */
class EXWIDGETS_EXPORT ExAudioLevelMeter final : public QWidget
{
    Q_OBJECT

public:
    enum ScalePosition
    {
        //! 不绘制刻度。
        NoScale,
        //! 在所有声道左侧绘制刻度。
        LeftScale,
        //! 在所有声道右侧绘制刻度。
        RightScale,
        //! 在两个声道之间绘制刻度；非双声道时按 RightScale 绘制。
        CenterScale
    };
    Q_ENUM( ScalePosition )

    enum ScaleMode
    {
        //! 从 maximumDecibels 开始，按 scaleInterval 生成刻度。
        IntervalScale,
        //! 在量程内均匀生成 scaleTickCount 个刻度，包含两个端点。
        FixedTickCount,
        //! 使用 customScaleValues 中位于当前量程内的数值。
        CustomScale
    };
    Q_ENUM( ScaleMode )

    enum ColorMode
    {
        //! 所有激活分段使用 activeColor。
        SingleColor,
        //! 根据 warningDecibels 和 clipDecibels 切换颜色。
        ThresholdColors,
        //! 在 activeColor、warningColor 和 clipColor 之间连续插值。
        GradientColors
    };
    Q_ENUM( ColorMode )

    //! 声道数，范围为 1～8。setLevels() 输入数量不同时会自动调整。
    EXWIDGETS_DECLARE_PROPERTY( int, channelCount, channelCount, setChannelCount, 2 )
    //! 显示量程下限，范围为 -160 dB～maximumDecibels - 1 dB。
    EXWIDGETS_DECLARE_PROPERTY( qreal, minimumDecibels, minimumDecibels, setMinimumDecibels, -60.0 )
    //! 显示量程上限，范围为 minimumDecibels + 1 dB～24 dB。
    EXWIDGETS_DECLARE_PROPERTY( qreal, maximumDecibels, maximumDecibels, setMaximumDecibels, 0.0 )
    //! 警告颜色起点，限制在 minimumDecibels 与 clipDecibels 之间。
    EXWIDGETS_DECLARE_PROPERTY( qreal, warningDecibels, warningDecibels, setWarningDecibels, -12.0 )
    //! 过载颜色起点，限制在 warningDecibels 与 maximumDecibels 之间。
    EXWIDGETS_DECLARE_PROPERTY( qreal, clipDecibels, clipDecibels, setClipDecibels, -3.0 )
    //! 期望分段数量，范围为 2～120；空间不足时绘制数量会自动减少。
    EXWIDGETS_DECLARE_PROPERTY( int, segmentCount, segmentCount, setSegmentCount, 30 )
    //! 相邻分段间距，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, segmentSpacing, segmentSpacing, setSegmentSpacing, 3.0 )
    //! 分段圆角半径，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, segmentRadius, segmentRadius, setSegmentRadius, 2.0 )
    //! 相邻声道以及声道与刻度之间的间距，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, channelSpacing, channelSpacing, setChannelSpacing, 8.0 )

    //! 刻度的位置。
    EXWIDGETS_DECLARE_PROPERTY( ScalePosition, scalePosition, scalePosition, setScalePosition, RightScale )
    //! 刻度数值的生成方式。
    EXWIDGETS_DECLARE_PROPERTY( ScaleMode, scaleMode, scaleMode, setScaleMode, IntervalScale )
    //! IntervalScale 的刻度间隔，单位为 dB。
    EXWIDGETS_DECLARE_PROPERTY( qreal, scaleInterval, scaleInterval, setScaleInterval, 10.0 )
    //! FixedTickCount 的刻度数量，范围为 2～64。
    EXWIDGETS_DECLARE_PROPERTY( int, scaleTickCount, scaleTickCount, setScaleTickCount, 7 )
    //! 刻度单位文本，例如 dB 或 dBFS。
    EXWIDGETS_DECLARE_PROPERTY( QString, scaleUnit, scaleUnit, setScaleUnit, QStringLiteral( "dB" ) )
    //! 是否把 scaleUnit 附加到每个刻度标签后。
    EXWIDGETS_DECLARE_PROPERTY( bool, scaleUnitVisible, isScaleUnitVisible, setScaleUnitVisible, false )
    //! 刻度标签的小数位数，范围为 0～3。
    EXWIDGETS_DECLARE_PROPERTY( int, scalePrecision, scalePrecision, setScalePrecision, 0 )
    //! 是否在刻度标签旁绘制短刻度线。
    EXWIDGETS_DECLARE_PROPERTY( bool, scaleTickMarksVisible, areScaleTickMarksVisible, setScaleTickMarksVisible, false )
    //! 短刻度线长度，单位为逻辑像素。
    EXWIDGETS_DECLARE_PROPERTY( qreal, scaleTickLength, scaleTickLength, setScaleTickLength, 4.0 )
    //! 是否在声道底部绘制标签。
    EXWIDGETS_DECLARE_PROPERTY( bool, channelLabelsVisible, areChannelLabelsVisible, setChannelLabelsVisible, true )

    //! 是否绘制并保持峰值标记。
    EXWIDGETS_DECLARE_PROPERTY( bool, peakHoldEnabled, isPeakHoldEnabled, setPeakHoldEnabled, true )
    //! 峰值开始回落前的保持时间，单位为毫秒；0 表示立即回落。
    EXWIDGETS_DECLARE_PROPERTY( int, peakHoldDuration, peakHoldDuration, setPeakHoldDuration, 1000 )
    //! 主电平的回落速度，单位为 dB/s；0 表示保持当前显示值。
    EXWIDGETS_DECLARE_PROPERTY( qreal, decayRate, decayRate, setDecayRate, 36.0 )
    //! 峰值标记的回落速度，单位为 dB/s；0 表示峰值不自动回落。
    EXWIDGETS_DECLARE_PROPERTY( qreal, peakDecayRate, peakDecayRate, setPeakDecayRate, 18.0 )
    //! 无新输入达到该时间后把目标电平重置到量程下限，单位为毫秒；0 表示禁用超时。
    EXWIDGETS_DECLARE_PROPERTY( int, inputTimeout, inputTimeout, setInputTimeout, 120 )
    //! 是否启用回落、峰值保持和输入超时动画；关闭后显示值立即跟随输入值。
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )

    //! 激活分段的配色方式。
    EXWIDGETS_DECLARE_PROPERTY( ColorMode, colorMode, colorMode, setColorMode, SingleColor )
    //! 背景颜色；无效 QColor 表示使用当前明暗主题的默认值。
    EXWIDGETS_DECLARE_PROPERTY( QColor, backgroundColor, backgroundColor, setBackgroundColor, QColor() )
    //! 普通激活分段颜色；无效 QColor 表示使用主题默认值。
    EXWIDGETS_DECLARE_PROPERTY( QColor, activeColor, activeColor, setActiveColor, QColor() )
    //! 未激活分段颜色；无效 QColor 表示根据 palette 自动生成。
    EXWIDGETS_DECLARE_PROPERTY( QColor, inactiveColor, inactiveColor, setInactiveColor, QColor() )
    //! 警告分段颜色；无效 QColor 表示使用主题默认值。
    EXWIDGETS_DECLARE_PROPERTY( QColor, warningColor, warningColor, setWarningColor, QColor() )
    //! 过载分段颜色；无效 QColor 表示使用主题默认值。
    EXWIDGETS_DECLARE_PROPERTY( QColor, clipColor, clipColor, setClipColor, QColor() )
    //! 峰值标记颜色；无效 QColor 表示使用主题默认值。
    EXWIDGETS_DECLARE_PROPERTY( QColor, peakColor, peakColor, setPeakColor, QColor() )
    //! 刻度及声道标签颜色；无效 QColor 表示根据 palette 自动生成。
    EXWIDGETS_DECLARE_PROPERTY( QColor, scaleColor, scaleColor, setScaleColor, QColor() )

    //! CustomScale 使用的刻度值；设置时会过滤非有限值、降序排序并去重。
    Q_PROPERTY( QVector<qreal> customScaleValues READ customScaleValues WRITE setCustomScaleValues NOTIFY customScaleValuesChanged )
    //! 各声道标签；缺失项会自动使用 L/R 或从 1 开始的声道编号。
    Q_PROPERTY( QStringList channelLabels READ channelLabels WRITE setChannelLabels NOTIFY channelLabelsChanged )
    //! 动画计时器当前是否运行，不表示音频设备或播放状态。
    Q_PROPERTY( bool running READ isRunning NOTIFY runningChanged )

    //! 构造一个默认双声道电平表。
    explicit ExAudioLevelMeter( QWidget* parent = nullptr );

    //! 返回用户设置的声道标签。
    [[nodiscard]] QStringList channelLabels() const;
    //! 设置声道标签；数量可以少于 channelCount。
    void setChannelLabels( const QStringList& labels );
    //! 返回已排序和去重的自定义刻度值。
    [[nodiscard]] QVector<qreal> customScaleValues() const;
    //! 设置 CustomScale 的刻度值；量程外的值会保留，但绘制时忽略。
    void setCustomScaleValues( const QVector<qreal>& values );

    //! 返回最近一次输入的 dBFS 数值；输入超时后会变为 minimumDecibels。
    [[nodiscard]] QVector<qreal> levels() const;
    //! 返回当前画面使用的衰减后电平。
    [[nodiscard]] QVector<qreal> displayedLevels() const;
    //! 返回当前峰值标记电平。
    [[nodiscard]] QVector<qreal> peakLevels() const;
    //! 返回指定声道的最近输入；索引无效时返回 minimumDecibels。
    [[nodiscard]] qreal level( int channel ) const;
    //! 返回指定声道的当前显示电平；索引无效时返回 minimumDecibels。
    [[nodiscard]] qreal displayedLevel( int channel ) const;
    //! 返回指定声道的峰值电平；索引无效时返回 minimumDecibels。
    [[nodiscard]] qreal peakLevel( int channel ) const;
    //! 返回内部动画计时器是否正在运行。
    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

public Q_SLOTS:
    /**
     * \brief 一次设置显示量程。
     *
     * minimumDecibels 必须小于 maximumDecibels；有效范围为 -160～24 dB，
     * 并且上下限至少相差 1 dB。警告值、过载值和现有电平会自动限制到新量程。
     */
    void setRange( qreal minimumDecibels, qreal maximumDecibels );
    //! 提交一个声道的 dBFS，并自动切换为单声道。
    void setLevel( qreal decibels );
    //! 提交左右声道的 dBFS，并自动切换为双声道。
    void setStereoLevels( qreal leftDecibels, qreal rightDecibels );
    /**
     * \brief 提交各声道的 dBFS。
     *
     * 非空数组会把 channelCount 自动调整为输入数量，最多保留 8 个声道；
     * 空数组等同于 clear()。NaN 和无穷值按 minimumDecibels 处理。
     */
    void setLevels( const QVector<qreal>& decibels );
    //! 提交一个声道的线性峰值幅度，并自动切换为单声道。
    void setLinearLevel( qreal amplitude );
    /**
     * \brief 提交各声道的线性峰值幅度。
     *
     * 输入按 abs(amplitude) 计算，并通过 20 * log10(amplitude) 转换为 dBFS。
     * 典型范围为 0.0～1.0；超过 1.0 的值会按 maximumDecibels 截断。
     */
    void setLinearLevels( const QVector<qreal>& amplitudes );
    //! 把峰值标记重置到当前显示电平，并重新开始保持计时。
    void resetPeaks();
    //! 清除输入、显示值和峰值，并停止不再需要的动画。
    void clear();

Q_SIGNALS:
    //! 用户设置的声道标签发生变化。
    void channelLabelsChanged( const QStringList& labels );
    //! 自定义刻度值发生变化。
    void customScaleValuesChanged( const QVector<qreal>& values );
    //! 最近输入发生变化，包括输入超时、清除、量程钳制或声道数变化。
    void levelsChanged( const QVector<qreal>& levels );
    //! 峰值电平发生变化。
    void peakLevelsChanged( const QVector<qreal>& levels );
    //! 内部动画计时器的运行状态发生变化。
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
