#include "sinewavegenerator.h"

#include <QtMath>

#include <cmath>

SineWaveGenerator::SineWaveGenerator(int sampleRate)
    : m_sampleRate(qMax(8000, sampleRate))
{
    setFrequencies({80.0f, 160.0f, 320.0f, 640.0f, 1280.0f, 2560.0f, 5120.0f});
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

void SineWaveGenerator::setAnimated(bool animated)
{
    m_animated = animated;
}

QByteArray SineWaveGenerator::generate(int sampleCount)
{
    QByteArray pcm;
    pcm.resize(sampleCount * int(sizeof(qint16)));

    auto *out = reinterpret_cast<qint16 *>(pcm.data());
    const double twoPi = 2.0 * M_PI;
    const int bandCount = qMax(1, m_frequencies.size());

    for (int i = 0; i < sampleCount; ++i)
    {
        const double timeSec = m_phase / double(m_sampleRate);
        double sample = 0.0;

        for (int band = 0; band < m_frequencies.size(); ++band)
        {
            const double frequency = double(m_frequencies[band]);
            double weight = 1.0;

            if (m_animated)
            {
                // 各频带独立 LFO，让柱状高度随时间起伏
                const double modRate = 0.35 + band * 0.19;
                const double modPhase = band * 1.31;
                const double lfo = 0.5 + 0.5 * qSin(twoPi * modRate * timeSec + modPhase);
                weight = 0.18 + 0.82 * qPow(lfo, 1.35);
            }

            sample += weight * qSin(twoPi * frequency * m_phase / double(m_sampleRate));
        }

        if (m_animated)
        {
            // 低频节拍脉冲，模拟鼓点
            const double beatPeriod = 0.52;
            const double beatPhase = std::fmod(timeSec, beatPeriod);
            const double kickEnv = beatPhase < 0.07 ? qExp(-beatPhase * 55.0) : 0.0;
            sample += kickEnv * 1.4 * qSin(twoPi * 58.0 * m_phase / double(m_sampleRate));

            // 中高频随机感：缓慢扫频
            const double sweepHz = 900.0 + 2800.0 * (0.5 + 0.5 * qSin(twoPi * 0.11 * timeSec));
            const double sweepAmp = 0.22 * (0.5 + 0.5 * qSin(twoPi * 0.07 * timeSec + 0.8));
            sample += sweepAmp * qSin(twoPi * sweepHz * m_phase / double(m_sampleRate));
        }

        sample *= double(m_amplitude) / double(bandCount + (m_animated ? 1 : 0));
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
