#pragma once

#include <QMainWindow>

class SpectrumWidget;
class SineWaveGenerator;

/**
 * @brief SpectrumWidget 独立演示窗口（MainWindow 使用示例）。
 *
 * 通过 SineWaveGenerator 模拟 16-bit PCM 单声道数据，
 * 无需麦克风或音频文件即可观察频谱动画。
 */
class SpectrumDemoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SpectrumDemoWindow(QWidget *parent = nullptr);
    ~SpectrumDemoWindow() override;

private slots:
    void feedSimulatedAudio();

private:
    SpectrumWidget *m_spectrum{nullptr};
    SineWaveGenerator *m_generator{nullptr};
    class QTimer *m_feedTimer{nullptr};
};
