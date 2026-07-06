#pragma once

#include <QByteArray>
#include <QVector>

/// 正弦波 PCM 生成器，用于 Example 中 ExSpectrumWidget 的模拟演示。
class SineWaveGenerator
{
public:
    SineWaveGenerator(int sampleRate = 44100);

    void setSampleRate(int sampleRate);
    void setFrequencies(const QVector<float> &frequenciesHz);
    void setAmplitude(float amplitude);
    void setAnimated(bool animated);

    QByteArray generate(int sampleCount);

private:
    int m_sampleRate{44100};
    QVector<float> m_frequencies;
    float m_amplitude{0.35f};
    bool m_animated{true};
    double m_phase{0.0};
};
