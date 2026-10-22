#include "audiospectrumanalyzer.h"

#include <QAudioBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QTimer>
#include <QtMath>

namespace {

QByteArray toMonoInt16Pcm(const QAudioBuffer &buffer)
{
    const QAudioFormat format = buffer.format();
    const int channels = qMax(1, format.channelCount());
    const qsizetype frameCount = buffer.frameCount();
    if (frameCount <= 0)
    {
        return {};
    }

    QByteArray pcm;
    pcm.resize(int(frameCount * sizeof(qint16)));
    auto *out = reinterpret_cast<qint16 *>(pcm.data());

    switch (format.sampleFormat())
    {
    case QAudioFormat::Int16:
    {
        const auto *data = buffer.constData<qint16>();
        for (qsizetype i = 0; i < frameCount; ++i)
        {
            qint32 sum = 0;
            for (int ch = 0; ch < channels; ++ch)
            {
                sum += data[i * channels + ch];
            }
            out[i] = qint16(sum / channels);
        }
        break;
    }
    case QAudioFormat::UInt8:
    {
        const auto *data = buffer.constData<quint8>();
        for (qsizetype i = 0; i < frameCount; ++i)
        {
            int sum = 0;
            for (int ch = 0; ch < channels; ++ch)
            {
                sum += int(data[i * channels + ch]) - 128;
            }
            const int centered = sum / channels;
            out[i] = qint16(qBound(-32768, centered * 256, 32767));
        }
        break;
    }
    case QAudioFormat::Float:
    {
        const auto *data = buffer.constData<float>();
        for (qsizetype i = 0; i < frameCount; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
            {
                sum += data[i * channels + ch];
            }
            const float sample = qBound(-1.0f, sum / float(channels), 1.0f);
            out[i] = qint16(qRound(sample * 32767.0f));
        }
        break;
    }
    case QAudioFormat::Int32:
    {
        const auto *data = buffer.constData<qint32>();
        for (qsizetype i = 0; i < frameCount; ++i)
        {
            qint64 sum = 0;
            for (int ch = 0; ch < channels; ++ch)
            {
                sum += data[i * channels + ch];
            }
            const qint64 averaged = sum / channels / 65536;
            if (averaged > 32767)
            {
                out[i] = 32767;
            }
            else if (averaged < -32768)
            {
                out[i] = -32768;
            }
            else
            {
                out[i] = qint16(averaged);
            }
        }
        break;
    }
    default:
        pcm.clear();
        break;
    }

    return pcm;
}

} // namespace

AudioSpectrumAnalyzer::AudioSpectrumAnalyzer(QObject *parent)
    : QObject(parent)
{
    m_decoder = new QAudioDecoder(this);

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    m_decoder->setAudioFormat(format);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioSpectrumAnalyzer::onBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioSpectrumAnalyzer::onDecoderFinished);

    m_feedTimer = new QTimer(this);
    m_feedTimer->setTimerType(Qt::PreciseTimer);
    connect(m_feedTimer, &QTimer::timeout, this, &AudioSpectrumAnalyzer::feedPlaybackChunk);
}

AudioSpectrumAnalyzer::~AudioSpectrumAnalyzer() = default;

void AudioSpectrumAnalyzer::setSource(const QUrl &url)
{
    if (!m_decoder)
    {
        return;
    }

    m_pcmCache.clear();
    m_decodeFinished = false;
    m_samplePosition = 0;
    m_playbackPositionMs = 0;

    m_decoder->stop();
    m_decoder->setSource(url);

    if (url.isValid())
    {
        m_decoder->start();
    }

    emitSilence();
}

void AudioSpectrumAnalyzer::setActive(bool active)
{
    m_active = active;

    if (!m_active)
    {
        m_feedTimer->stop();
        emitSilence();
        return;
    }

    m_samplePosition = int(m_playbackPositionMs * m_sampleRate / 1000);
    m_feedTimer->start(kFeedIntervalMs);
}

void AudioSpectrumAnalyzer::setPlaybackPositionMs(qint64 positionMs)
{
    m_playbackPositionMs = qMax<qint64>(0, positionMs);

    const int newSamplePos = int(m_playbackPositionMs * m_sampleRate / 1000);
    const int driftSamples = qAbs(newSamplePos - m_samplePosition);

    // 仅在拖动进度条或偏差超过 50ms 时强制对齐，避免正常播放时抖动
    if (driftSamples > m_sampleRate / 20)
    {
        m_samplePosition = qBound(0, newSamplePos, totalSampleCount());
    }
}

void AudioSpectrumAnalyzer::restart()
{
    m_samplePosition = int(m_playbackPositionMs * m_sampleRate / 1000);
    m_samplePosition = qBound(0, m_samplePosition, totalSampleCount());
}

void AudioSpectrumAnalyzer::onBufferReady()
{
    while (m_decoder && m_decoder->bufferAvailable())
    {
        processBuffer();
    }

    if (m_active)
    {
        feedPlaybackChunk();
    }
}

void AudioSpectrumAnalyzer::onDecoderFinished()
{
    m_decodeFinished = true;
}

void AudioSpectrumAnalyzer::feedPlaybackChunk()
{
    if (!m_active || m_pcmCache.isEmpty())
    {
        return;
    }

    const int totalSamples = totalSampleCount();
    if (totalSamples <= 0)
    {
        return;
    }

    const int chunkSamples = qMax(256, m_sampleRate * kFeedIntervalMs / 1000);
    const int targetSamplePos = int(m_playbackPositionMs * m_sampleRate / 1000);

    // 与播放器对齐，避免读指针跑飞或落后太多
    if (qAbs(m_samplePosition - targetSamplePos) > m_sampleRate / 20)
    {
        m_samplePosition = qBound(0, targetSamplePos, totalSamples - 1);
    }

    int readPos = m_samplePosition;

    // 解码尚未追上播放进度：先喂缓存末尾最新数据，保证频谱有动静
    if (readPos >= totalSamples)
    {
        if (m_decodeFinished)
        {
            return;
        }
        readPos = qMax(0, totalSamples - chunkSamples);
    }

    const int availableSamples = qMin(chunkSamples, totalSamples - readPos);
    if (availableSamples <= 0)
    {
        return;
    }

    const int byteOffset = readPos * int(sizeof(qint16));
    emit pcmDataReady(m_pcmCache.mid(byteOffset, availableSamples * int(sizeof(qint16))));
    m_samplePosition = readPos + availableSamples;
}

void AudioSpectrumAnalyzer::processBuffer()
{
    const QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid() || buffer.frameCount() == 0)
    {
        return;
    }

    const QAudioFormat format = buffer.format();
    if (format.sampleRate() > 0 && format.sampleRate() != m_sampleRate)
    {
        m_sampleRate = format.sampleRate();
        emit sampleRateChanged(m_sampleRate);
    }

    const QByteArray pcm = toMonoInt16Pcm(buffer);
    if (!pcm.isEmpty())
    {
        m_pcmCache.append(pcm);
    }
}

void AudioSpectrumAnalyzer::emitSilence()
{
    emit pcmDataReady({});
}

int AudioSpectrumAnalyzer::totalSampleCount() const
{
    return m_pcmCache.size() / int(sizeof(qint16));
}
