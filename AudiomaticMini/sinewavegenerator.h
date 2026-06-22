#pragma once

#include <QByteArray>
#include <QVector>

/**
 * @brief 正弦波 PCM 生成器，用于不接麦克风/音频文件时的频谱演示。
 */
class SineWaveGenerator
{
public:
    SineWaveGenerator(int sampleRate = 44100);

    void setSampleRate(int sampleRate);
    void setFrequencies(const QVector<float> &frequenciesHz);
    void setAmplitude(float amplitude);

    /// 生成 sampleCount 个 16-bit PCM 单声道样本。
    QByteArray generate(int sampleCount);

private:
    int m_sampleRate{44100};
    QVector<float> m_frequencies;
    float m_amplitude{0.35f};
    double m_phase{0.0};
};
