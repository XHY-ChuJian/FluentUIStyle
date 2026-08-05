#include "spectrumshowcasewidget.h"

#include "exspectrumwidget.h"
#include "sinewavegenerator.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

SpectrumShowcaseWidget::SpectrumShowcaseWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setObjectName(QStringLiteral("pageExSpectrum"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *hint = new QLabel(
        tr("ExSpectrumWidget 演示：模拟 PCM 驱动（各频带 LFO + 节拍脉冲，Qt 5 为模拟数据）。"),
        this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    m_spectrum = new ExSpectrumWidget(this);
    m_spectrum->setSampleRate(44100);
    m_spectrum->setMinimumHeight(160);
    layout->addWidget(m_spectrum, 1);

    m_generator = new SineWaveGenerator(44100);
    m_generator->setAmplitude(0.55f);
    m_generator->setAnimated(true);

    m_feedTimer = new QTimer(this);
    m_feedTimer->setTimerType(Qt::PreciseTimer);
    connect(m_feedTimer, &QTimer::timeout, this, &SpectrumShowcaseWidget::feedSimulatedAudio);
    m_feedTimer->start(ExSpectrumWidget::DefaultRefreshIntervalMs);
}

SpectrumShowcaseWidget::~SpectrumShowcaseWidget() = default;

void SpectrumShowcaseWidget::feedSimulatedAudio()
{
    if (!m_spectrum || !m_generator)
    {
        return;
    }

    const int sampleCount = qMax(
        256,
        44100 * ExSpectrumWidget::DefaultRefreshIntervalMs / 1000);
    m_spectrum->pushAudioData(m_generator->generate(sampleCount));
}
