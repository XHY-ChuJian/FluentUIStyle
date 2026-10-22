#include "systemaudiocapture.h"

#include <QAtomicInt>
#include <QDebug>
#include <QMutex>
#include <QThread>
#include <QtGlobal>
#include <QtMath>
#include <cstring>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <ksmedia.h>
#include <functiondiscoverykeys_devpkey.h>

#ifndef AUDCLNT_STREAMFLAGS_LOOPBACK
#define AUDCLNT_STREAMFLAGS_LOOPBACK 0x00020000
#endif

#ifndef AUDCLNT_BUFFERFLAGS_SILENT
#define AUDCLNT_BUFFERFLAGS_SILENT 0x2
#endif

namespace {
// IEEE Float 子格式 GUID 常量防卫
static const GUID kGuidIeeeFloat =
    { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID kGuidPcm =
    { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
} // namespace

class WasapiLoopbackThread : public QThread
{
    Q_OBJECT

public:
    explicit WasapiLoopbackThread(QObject *parent = nullptr)
        : QThread(parent)
    {
    }

    ~WasapiLoopbackThread() override
    {
        stopThread();
    }

    void stopThread()
    {
        m_running.storeRelaxed(0);
        wait(2000);
    }

    int sampleRate() const { return m_sampleRate; }
    int channelCount() const { return m_channels; }
    QString deviceName() const { return m_deviceName; }

signals:
    void dataReady(const QByteArray &pcmData, int channels);
    void streamInitialized(int sampleRate, int channels, const QString &deviceName);
    void captureError(const QString &error);

protected:
    void run() override
    {
        const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(hrCo);

        m_running.storeRelaxed(1);

        while (m_running.loadRelaxed())
        {
            IMMDeviceEnumerator *pEnumerator = nullptr;
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&pEnumerator));
            if (FAILED(hr) || !pEnumerator)
            {
                emit captureError(QStringLiteral("无法创建音频枚举器 (CoCreateInstance 失败)"));
                QThread::msleep(1000);
                continue;
            }

            IMMDevice *pDevice = nullptr;
            // 优先获取多媒体播放角色端点 (eMultimedia)，失败再回退控制台端点 (eConsole)
            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
            if (FAILED(hr) || !pDevice)
            {
                hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
            }
            if (FAILED(hr) || !pDevice)
            {
                pEnumerator->Release();
                emit captureError(QStringLiteral("未找到系统默认音频输出端点"));
                QThread::msleep(1000);
                continue;
            }

            QString devName = QStringLiteral("默认音频设备 (系统声音)");
            IPropertyStore *pProps = nullptr;
            if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pProps)))
            {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName)))
                {
                    if (varName.pwszVal)
                    {
                        devName = QString::fromWCharArray(varName.pwszVal);
                    }
                    PropVariantClear(&varName);
                }
                pProps->Release();
            }

            IAudioClient *pAudioClient = nullptr;
            hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&pAudioClient));
            if (FAILED(hr) || !pAudioClient)
            {
                pDevice->Release();
                pEnumerator->Release();
                emit captureError(QStringLiteral("无法激活音频客户端接口 (Activate IAudioClient 失败)"));
                QThread::msleep(1000);
                continue;
            }

            WAVEFORMATEX *pwfx = nullptr;
            hr = pAudioClient->GetMixFormat(&pwfx);
            if (FAILED(hr) || !pwfx)
            {
                pAudioClient->Release();
                pDevice->Release();
                pEnumerator->Release();
                emit captureError(QStringLiteral("无法获取音频混音格式 (GetMixFormat 失败)"));
                QThread::msleep(1000);
                continue;
            }

            m_sampleRate = int(pwfx->nSamplesPerSec);
            m_channels = int(pwfx->nChannels);
            m_deviceName = devName;

            bool isFloat = false;
            if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            {
                isFloat = true;
            }
            else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                const auto *pExt = reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(pwfx);
                if (IsEqualGUID(pExt->SubFormat, kGuidIeeeFloat))
                {
                    isFloat = true;
                }
            }
            const int bitsPerSample = int(pwfx->wBitsPerSample);

            // 初始化为系统音频回环模式 (AUDCLNT_STREAMFLAGS_LOOPBACK)
            const REFERENCE_TIME hnsRequestedDuration = 1000000; // 100ms
            hr = pAudioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_LOOPBACK,
                hnsRequestedDuration,
                0,
                pwfx,
                nullptr);

            if (FAILED(hr))
            {
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
                pDevice->Release();
                pEnumerator->Release();
                emit captureError(QStringLiteral("初始化 WASAPI Loopback 失败: 0x%1").arg(ulong(hr), 8, 16, QChar('0')));
                QThread::msleep(1000);
                continue;
            }

            IAudioCaptureClient *pCaptureClient = nullptr;
            hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(&pCaptureClient));
            if (FAILED(hr) || !pCaptureClient)
            {
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
                pDevice->Release();
                pEnumerator->Release();
                emit captureError(QStringLiteral("无法获取 IAudioCaptureClient 接口"));
                QThread::msleep(1000);
                continue;
            }

            hr = pAudioClient->Start();
            if (FAILED(hr))
            {
                pCaptureClient->Release();
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
                pDevice->Release();
                pEnumerator->Release();
                emit captureError(QStringLiteral("启动音频捕获流失败"));
                QThread::msleep(1000);
                continue;
            }

            emit streamInitialized(m_sampleRate, m_channels, m_deviceName);

            bool deviceInvalidated = false;

            while (m_running.loadRelaxed() && !deviceInvalidated)
            {
                UINT32 packetLength = 0;
                hr = pCaptureClient->GetNextPacketSize(&packetLength);
                if (FAILED(hr))
                {
                    // 检测到设备错误（如 AUDCLNT_E_DEVICE_INVALIDATED），触发自愈重新枚举重连
                    deviceInvalidated = true;
                    break;
                }

                if (packetLength == 0)
                {
                    QThread::msleep(10);
                    continue;
                }

                while (packetLength > 0 && m_running.loadRelaxed())
                {
                    BYTE *pData = nullptr;
                    UINT32 numFramesAvailable = 0;
                    DWORD flags = 0;

                    hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
                    if (SUCCEEDED(hr) && numFramesAvailable > 0 && pData)
                    {
                        const int sampleCount = int(numFramesAvailable * m_channels);
                        QByteArray pcmData;
                        pcmData.resize(int(sampleCount * sizeof(qint16)));
                        auto *out = reinterpret_cast<qint16 *>(pcmData.data());

                        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                        {
                            pcmData.fill(0);
                        }
                        else if (isFloat)
                        {
                            const auto *inFloat = reinterpret_cast<const float *>(pData);
                            for (int i = 0; i < sampleCount; ++i)
                            {
                                const float sample = qBound(-1.0f, inFloat[i], 1.0f);
                                out[i] = static_cast<qint16>(sample * 32767.0f);
                            }
                        }
                        else if (bitsPerSample == 16)
                        {
                            const auto *in16 = reinterpret_cast<const qint16 *>(pData);
                            std::memcpy(out, in16, sampleCount * sizeof(qint16));
                        }
                        else if (bitsPerSample == 24)
                        {
                            const auto *b = reinterpret_cast<const unsigned char *>(pData);
                            for (int i = 0; i < sampleCount; ++i)
                            {
                                const int base = i * 3;
                                const int32_t val = (static_cast<int32_t>(b[base]) << 8) |
                                                    (static_cast<int32_t>(b[base + 1]) << 16) |
                                                    (static_cast<int32_t>(static_cast<int8_t>(b[base + 2])) << 24);
                                out[i] = static_cast<qint16>(val >> 16);
                            }
                        }
                        else if (bitsPerSample == 32)
                        {
                            const auto *in32 = reinterpret_cast<const qint32 *>(pData);
                            for (int i = 0; i < sampleCount; ++i)
                            {
                                out[i] = static_cast<qint16>(in32[i] >> 16);
                            }
                        }
                        else
                        {
                            pcmData.fill(0);
                        }

                        emit dataReady(pcmData, m_channels);
                        pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    }
                    else if (FAILED(hr))
                    {
                        deviceInvalidated = true;
                        break;
                    }

                    hr = pCaptureClient->GetNextPacketSize(&packetLength);
                    if (FAILED(hr))
                    {
                        deviceInvalidated = true;
                        break;
                    }
                }

                QThread::msleep(10);
            }

            pAudioClient->Stop();
            pCaptureClient->Release();
            CoTaskMemFree(pwfx);
            pAudioClient->Release();
            pDevice->Release();
            pEnumerator->Release();

            if (deviceInvalidated && m_running.loadRelaxed())
            {
                QThread::msleep(500); // 设备失效或音频流断开，等待 500ms 后自愈重新枚举与初始化
            }
        }

        if (coInitialized)
        {
            CoUninitialize();
        }
    }

