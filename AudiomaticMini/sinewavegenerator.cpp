#include "sinewavegenerator.h"

#include <QtMath>

#include <cmath>

SineWaveGenerator::SineWaveGenerator(int sampleRate)
    : m_sampleRate(qMax(8000, sampleRate))
{
    setFrequencies({220.0f, 440.0f, 880.0f, 1760.0f});
}

void SineWaveGenerator::setSampleRate(int sampleRate)
{
    m_sampleRate = qMax(8000, sampleRate);
}

void SineWaveGenerator::setFrequencies(const QVector<float> &frequenciesHz)
{
    m_frequencies = frequenciesHz;
    m_frequencies.removeAll(0.0f);
    if (m_frequencies.isEmpty())
    {
        m_frequencies.append(440.0f);
    }
}

void SineWaveGenerator::setAmplitude(float amplitude)
{
    m_amplitude = qBound(0.0f, amplitude, 1.0f);
}

QByteArray SineWaveGenerator::generate(int sampleCount)
{
    QByteArray pcm;
    pcm.resize(sampleCount * int(sizeof(qint16)));

    auto *out = reinterpret_cast<qint16 *>(pcm.data());
    const double twoPi = 2.0 * M_PI;

    for (int i = 0; i < sampleCount; ++i)
    {
        double sample = 0.0;
        for (float frequency : m_frequencies)
        {
            sample += qSin(twoPi * double(frequency) * m_phase / double(m_sampleRate));
        }

        sample *= double(m_amplitude) / double(qMax(1, m_frequencies.size()));
        sample = qBound(-1.0, sample, 1.0);

        out[i] = qint16(qRound(sample * 32767.0));
        m_phase += 1.0;
        if (m_phase >= double(m_sampleRate))
        {
            m_phase -= double(m_sampleRate);
        }
    }

    return pcm;
}
