#pragma once

#include <QColor>
#include <QList>
#include <QMap>
#include <QVector>
#include <QWidget>

struct WaveformChannel {
    int id{0};
    QString name;
    QColor color;
    bool visible{true};
    QVector<double> dataPoints;
};

class WaveformWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    void setMaxPoints(int maxPoints);
    int maxPoints() const { return m_maxPoints; }

    void addDataSample(const QVector<double> &values);
    void clearData();
    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

    void setChannelVisible(int channelIndex, bool visible);
    void setChannelColor(int channelIndex, const QColor &color);
    void setChannelName(int channelIndex, const QString &name);
    int channelCount() const { return m_channels.size(); }
    const QVector<WaveformChannel> &channels() const { return m_channels; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initDefaultChannels();

    int m_maxPoints{300};
    bool m_paused{false};
    QVector<WaveformChannel> m_channels;

    double m_yMin{-10.0};
    double m_yMax{10.0};
    bool m_autoScale{true};
};