private:
    QAtomicInt m_running{0};
    int m_sampleRate{48000};
    int m_channels{2};
    QString m_deviceName;
};

#else

#include <QAudioFormat>
#include <QAudioSource>
#include <QMediaDevices>
#include <QIODevice>

class QtAudioSourceWorker : public QObject
{
    Q_OBJECT

public:
    explicit QtAudioSourceWorker(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~QtAudioSourceWorker() override
    {
        stop();
    }

    bool start()
    {
        const auto device = QMediaDevices::defaultAudioInput();
        if (device.isNull())
        {
            emit captureError(QStringLiteral("未找到系统音频输入设备"));
            return false;
        }

        m_deviceName = device.description();
        QAudioFormat format = device.preferredFormat();
        if (format.sampleRate() <= 0)
        {
            format.setSampleRate(44100);
            format.setChannelCount(2);
            format.setSampleFormat(QAudioFormat::Int16);
        }

        m_sampleRate = format.sampleRate();
        m_channels = qMax(1, format.channelCount());

        m_audioSource = new QAudioSource(device, format, this);
        m_ioDevice = m_audioSource->start();
        if (!m_ioDevice)
        {
            emit captureError(QStringLiteral("启动音频输入流失败"));
            return false;
        }

        connect(m_ioDevice, &QIODevice::readyRead, this, [this]() {
            if (!m_ioDevice)
            {
                return;
            }
            const QByteArray data = m_ioDevice->readAll();
            if (!data.isEmpty())
            {
                emit dataReady(data, m_channels);
            }
        });

        emit streamInitialized(m_sampleRate, m_channels, m_deviceName);
        return true;
    }

    void stop()
    {
        if (m_audioSource)
        {
            m_audioSource->stop();
            m_audioSource->deleteLater();
            m_audioSource = nullptr;
            m_ioDevice = nullptr;
        }
    }

    int sampleRate() const { return m_sampleRate; }
    int channelCount() const { return m_channels; }
    QString deviceName() const { return m_deviceName; }

signals:
    void dataReady(const QByteArray &pcmData, int channels);
    void streamInitialized(int sampleRate, int channels, const QString &deviceName);
    void captureError(const QString &error);

private:
    QAudioSource *m_audioSource{nullptr};
    QIODevice *m_ioDevice{nullptr};
    int m_sampleRate{44100};
    int m_channels{2};
    QString m_deviceName;
};

#endif

// ============================================================================
// SystemAudioCapturePrivate
// ============================================================================

class SystemAudioCapturePrivate
{
public:
    explicit SystemAudioCapturePrivate(SystemAudioCapture *q)
        : q_ptr(q)
    {
    }

