#include "exspectrumwidget.h"

#include <QEvent>
#include <QLinearGradient>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>

#include <cstring>

extern "C" {
#include "kiss_fft.h"
#include "kiss_fftr.h"
}

namespace {

constexpr float kAttackFactor = 0.32f;
constexpr float kDecayFactor = 0.93f;
constexpr float kMinDisplayLevel = 0.02f;
constexpr float kMinFrequencyHz = 60.0f;
constexpr float kMaxFrequencyHz = 16000.0f;
constexpr int kBarGapPx = 2;
constexpr int kBarRadiusPx = 3;

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

struct ExSpectrumWidget::Private
{
    kiss_fftr_cfg fftCfg{nullptr};

    QMutex pcmMutex;
    QByteArray pendingPcm;

    QVector<float> pcmSamples;
    QVector<float> fftInput;
    QVector<kiss_fft_cpx> fftOutput;
    QVector<float> window;
    float windowSum{1.0f};

    QVector<int> binStart;
    QVector<int> binEnd;
    QVector<float> bandCenterHz;

    QVector<float> targets;
    QVector<float> bars;

    int barCount{ExSpectrumWidget::DefaultBarCount};
    int sampleRate{44100};
    int refreshIntervalMs{ExSpectrumWidget::DefaultRefreshIntervalMs};
    QColor barColor{QColor(96, 205, 255)};
    bool useCustomBarColor{false};

    QTimer *timer{nullptr};
};

ExSpectrumWidget::ExSpectrumWidget(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);

    d->targets.fill(kMinDisplayLevel, d->barCount);
    d->bars.fill(kMinDisplayLevel, d->barCount);
    d->fftInput.fill(0.0f, DefaultFftSize);
    d->fftOutput.resize(DefaultFftSize / 2 + 1);
    d->window = buildHannWindow(DefaultFftSize);
    d->windowSum = 0.0f;
    for (float w : d->window)
    {
        d->windowSum += w;
    }
    if (d->windowSum <= 0.0f)
    {
        d->windowSum = 1.0f;
    }

    d->fftCfg = kiss_fftr_alloc(DefaultFftSize, 0, nullptr, nullptr);

    d->timer = new QTimer(this);
    d->timer->setTimerType(Qt::PreciseTimer);
    connect(d->timer, &QTimer::timeout, this, &ExSpectrumWidget::onRefreshTimer);
    d->timer->start(d->refreshIntervalMs);

    rebuildLogBinMap();
    syncBarColorFromPalette();
}

ExSpectrumWidget::~ExSpectrumWidget()
{
    if (d->fftCfg)
    {
        kiss_fftr_free(d->fftCfg);
        d->fftCfg = nullptr;
    }
    delete d;
}

void ExSpectrumWidget::pushAudioData(const QByteArray &pcmData)
{
    if (pcmData.isEmpty())
    {
        return;
    }

    QMutexLocker locker(&d->pcmMutex);
    d->pendingPcm.append(pcmData);
}

void ExSpectrumWidget::setAudioData(const QByteArray &pcmData)
{
    pushAudioData(pcmData);
}

void ExSpectrumWidget::setSampleRate(int sampleRate)
{
    const int clamped = qMax(8000, sampleRate);
    if (d->sampleRate == clamped)
    {
        return;
    }

    d->sampleRate = clamped;
    rebuildLogBinMap();
    emit sampleRateChanged(d->sampleRate);
}

int ExSpectrumWidget::sampleRate() const
{
    return d->sampleRate;
}

void ExSpectrumWidget::setBarColor(const QColor &color)
{
    if (d->barColor == color && d->useCustomBarColor)
    {
        return;
    }

    d->useCustomBarColor = true;
    d->barColor = color;
    emit barColorChanged(d->barColor);
    update();
}

QColor ExSpectrumWidget::barColor() const
{
    return d->barColor;
}

void ExSpectrumWidget::setBarCount(int count)
{
    const int clamped = qBound(8, count, 256);
    if (d->barCount == clamped)
    {
        return;
    }

    d->barCount = clamped;
    d->targets.fill(kMinDisplayLevel, d->barCount);
    d->bars.fill(kMinDisplayLevel, d->barCount);
    rebuildLogBinMap();
    emit barCountChanged(d->barCount);
    emit barsChanged(d->bars);
    update();
}

int ExSpectrumWidget::barCount() const
{
    return d->barCount;
}

