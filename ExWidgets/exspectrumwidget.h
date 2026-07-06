#pragma once

#include "exwidgets_global.h"

#include <QByteArray>
#include <QColor>
#include <QSize>
#include <QVector>
#include <QWidget>

/**
 * @brief 通用实时音频频谱控件（Fluent 风格）。
 *
 * 接收 16-bit PCM 单声道 little-endian 数据，内部 FFT 后绘制对数频带柱状图。
 *
 * @note 建议在 GUI 线程调用 pushAudioData() / setAudioData()。
 */
class EXWIDGETS_EXPORT ExSpectrumWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged FINAL)
    Q_PROPERTY(int barCount READ barCount WRITE setBarCount NOTIFY barCountChanged FINAL)
    Q_PROPERTY(QColor barColor READ barColor WRITE setBarColor NOTIFY barColorChanged FINAL)
    Q_PROPERTY(int refreshIntervalMs READ refreshIntervalMs WRITE setRefreshIntervalMs NOTIFY refreshIntervalMsChanged FINAL)

public:
    static constexpr int DefaultBarCount = 64;
    static constexpr int DefaultFftSize = 2048;
    static constexpr int DefaultRefreshIntervalMs = 16;

    explicit ExSpectrumWidget(QWidget *parent = nullptr);
    ~ExSpectrumWidget() override;

    /// 追加 mono int16 PCM 样本。
    Q_INVOKABLE void pushAudioData(const QByteArray &pcmData);

    void setSampleRate(int sampleRate);
    int sampleRate() const;

    void setBarCount(int count);
    int barCount() const;

    void setBarColor(const QColor &color);
    QColor barColor() const;

    void setRefreshIntervalMs(int intervalMs);
    int refreshIntervalMs() const;

    /// 当前平滑后的频带高度（0.0 ~ 1.0）。
    QVector<float> bars() const;

    /// 清空缓冲与显示，频带回落至最小高度。
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    void setAudioData(const QByteArray &pcmData);

signals:
    void sampleRateChanged(int sampleRate);
    void barCountChanged(int barCount);
    void barColorChanged(const QColor &color);
    void refreshIntervalMsChanged(int intervalMs);
    void barsChanged(const QVector<float> &bars);

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

    struct Private;
    Private *d;
};
