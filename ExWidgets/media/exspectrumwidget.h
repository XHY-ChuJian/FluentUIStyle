#pragma once

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

#include <QByteArray>
#include <QColor>
#include <QSize>
#include <QVector>
#include <QWidget>

/**
 * @brief 通用工业级实时音频频谱可视化控件（WinUI 3 / Fluent 风格）。
 *
 * ============================================================================
 * 【设计哲学与职责边界】
 * ============================================================================
 * 1. 通用性与低耦合：
 *    - 本控件为纯粹的通用媒体展示组件，不绑定任何特定业务（如特定播放器或采集源）；
 *    - 外部接口保持极简契约：外界仅需通过 pushAudioData() 持续输入原始的 16-bit PCM 二进制流，
 *      无需外界进行任何傅里叶变换（FFT）、频带分割或动力学滤波。
 *    - 内部具备独立的线程安全互斥锁缓冲队列，支持从后台音频工作线程直接并发推送。
 *
 * ============================================================================
 * 【DSP 信号处理管线 (Signal Processing Pipeline)】
 * ============================================================================
 * 1. 声道自适应与归一化：
 *    - 自动根据 channelCount 解析输入数据，支持多声道硬件格式按样本平均下混为单声道，
 *      并归一化为 [-1.0, 1.0] 浮点环形滑动窗口（最大限制 8 倍 FFT 窗口，杜绝内存膨胀）。
 * 2. 频域分析 (STFT)：
 *    - 采用 2048 点加汉宁窗（Hann Window）抑制频谱泄漏，通过 KissFFT 执行实数快速傅里叶变换。
 * 3. 连续频率插值重采样 (Continuous Magnitude Resampling)：
 *    - 针对低频窄带（60~150Hz）在离散 FFT 中仅占 2~3 个 Bin 的声学物理缺陷，
 *      创新性地采用连续浮点 Bin 线性插值与加权滤波，彻底杜绝音调变调时由于整数截断导致的跳闪、抽动与台阶感。
 * 4. 听觉等响度与频倾补偿 (Tilt Gain)：
 *    - 引入 +3.0 dB/octave 的自然能量滚降与人耳等响度补偿曲线（最高限幅 +9dB），让低音厚重扎实、高音挺拔清脆。
 * 5. 声学分贝标尺 (dBFS Logarithmic Scale)：
 *    - 放弃机械的自然对数，改用真实声学分贝标尺（动态范围 -54 dBFS ~ -3 dBFS），
 *      精准契合人耳对音量感知的韦伯-费希纳定律（Fechner's Law），弱音灵敏、强音震撼。
 *
 * ============================================================================
 * 【动力学物理引擎 (Physics Dynamics Engine)】
 * ============================================================================
 * 1. 突破传统一阶指数衰减（* 0.93）所导致的“上升无力、底部发粘”缺陷，构建现实世界物理力学模型：
 * 2. 瞬态爆发冲刺 (Attack)：
 *    - 遇到音频重击/鼓点冲击时，以极快的瞬态（~30ms）瞬间弹起，跟手度与打击感拉满；
 * 3. 重力自由落体 (Gravity Drop)：
 *    - 音频能量回落时，受重力加速度 g（gravity 属性可调，默认 4.2）自由落体加速下落，
 *      干脆利落地砸落到底部，杜绝拖泥带水的发粘感；
 * 4. 浮动峰值帽系统 (Floating Peak Cap)：
 *    - 柱顶配有高亮浮动细胶囊横条；被柱体顶飞后在最高点短暂停留（peakCapHoldMs，默认 200ms），
 *      随后以柔和的二次曲线优雅飘落，形成贯穿全频段的视觉追踪焦点。
 *
 * ============================================================================
 * 【渲染管线 (Rendering)】
 * ============================================================================
 * 1. Fluent 现代化抗锯齿圆角柱体绘制；
 * 2. 支持水波倒影（reflectionEnabled 与 reflectionOpacity 动态透明度渐变）及峰值帽镜像；
 * 3. 强调色系统（accentColor）：支持直接定制，或默认动态跟随系统的 Accent / Highlight 配色；
 * 4. 基于真实时间间隔 dt 自适应动力学步长，完美支持 60Hz 到 144Hz 高刷屏。
 */
class EXWIDGETS_EXPORT ExSpectrumWidget : public QWidget
{
    Q_OBJECT

public:
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, sampleRate, sampleRate, setSampleRate, sampleRateChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, channelCount, channelCount, setChannelCount, channelCountChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, barCount, barCount, setBarCount, barCountChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, refreshIntervalMs, refreshIntervalMs, setRefreshIntervalMs, refreshIntervalMsChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(bool, reflectionEnabled, isReflectionEnabled, setReflectionEnabled, reflectionEnabledChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(float, reflectionOpacity, reflectionOpacity, setReflectionOpacity, reflectionOpacityChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, barGap, barGap, setBarGap, barGapChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, barRadius, barRadius, setBarRadius, barRadiusChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(float, minFrequency, minFrequency, setMinFrequency, minFrequencyChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(float, maxFrequency, maxFrequency, setMaxFrequency, maxFrequencyChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(QColor, accentColor, accentColor, setAccentColor, accentColorChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(bool, peakCapEnabled, isPeakCapEnabled, setPeakCapEnabled, peakCapEnabledChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, peakCapHoldMs, peakCapHoldMs, setPeakCapHoldMs, peakCapHoldMsChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(int, peakCapHeight, peakCapHeight, setPeakCapHeight, peakCapHeightChanged)
    EXWIDGETS_DECLARE_PROPERTY_D_NOTIFY(float, gravity, gravity, setGravity, gravityChanged)

    static constexpr int DefaultBarCount = 64;
    static constexpr int DefaultFftSize = 2048;
    static constexpr int DefaultRefreshIntervalMs = 16;
    static constexpr int DefaultChannelCount = 1;
    static constexpr int DefaultBarGap = 2;
    static constexpr int DefaultBarRadius = 3;
    static constexpr float DefaultMinFrequency = 60.0f;
    static constexpr float DefaultMaxFrequency = 16000.0f;
    static constexpr float DefaultReflectionOpacity = 0.58f;
    static constexpr int DefaultPeakCapHoldMs = 200;
    static constexpr int DefaultPeakCapHeight = 2;
    static constexpr float DefaultGravity = 4.2f;

    explicit ExSpectrumWidget(QWidget *parent = nullptr);
    ~ExSpectrumWidget() override;

    /// 追加 mono/stereo int16 PCM 样本（按当前 channelCount 解析；多声道自动平均下混）。
    Q_INVOKABLE void pushAudioData(const QByteArray &pcmData);
    /// 追加指定声道的 int16 PCM 样本（若 channels > 0 且与当前不同，会自动更新 channelCount）。
    Q_INVOKABLE void pushAudioData(const QByteArray &pcmData, int channels);

    /// 当前平滑后的频带高度（0.0 ~ 1.0）。
    QVector<float> bars() const;
    /// 当前浮动峰值帽高度（0.0 ~ 1.0）。
    QVector<float> peakCaps() const;

    /// 清空缓冲与显示，频带回落至最小高度。
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    void setAudioData(const QByteArray &pcmData);

signals:
    void barsChanged(const QVector<float> &bars);
    void peakCapsChanged(const QVector<float> &peakCaps);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onRefreshTimer();

private:
    void updateSpectrum();
    void rebuildLogBinMap();
    void stepPhysics(float dt);

    struct Private;
    Private *d;
};
