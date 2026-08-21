#include "waveformwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QtMath>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(240);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    initDefaultChannels();
}

void WaveformWidget::initDefaultChannels()
{
    const QList<QColor> colors = {
        QColor(0, 164, 239),    // Fluent Blue
        QColor(16, 124, 65),    // Fluent Green
        QColor(255, 185, 0),    // Fluent Gold
        QColor(232, 17, 35),    // Fluent Red
        QColor(180, 0, 158),    // Fluent Purple
        QColor(0, 183, 195),    // Fluent Teal
        QColor(255, 140, 0),    // Fluent Orange
        QColor(135, 100, 184)   // Fluent Lavender
    };

    m_channels.clear();
    for (int i = 0; i < 8; ++i) {
        WaveformChannel ch;
        ch.id = i;
        ch.name = QStringLiteral("CH %1").arg(i + 1);
        ch.color = colors[i % colors.size()];
        ch.visible = (i < 3); // 默认开启前 3 个通道
        m_channels.append(ch);
    }
}

void WaveformWidget::setMaxPoints(int maxPoints)
{
    m_maxPoints = qMax(50, maxPoints);
    update();
}

void WaveformWidget::addDataSample(const QVector<double> &values)
{
    if (m_paused || values.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_channels.size(); ++i) {
        if (i < values.size()) {
            m_channels[i].dataPoints.append(values[i]);
            if (m_channels[i].dataPoints.size() > m_maxPoints) {
                m_channels[i].dataPoints.removeFirst();
            }
        }
    }

    // 动态计算 Y 轴量程
    if (m_autoScale) {
        double minVal = 1e9;
        double maxVal = -1e9;
        bool hasData = false;

        for (const auto &ch : m_channels) {
            if (!ch.visible || ch.dataPoints.isEmpty()) continue;
            for (double val : ch.dataPoints) {
                if (std::isnan(val) || std::isinf(val)) continue;
                minVal = qMin(minVal, val);
                maxVal = qMax(maxVal, val);
                hasData = true;
            }
        }

        if (hasData) {
            double margin = (maxVal - minVal) * 0.1;
            if (margin < 0.5) margin = 0.5;
            m_yMin = minVal - margin;
            m_yMax = maxVal + margin;
        }
    }

    update();
}

void WaveformWidget::clearData()
{
    for (auto &ch : m_channels) {
        ch.dataPoints.clear();
    }
    update();
}

void WaveformWidget::setChannelVisible(int channelIndex, bool visible)
{
    if (channelIndex >= 0 && channelIndex < m_channels.size()) {
        m_channels[channelIndex].visible = visible;
        update();
    }
}

void WaveformWidget::setChannelColor(int channelIndex, const QColor &color)
{
    if (channelIndex >= 0 && channelIndex < m_channels.size()) {
        m_channels[channelIndex].color = color;
        update();
    }
}

void WaveformWidget::setChannelName(int channelIndex, const QString &name)
{
    if (channelIndex >= 0 && channelIndex < m_channels.size()) {
        m_channels[channelIndex].name = name;
        update();
    }
}

void WaveformWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect rect = this->rect();
    const int leftMargin = 55;
    const int rightMargin = 20;
    const int topMargin = 20;
    const int bottomMargin = 25;

    const QRect plotRect(leftMargin, topMargin, rect.width() - leftMargin - rightMargin, rect.height() - topMargin - bottomMargin);

    // 1. 绘制波形图背景网格
    painter.fillRect(plotRect, palette().color(QPalette::Base));
    painter.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::SolidLine));
    painter.drawRect(plotRect);

    // 网格线
    painter.setPen(QPen(palette().color(QPalette::Midlight), 1, Qt::DashLine));
    const int gridRows = 6;
    const int gridCols = 8;

    for (int r = 1; r < gridRows; ++r) {
        int y = plotRect.top() + (plotRect.height() * r) / gridRows;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);
    }
    for (int c = 1; c < gridCols; ++c) {
        int x = plotRect.left() + (plotRect.width() * c) / gridCols;
        painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
    }

    // 零刻度线加粗
    if (m_yMin < 0.0 && m_yMax > 0.0) {
        double zeroRatio = (0.0 - m_yMin) / (m_yMax - m_yMin);
        int zeroY = plotRect.bottom() - static_cast<int>(zeroRatio * plotRect.height());
        painter.setPen(QPen(QColor(128, 128, 128, 160), 1.5, Qt::SolidLine));
        painter.drawLine(plotRect.left(), zeroY, plotRect.right(), zeroY);
    }

    // 2. 绘制 Y 轴数值标签
    painter.setPen(palette().color(QPalette::WindowText));
    QFont font = painter.font();
    font.setPixelSize(11);
    painter.setFont(font);

    for (int r = 0; r <= gridRows; ++r) {
        double val = m_yMax - (m_yMax - m_yMin) * (static_cast<double>(r) / gridRows);
        int y = plotRect.top() + (plotRect.height() * r) / gridRows;
        QString text = QString::number(val, 'f', (qAbs(m_yMax - m_yMin) < 10.0) ? 2 : 1);
        painter.drawText(QRect(0, y - 8, leftMargin - 6, 16), Qt::AlignRight | Qt::AlignVCenter, text);
    }

    // 3. 绘制各通道折线
    painter.setClipRect(plotRect);

    for (const auto &ch : m_channels) {
        if (!ch.visible || ch.dataPoints.size() < 2) {
            continue;
        }

        QPainterPath path;
        const int count = ch.dataPoints.size();
        const double xStep = static_cast<double>(plotRect.width()) / qMax(1, m_maxPoints - 1);
        const double yRange = (m_yMax - m_yMin) > 1e-6 ? (m_yMax - m_yMin) : 1.0;

        for (int i = 0; i < count; ++i) {
            double v = ch.dataPoints[i];
            double x = plotRect.left() + (m_maxPoints - count + i) * xStep;
            double yRatio = (v - m_yMin) / yRange;
            double y = plotRect.bottom() - (yRatio * plotRect.height());

            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }

        painter.setPen(QPen(ch.color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
    }

    painter.setClipping(false);

    // 4. 绘制右上方通道图例
    int legendX = plotRect.right() - 10;
    int legendY = plotRect.top() + 10;

    for (int i = m_channels.size() - 1; i >= 0; --i) {
        const auto &ch = m_channels[i];
        if (!ch.visible) continue;

        QString legendText = ch.name;
        if (!ch.dataPoints.isEmpty()) {
            legendText += QStringLiteral(": %1").arg(ch.dataPoints.last(), 0, 'f', 1);
        }

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(legendText);
        legendX -= (textWidth + 24);

        painter.setPen(Qt::NoPen);
        painter.setBrush(ch.color);
        painter.drawRoundedRect(legendX, legendY + 2, 10, 10, 2, 2);

        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRect(legendX + 14, legendY, textWidth, 14), Qt::AlignLeft | Qt::AlignVCenter, legendText);

        legendX -= 8;
    }
}