    SystemAudioCapture *q_ptr{nullptr};
    bool isCapturing{false};
    int sampleRate{48000};
    int channelCount{2};
    QString deviceName{QStringLiteral("电脑声音 (系统音频)")};

#ifdef Q_OS_WIN
    WasapiLoopbackThread *winThread{nullptr};
#else
    QtAudioSourceWorker *qtWorker{nullptr};
#endif
};

// ============================================================================
// SystemAudioCapture
// ============================================================================

SystemAudioCapture::SystemAudioCapture(QObject *parent)
    : QObject(parent), d(new SystemAudioCapturePrivate(this))
{
}

SystemAudioCapture::~SystemAudioCapture()
{
    stop();
    delete d;
}

bool SystemAudioCapture::isCapturing() const
{
    return d->isCapturing;
}

int SystemAudioCapture::sampleRate() const
{
    return d->sampleRate;
}

int SystemAudioCapture::channelCount() const
{
    return d->channelCount;
}

QString SystemAudioCapture::currentDeviceName() const
{
    return d->deviceName;
}

bool SystemAudioCapture::start()
{
    if (d->isCapturing)
    {
        return true;
    }

#ifdef Q_OS_WIN
    if (!d->winThread)
    {
        d->winThread = new WasapiLoopbackThread(this);

        connect(d->winThread, &WasapiLoopbackThread::dataReady,
                this, &SystemAudioCapture::pcmDataReady, Qt::QueuedConnection);

        connect(d->winThread, &WasapiLoopbackThread::streamInitialized,
                this, [this](int sampleRate, int channels, const QString &devName) {
                    d->sampleRate = sampleRate;
                    d->channelCount = channels;
                    d->deviceName = devName;
                    emit sampleRateChanged(sampleRate);
                    emit deviceNameChanged(devName);
                }, Qt::QueuedConnection);

        connect(d->winThread, &WasapiLoopbackThread::captureError,
                this, &SystemAudioCapture::errorOccurred, Qt::QueuedConnection);
    }

    d->isCapturing = true;
    emit capturingChanged(true);
    d->winThread->start();
    return true;

#else
    if (!d->qtWorker)
    {
        d->qtWorker = new QtAudioSourceWorker(this);

        connect(d->qtWorker, &QtAudioSourceWorker::dataReady,
                this, &SystemAudioCapture::pcmDataReady);

        connect(d->qtWorker, &QtAudioSourceWorker::streamInitialized,
                this, [this](int sampleRate, int channels, const QString &devName) {
                    d->sampleRate = sampleRate;
                    d->channelCount = channels;
                    d->deviceName = devName;
                    emit sampleRateChanged(sampleRate);
                    emit deviceNameChanged(devName);
                });

        connect(d->qtWorker, &QtAudioSourceWorker::captureError,
                this, &SystemAudioCapture::errorOccurred);
    }

    const bool ok = d->qtWorker->start();
    if (ok)
    {
        d->isCapturing = true;
        emit capturingChanged(true);
    }
    return ok;
#endif
}

void SystemAudioCapture::stop()
{
    if (!d->isCapturing)
    {
        return;
    }

    d->isCapturing = false;

#ifdef Q_OS_WIN
    if (d->winThread)
    {
        d->winThread->stopThread();
        d->winThread->deleteLater();
        d->winThread = nullptr;
    }
#else
    if (d->qtWorker)
    {
        d->qtWorker->stop();
        d->qtWorker->deleteLater();
        d->qtWorker = nullptr;
    }
#endif

    emit capturingChanged(false);
}

#include "systemaudiocapture.moc"