void ExSpectrumWidget::setRefreshIntervalMs(int intervalMs)
{
    const int clamped = qBound(8, intervalMs, 1000);
    if (d->refreshIntervalMs == clamped)
    {
        return;
    }

    d->refreshIntervalMs = clamped;
    if (d->timer)
    {
        d->timer->setInterval(d->refreshIntervalMs);
    }
    emit refreshIntervalMsChanged(d->refreshIntervalMs);
}

int ExSpectrumWidget::refreshIntervalMs() const
{
    return d->refreshIntervalMs;
}

QVector<float> ExSpectrumWidget::bars() const
{
    return d->bars;
}

void ExSpectrumWidget::clear()
{
    {
        QMutexLocker locker(&d->pcmMutex);
        d->pendingPcm.clear();
    }
    d->pcmSamples.clear();
    d->targets.fill(kMinDisplayLevel, d->barCount);
    d->bars.fill(kMinDisplayLevel, d->barCount);
    emit barsChanged(d->bars);
    update();
}

QSize ExSpectrumWidget::sizeHint() const
{
    return {640, 160};
}

QSize ExSpectrumWidget::minimumSizeHint() const
{
    return {200, 80};
}

void ExSpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect area = rect();
    painter.fillRect(area, palette().color(QPalette::Window));

    if (d->barCount <= 0 || area.width() <= 0 || area.height() <= 0)
    {
        return;
    }

    const int waterY = area.height() / 2;
    const int totalGap = kBarGapPx * (d->barCount - 1);
    const int barWidth = qMax(1, (area.width() - totalGap) / d->barCount);
    const int usedWidth = barWidth * d->barCount + totalGap;
    const int xOffset = (area.width() - usedWidth) / 2;
    const int maxBarHeight = qMax(8, waterY - 10);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor divider = QColor(palette().color(QPalette::Text).red(),
                                  palette().color(QPalette::Text).green(),
                                  palette().color(QPalette::Text).blue(),
                                  dark ? 20 : 30);

    painter.setPen(Qt::NoPen);

    for (int i = 0; i < d->barCount; ++i)
    {
        const int x = xOffset + i * (barWidth + kBarGapPx);
        const int barHeight = qMax(2, int(d->bars[i] * maxBarHeight));

        const QRectF upperRect(x, waterY - barHeight, barWidth, barHeight);
        painter.setBrush(d->barColor);
        painter.drawRoundedRect(upperRect, kBarRadiusPx, kBarRadiusPx);

        const int reflectHeight = qMax(2, int(barHeight * 0.92f));
        const QRectF lowerRect(x, waterY + 1, barWidth, reflectHeight);

        QLinearGradient gradient(x, waterY + 1, x, waterY + 1 + reflectHeight);
        QColor top = d->barColor;
        top.setAlpha(150);
        QColor bottom = d->barColor;
        bottom.setAlpha(0);
        gradient.setColorAt(0.0, top);
        gradient.setColorAt(1.0, bottom);

        painter.setBrush(gradient);
        painter.drawRoundedRect(lowerRect, kBarRadiusPx, kBarRadiusPx);
    }

    painter.setPen(divider);
    painter.drawLine(0, waterY, area.width(), waterY);
}

void ExSpectrumWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange && !d->useCustomBarColor)
    {
        syncBarColorFromPalette();
    }
}

void ExSpectrumWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void ExSpectrumWidget::onRefreshTimer()
{
    updateSpectrum();
    applyAttackDecay();
    update();
}

