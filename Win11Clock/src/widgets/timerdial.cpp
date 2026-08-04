#include "widgets/timerdial.h"

#include "common/fluenthelpers.h"

#include <QPainter>
#include <QPainterPath>
#include <QTime>

#include <limits>

TimerDial::TimerDial(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TimerDial::setTime(qint64 remainingMilliseconds,
                        qint64 totalMilliseconds,
                        bool running)
{
    m_remainingMilliseconds = qMax<qint64>(0, remainingMilliseconds);
    m_totalMilliseconds = qMax<qint64>(1, totalMilliseconds);
    m_running = running;
    update();
}

QSize TimerDial::sizeHint() const
{
    return QSize(260, 260);
}

QSize TimerDial::minimumSizeHint() const
{
    return QSize(190, 190);
}

void TimerDial::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const int side = qMin(width(), height());
    const qreal ringWidth = qMax<qreal>(10.0, side * 0.055);
    const QRectF ringRect((width() - side) / 2.0 + ringWidth,
                          (height() - side) / 2.0 + ringWidth,
                          side - ringWidth * 2.0,
                          side - ringWidth * 2.0);

    QColor track = palette().color(QPalette::Mid);
    track.setAlpha(105);
    QPen trackPen(track, ringWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(trackPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 0, 360 * 16);

    if (m_running || m_remainingMilliseconds < m_totalMilliseconds)
    {
        const qreal progress = qBound<qreal>(
            0.0,
            static_cast<qreal>(m_remainingMilliseconds)
                / static_cast<qreal>(m_totalMilliseconds),
            1.0);
        QPen progressPen(palette().color(QPalette::Highlight),
                         ringWidth,
                         Qt::SolidLine,
                         Qt::RoundCap);
        painter.setPen(progressPen);
        painter.drawArc(ringRect, 90 * 16, -qRound(progress * 360.0 * 16.0));
    }

    const QString timeText = ClockUi::formatDuration(m_remainingMilliseconds);
    QFont timeFont = font();
    timeFont.setFamily(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(qBound(28, qRound(side * 0.17), 48));
    timeFont.setWeight(QFont::DemiBold);
    painter.setFont(timeFont);
    painter.setPen(palette().color(QPalette::WindowText));

    QRectF textRect = ringRect;
    if (m_running)
        textRect.translate(0, -12);
    painter.drawText(textRect, Qt::AlignCenter, timeText);

    if (m_running)
    {
        const QTime finishTime = QTime::currentTime().addMSecs(
            static_cast<int>(qMin<qint64>(m_remainingMilliseconds,
                                         std::numeric_limits<int>::max())));
        const QString finishText = QStringLiteral("\uE7ED  %1")
                                       .arg(finishTime.toString(QStringLiteral("HH:mm")));
        QFont finishFont = font();
        finishFont.setPixelSize(12);
        painter.setFont(finishFont);

        const QFontMetrics metrics(finishFont);
        const QSize pillSize(metrics.horizontalAdvance(finishText) + 24, 30);
        QRectF pillRect(0, 0, pillSize.width(), pillSize.height());
        pillRect.moveCenter(QPointF(width() / 2.0, height() / 2.0 + 42));

        QColor pillColor = palette().color(QPalette::Button);
        pillColor.setAlpha(210);
        painter.setPen(Qt::NoPen);
        painter.setBrush(pillColor);
        painter.drawRoundedRect(pillRect, 15, 15);
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(pillRect, Qt::AlignCenter, finishText);
    }
}
