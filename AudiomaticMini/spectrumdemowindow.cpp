#include "spectrumdemowindow.h"

#include "sinewavegenerator.h"
#include "spectrumwidget.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

SpectrumDemoWindow::SpectrumDemoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SpectrumWidget Demo"));
    resize(720, 280);
    setMinimumSize(320, 120);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);

    m_spectrum = new SpectrumWidget(central);
    m_spectrum->setSampleRate(44100);
    layout->addWidget(m_spectrum);

    setCentralWidget(central);

    m_generator = new SineWaveGenerator(44100);
    m_generator->setFrequencies({80.0f, 160.0f, 320.0f, 640.0f, 1280.0f, 2560.0f, 5120.0f});
    m_generator->setAmplitude(0.45f);

    // 每 16ms 推送一帧模拟 PCM，与控件刷新率一致
    m_feedTimer = new QTimer(this);
    m_feedTimer->setTimerType(Qt::PreciseTimer);
    connect(m_feedTimer, &QTimer::timeout, this, &SpectrumDemoWindow::feedSimulatedAudio);
    m_feedTimer->start(SpectrumWidget::kRefreshIntervalMs);
}

SpectrumDemoWindow::~SpectrumDemoWindow() = default;

void SpectrumDemoWindow::feedSimulatedAudio()
{
    if (!m_spectrum || !m_generator)
    {
        return;
    }

    // 44100 Hz * 16ms ≈ 706 个样本
    const int sampleCount = qMax(256, m_generator ? (44100 * SpectrumWidget::kRefreshIntervalMs / 1000) : 512);
    const QByteArray pcm = m_generator->generate(sampleCount);
    m_spectrum->setAudioData(pcm);
}
