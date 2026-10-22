#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class SystemAudioCapturePrivate;

/**
 * @brief 系统音频捕获器。
 *
 * 在 Windows 平台上基于 Core Audio WASAPI Loopback 机制直接捕获当前扬声器/耳机正在播放的混音音频流
 * （如网易云、QQ音乐、浏览器、游戏等电脑正在发出的声音），支持多声道自动采样与低延迟传输；
 * 在非 Windows 平台上回退至 QAudioSource 音频输入设备。
 */
class SystemAudioCapture : public QObject
{
    Q_OBJECT

public:
    explicit SystemAudioCapture(QObject *parent = nullptr);
    ~SystemAudioCapture() override;

    bool isCapturing() const;
    int sampleRate() const;
    int channelCount() const;
    QString currentDeviceName() const;

public slots:
    bool start();
    void stop();

signals:
    void pcmDataReady(const QByteArray &pcmData, int channels);
    void sampleRateChanged(int sampleRate);
    void capturingChanged(bool capturing);
    void deviceNameChanged(const QString &deviceName);
    void errorOccurred(const QString &error);

private:
    SystemAudioCapturePrivate *d{nullptr};
};
