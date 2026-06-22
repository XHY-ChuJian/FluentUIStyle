#pragma once

#include <QByteArray>
#include <QColor>
#include <QMutex>
#include <QVector>
#include <QWidget>

extern "C" {
#include "kiss_fft.h"
#include "kiss_fftr.h"
}

/**
 * @brief Fluent 风格的实时音频频谱控件。
 *
 * 接收 16-bit PCM 单声道数据，在内部使用 KissFFT 做 2048 点 FFT，
 * 将结果映射为对数频带的柱状图，并以 Attack/Decay 平滑后绘制。
 */
class SpectrumWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kDefaultBarCount = 64;
    static constexpr int kFftSize = 2048;
    static constexpr int kRefreshIntervalMs = 16; // ~60 FPS
    static constexpr int kBarGapPx = 2;
    static constexpr int kBarRadiusPx = 3;

    explicit SpectrumWidget(QWidget *parent = nullptr);
    ~SpectrumWidget() override;

    /// 追加 16-bit PCM 单声道样本（little-endian int16）。
    void setAudioData(const QByteArray &pcmData);

    /// 设置采样率，用于对数频带映射（默认 44100 Hz）。
    void setSampleRate(int sampleRate);
    int sampleRate() const { return m_sampleRate; }

    void setBarColor(const QColor &color);
    QColor barColor() const { return m_barColor; }

    void setBarCount(int count);
    int barCount() const { return m_barCount; }

    /// 当前平滑后的频带高度（0.0 ~ 1.0）。
    QVector<float> bars() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onRefreshTimer();

private:
    void updateSpectrum();
    void rebuildLogBinMap();
    void applyAttackDecay();
    void syncBarColorFromPalette();

    kiss_fftr_cfg m_fftCfg{nullptr};

    QMutex m_pcmMutex;
    QByteArray m_pendingPcm;

    QVector<float> m_pcmSamples;
    QVector<float> m_fftInput;
    QVector<kiss_fft_cpx> m_fftOutput;

    /// 每个 bar 对应的 FFT bin 范围 [start, end)
    QVector<int> m_binStart;
    QVector<int> m_binEnd;

    /// 原始频带幅值与平滑后的显示值
    QVector<float> m_targets;
    QVector<float> m_bars;

    int m_barCount{kDefaultBarCount};
    int m_sampleRate{44100};
    QColor m_barColor{QColor(96, 205, 255)};
    bool m_useCustomBarColor{false};

    class QTimer *m_timer{nullptr};
};
