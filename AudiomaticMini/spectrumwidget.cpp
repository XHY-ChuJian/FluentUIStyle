#include "spectrumwidget.h"

#include <QEvent>
#include <QLinearGradient>
#include <QMutexLocker>
#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>

#include <cstring>

namespace {

constexpr float kDecayFactor = 0.95f;
constexpr float kMinDisplayLevel = 0.02f;
constexpr float kMinFrequencyHz = 60.0f;
constexpr float kMaxFrequencyHz = 16000.0f;

/// 生成 Hann 窗，降低 FFT 频谱泄漏。
QVector<float> buildHannWindow(int size)
{
    QVector<float> window(size);
    if (size <= 1)
    {
        if (!window.isEmpty())
        {
            window[0] = 1.0f;
        }
        return window;
    }

    for (int i = 0; i < size; ++i)
    {
        window[i] = 0.5f * (1.0f - qCos(2.0f * float(M_PI) * float(i) / float(size - 1)));
    }
    return window;
}

} // namespace

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);

    m_targets.fill(kMinDisplayLevel, m_barCount);
    m_bars.fill(kMinDisplayLevel, m_barCount);
    m_fftInput.fill(0.0f, kFftSize);
    m_fftOutput.resize(kFftSize / 2 + 1);

    m_fftCfg = kiss_fftr_alloc(kFftSize, 0, nullptr, nullptr);
    rebuildLogBinMap();
    syncBarColorFromPalette();

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &SpectrumWidget::onRefreshTimer);
    m_timer->start(kRefreshIntervalMs);
}

SpectrumWidget::~SpectrumWidget()
{
    if (m_fftCfg)
    {
        kiss_fftr_free(m_fftCfg);
        m_fftCfg = nullptr;
    }
}

void SpectrumWidget::setAudioData(const QByteArray &pcmData)
{
    if (pcmData.isEmpty())
    {
        return;
    }

    QMutexLocker locker(&m_pcmMutex);
    m_pendingPcm.append(pcmData);
}

void SpectrumWidget::setSampleRate(int sampleRate)
{
    const int clamped = qMax(8000, sampleRate);
    if (m_sampleRate == clamped)
    {
        return;
    }

    m_sampleRate = clamped;
    rebuildLogBinMap();
}

void SpectrumWidget::setBarColor(const QColor &color)
{
    m_useCustomBarColor = true;
    m_barColor = color;
    update();
}

void SpectrumWidget::setBarCount(int count)
{
    const int clamped = qBound(8, count, 256);
    if (m_barCount == clamped)
    {
        return;
    }

    m_barCount = clamped;
    m_targets.fill(kMinDisplayLevel, m_barCount);
    m_bars.fill(kMinDisplayLevel, m_barCount);
    rebuildLogBinMap();
    update();
}

QVector<float> SpectrumWidget::bars() const
{
    return m_bars;
}

void SpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect area = rect();
    painter.fillRect(area, palette().color(QPalette::Window));

    if (m_barCount <= 0 || area.width() <= 0 || area.height() <= 0)
    {
        return;
    }

    const int waterY = area.height() / 2;
    const int totalGap = kBarGapPx * (m_barCount - 1);
    const int barWidth = qMax(1, (area.width() - totalGap) / m_barCount);
    const int usedWidth = barWidth * m_barCount + totalGap;
    const int xOffset = (area.width() - usedWidth) / 2;
    const int maxBarHeight = qMax(8, waterY - 10);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor divider = QColor(palette().color(QPalette::Text).red(),
                                  palette().color(QPalette::Text).green(),
                                  palette().color(QPalette::Text).blue(),
                                  dark ? 20 : 30);

    painter.setPen(Qt::NoPen);

    for (int i = 0; i < m_barCount; ++i)
    {
        const int x = xOffset + i * (barWidth + kBarGapPx);
        const int barHeight = qMax(2, int(m_bars[i] * maxBarHeight));

        // 上半部分：主频谱柱
        const QRectF upperRect(x, waterY - barHeight, barWidth, barHeight);
        painter.setBrush(m_barColor);
        painter.drawRoundedRect(upperRect, kBarRadiusPx, kBarRadiusPx);

        // 下半部分：镜像倒影（渐变淡出）
        const int reflectHeight = qMax(2, int(barHeight * 0.92f));
        const QRectF lowerRect(x, waterY + 1, barWidth, reflectHeight);

        QLinearGradient gradient(x, waterY + 1, x, waterY + 1 + reflectHeight);
        QColor top = m_barColor;
        top.setAlpha(150);
        QColor bottom = m_barColor;
        bottom.setAlpha(0);
        gradient.setColorAt(0.0, top);
        gradient.setColorAt(1.0, bottom);

        painter.setBrush(gradient);
        painter.drawRoundedRect(lowerRect, kBarRadiusPx, kBarRadiusPx);
    }

    // 中线分隔
    painter.setPen(divider);
    painter.drawLine(0, waterY, area.width(), waterY);
}

void SpectrumWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange && !m_useCustomBarColor)
    {
        syncBarColorFromPalette();
    }
}

void SpectrumWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void SpectrumWidget::onRefreshTimer()
{
    updateSpectrum();
    applyAttackDecay();
    update();
}

void SpectrumWidget::updateSpectrum()
{
    if (!m_fftCfg)
    {
        return;
    }

    // 将待处理的 PCM 转为 float 样本并追加到环形缓冲
    QByteArray pcmChunk;
    {
        QMutexLocker locker(&m_pcmMutex);
        if (!m_pendingPcm.isEmpty())
        {
            pcmChunk.swap(m_pendingPcm);
        }
    }

    if (!pcmChunk.isEmpty())
    {
        const int sampleCount = pcmChunk.size() / int(sizeof(qint16));
        const auto *samples = reinterpret_cast<const qint16 *>(pcmChunk.constData());
        m_pcmSamples.reserve(m_pcmSamples.size() + sampleCount);

        for (int i = 0; i < sampleCount; ++i)
        {
            m_pcmSamples.append(float(samples[i]) / 32768.0f);
        }

        // 限制缓冲长度，避免长时间无播放时内存增长
        constexpr int kMaxBufferSamples = kFftSize * 8;
        if (m_pcmSamples.size() > kMaxBufferSamples)
        {
            m_pcmSamples.remove(0, m_pcmSamples.size() - kMaxBufferSamples);
        }
    }

    if (m_pcmSamples.size() < kFftSize)
    {
        // 样本不足时缓慢衰减
        for (int i = 0; i < m_barCount; ++i)
        {
            m_targets[i] = qMax(kMinDisplayLevel, m_targets[i] * kDecayFactor);
        }
        return;
    }

    static const QVector<float> hannWindow = buildHannWindow(kFftSize);

    // 取最新 kFftSize 个样本做 FFT
    const float *recent = m_pcmSamples.constData() + (m_pcmSamples.size() - kFftSize);
    for (int i = 0; i < kFftSize; ++i)
    {
        m_fftInput[i] = recent[i] * hannWindow[i];
    }

    kiss_fftr(m_fftCfg, m_fftInput.constData(), m_fftOutput.data());

    // 只使用前半部分频谱 bin（不含 Nyquist）
    const int maxBin = kFftSize / 2;

    for (int bar = 0; bar < m_barCount; ++bar)
    {
        const int start = qBound(1, m_binStart[bar], maxBin - 1);
        const int end = qBound(start + 1, m_binEnd[bar], maxBin);

        float sum = 0.0f;
        int count = 0;
        for (int bin = start; bin < end; ++bin)
        {
            const float re = m_fftOutput[bin].r;
            const float im = m_fftOutput[bin].i;
            sum += qSqrt(re * re + im * im);
            ++count;
        }

        const float magnitude = count > 0 ? (sum / float(count)) / float(kFftSize) : 0.0f;
        const float normalized = qBound(0.0f, qLn(1.0f + magnitude * 180.0f) / 5.0f, 1.0f);
        m_targets[bar] = qMax(kMinDisplayLevel, normalized);
    }
}

void SpectrumWidget::rebuildLogBinMap()
{
    m_binStart.resize(m_barCount);
    m_binEnd.resize(m_barCount);

    const int maxBin = kFftSize / 2;
    const float nyquist = float(m_sampleRate) * 0.5f;
    const float maxFreq = qMin(kMaxFrequencyHz, nyquist * 0.95f);
    const float logMin = qLn(kMinFrequencyHz);
    const float logMax = qLn(maxFreq);

    for (int bar = 0; bar < m_barCount; ++bar)
    {
        const float t0 = float(bar) / float(m_barCount);
        const float t1 = float(bar + 1) / float(m_barCount);

        const float f0 = qExp(logMin + (logMax - logMin) * t0);
        const float f1 = qExp(logMin + (logMax - logMin) * t1);

        const int bin0 = qBound(1, int(f0 * float(kFftSize) / float(m_sampleRate)), maxBin - 1);
        const int bin1 = qBound(bin0 + 1, int(f1 * float(kFftSize) / float(m_sampleRate)), maxBin);

        m_binStart[bar] = bin0;
        m_binEnd[bar] = bin1;
    }
}

void SpectrumWidget::applyAttackDecay()
{
    bool changed = false;
    for (int i = 0; i < m_barCount; ++i)
    {
        const float target = m_targets[i];
        float value = m_bars[i];

        if (target > value)
        {
            value = target; // Attack：立即跟随上升
        }
        else
        {
            value *= kDecayFactor; // Decay：缓慢下降
            value = qMax(target, value);
        }

        value = qMax(kMinDisplayLevel, qMin(1.0f, value));
        if (qAbs(value - m_bars[i]) > 0.0005f)
        {
            m_bars[i] = value;
            changed = true;
        }
    }

    Q_UNUSED(changed)
}

void SpectrumWidget::syncBarColorFromPalette()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    m_barColor = dark ? QColor(96, 205, 255) : QColor(0, 120, 212);
    update();
}