void ExSpectrumWidget::updateSpectrum()
{
    if (!d->fftCfg)
    {
        return;
    }

    QByteArray pcmChunk;
    {
        QMutexLocker locker(&d->pcmMutex);
        if (!d->pendingPcm.isEmpty())
        {
            pcmChunk.swap(d->pendingPcm);
        }
    }

    if (!pcmChunk.isEmpty())
    {
        const int sampleCount = pcmChunk.size() / int(sizeof(qint16));
        const auto *samples = reinterpret_cast<const qint16 *>(pcmChunk.constData());
        d->pcmSamples.reserve(d->pcmSamples.size() + sampleCount);

        for (int i = 0; i < sampleCount; ++i)
        {
            d->pcmSamples.append(float(samples[i]) / 32768.0f);
        }

        constexpr int kMaxBufferSamples = DefaultFftSize * 8;
        if (d->pcmSamples.size() > kMaxBufferSamples)
        {
            d->pcmSamples.remove(0, d->pcmSamples.size() - kMaxBufferSamples);
        }
    }

    if (d->pcmSamples.size() < DefaultFftSize)
    {
        for (int i = 0; i < d->barCount; ++i)
        {
            d->targets[i] = qMax(kMinDisplayLevel, d->targets[i] * kDecayFactor);
        }
        return;
    }

    const float *recent = d->pcmSamples.constData() + (d->pcmSamples.size() - DefaultFftSize);
    for (int i = 0; i < DefaultFftSize; ++i)
    {
        d->fftInput[i] = recent[i] * d->window[i];
    }

    kiss_fftr(d->fftCfg, d->fftInput.constData(), d->fftOutput.data());

    const int maxBin = DefaultFftSize / 2;

    for (int bar = 0; bar < d->barCount; ++bar)
    {
        const int start = qBound(1, d->binStart[bar], maxBin - 1);
        const int end = qBound(start + 1, d->binEnd[bar], maxBin);

        float sumSq = 0.0f;
        float peakMag = 0.0f;
        int count = 0;
        for (int bin = start; bin < end; ++bin)
        {
            const float re = d->fftOutput[bin].r;
            const float im = d->fftOutput[bin].i;
            const float binMag = qSqrt(re * re + im * im);
            sumSq += binMag * binMag;
            peakMag = qMax(peakMag, binMag);
            ++count;
        }

        const float rms = count > 0 ? qSqrt(sumSq / float(count)) : 0.0f;
        const float peakWeight = qBound(0.25f, d->bandCenterHz[bar] / 4000.0f, 0.75f);
        const float blended = rms * (1.0f - peakWeight) + peakMag * peakWeight;
        const float amplitude = (blended * 2.0f) / float(DefaultFftSize);

        const float octavesFrom1k = float(qLn(qMax(d->bandCenterHz[bar], 100.0f) / 1000.0f) / qLn(2.0f));
        const float tiltGain = qBound(0.35f, float(qPow(10.0f, 3.0f * octavesFrom1k / 20.0f)), 3.0f);
        const float normalized = qBound(0.0f, float(qLn(1.0f + amplitude * 180.0f * tiltGain) / 5.0f), 1.0f);
        d->targets[bar] = qMax(kMinDisplayLevel, normalized);
    }
}

void ExSpectrumWidget::rebuildLogBinMap()
{
    d->binStart.resize(d->barCount);
    d->binEnd.resize(d->barCount);
    d->bandCenterHz.resize(d->barCount);

    const int maxBin = DefaultFftSize / 2;
    const float nyquist = float(d->sampleRate) * 0.5f;
    const float maxFreq = qMin(kMaxFrequencyHz, nyquist * 0.95f);
    const float logMin = qLn(kMinFrequencyHz);
    const float logMax = qLn(maxFreq);

    for (int bar = 0; bar < d->barCount; ++bar)
    {
        const float t0 = float(bar) / float(d->barCount);
        const float t1 = float(bar + 1) / float(d->barCount);

        const float f0 = qExp(logMin + (logMax - logMin) * t0);
        const float f1 = qExp(logMin + (logMax - logMin) * t1);

        const int bin0 = qBound(1, int(f0 * float(DefaultFftSize) / float(d->sampleRate)), maxBin - 1);
        const int bin1 = qBound(bin0 + 1, int(f1 * float(DefaultFftSize) / float(d->sampleRate)), maxBin);

        d->binStart[bar] = bin0;
        d->binEnd[bar] = bin1;
        d->bandCenterHz[bar] = qSqrt(f0 * f1);
    }
}

void ExSpectrumWidget::applyAttackDecay()
{
    bool changed = false;
    for (int i = 0; i < d->barCount; ++i)
    {
        const float target = d->targets[i];
        float value = d->bars[i];

        if (target > value)
        {
            value += (target - value) * kAttackFactor;
        }
        else
        {
            value *= kDecayFactor;
            value = qMax(target, value);
        }

        value = qMax(kMinDisplayLevel, qMin(1.0f, value));
        if (qAbs(value - d->bars[i]) > 0.0005f)
        {
            d->bars[i] = value;
            changed = true;
        }
    }

    if (changed)
    {
        emit barsChanged(d->bars);
    }
}

void ExSpectrumWidget::syncBarColorFromPalette()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    d->barColor = dark ? QColor(96, 205, 255) : QColor(0, 120, 212);
    emit barColorChanged(d->barColor);
    update();
}
