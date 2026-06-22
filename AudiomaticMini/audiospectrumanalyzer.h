#pragma once

#include <QByteArray>
#include <QObject>
#include <QUrl>

class QAudioDecoder;
class QTimer;

/**
 * @brief 从媒体文件解码音频，按播放进度输出 16-bit PCM 单声道数据。
 *
 * 文件会预先解码到内存缓存，播放期间按实时速率从缓存中切片推送，
 * 避免 QAudioDecoder 快速解码完毕后频谱停止的问题。
 */
class AudioSpectrumAnalyzer : public QObject
{
    Q_OBJECT

public:
    static constexpr int kFeedIntervalMs = 16;

    explicit AudioSpectrumAnalyzer(QObject *parent = nullptr);
    ~AudioSpectrumAnalyzer() override;

    int sampleRate() const { return m_sampleRate; }

public slots:
    void setSource(const QUrl &url);
    void setActive(bool active);
    void setPlaybackPositionMs(qint64 positionMs);
    void restart();

signals:
    void pcmDataReady(const QByteArray &pcmData);
    void sampleRateChanged(int sampleRate);

private slots:
    void onBufferReady();
    void onDecoderFinished();
    void feedPlaybackChunk();

private:
    void processBuffer();
    void emitSilence();
    int totalSampleCount() const;

    QAudioDecoder *m_decoder{nullptr};
    QTimer *m_feedTimer{nullptr};

    QByteArray m_pcmCache;
    int m_sampleRate{44100};
    int m_samplePosition{0};
    qint64 m_playbackPositionMs{0};
    bool m_active{false};
    bool m_decodeFinished{false};
};
