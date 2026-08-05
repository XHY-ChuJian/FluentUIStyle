#include "extimerdial.h"

#include "fluentui3styleproperties.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QResizeEvent>
#include <QTime>

#include <limits>

namespace
{
constexpr int ProgressResolution = 10000;
constexpr int TrackAlpha = 105;

QString formatDuration(qint64 milliseconds)
{
    const qint64 safeMilliseconds = qMax<qint64>(0, milliseconds);
    const qint64 totalSeconds = safeMilliseconds / 1000
                                + (safeMilliseconds % 1000 != 0 ? 1 : 0);
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 seconds = totalSeconds % 60;

    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
}

ExTimerDial::ExTimerDial(QWidget* parent)
    : QProgressBar(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setProperty(ProgressBarStyleProperty, ProgressBarRing);
    setTextVisible(false);
    setRange(0, ProgressResolution);
    syncTrackPalette();
    syncRingThickness();
    syncProgressValue();
}

qint64 ExTimerDial::remainingMilliseconds() const
{
    return m_remainingMilliseconds;
}

qint64 ExTimerDial::totalMilliseconds() const
{
    return m_totalMilliseconds;
}

bool ExTimerDial::isRunning() const
{
    return m_running;
}

bool ExTimerDial::isFinishTimeVisible() const
{
    return m_finishTimeVisible;
}

QSize ExTimerDial::sizeHint() const
{
    return QSize(260, 260);
}

QSize ExTimerDial::minimumSizeHint() const
{
    return QSize(190, 190);
}

void ExTimerDial::setTime(qint64 remainingMilliseconds,
                          qint64 totalMilliseconds,
                          bool running)
{
    const qint64 newRemaining = qMax<qint64>(0, remainingMilliseconds);
    const qint64 newTotal = qMax<qint64>(1, totalMilliseconds);
    const bool remainingChanged = m_remainingMilliseconds != newRemaining;
    const bool totalChanged = m_totalMilliseconds != newTotal;
    const bool stateChanged = m_running != running;

    if (!remainingChanged && !totalChanged && !stateChanged)
        return;

    m_remainingMilliseconds = newRemaining;
    m_totalMilliseconds = newTotal;
    m_running = running;

    if (remainingChanged)
        emit remainingMillisecondsChanged(m_remainingMilliseconds);
    if (totalChanged)
        emit totalMillisecondsChanged(m_totalMilliseconds);
    if (stateChanged)
        emit runningChanged(m_running);
    syncProgressValue();
    update();
}

void ExTimerDial::setRemainingMilliseconds(qint64 milliseconds)
{
    setTime(milliseconds, m_totalMilliseconds, m_running);
}

void ExTimerDial::setTotalMilliseconds(qint64 milliseconds)
{
    setTime(m_remainingMilliseconds, milliseconds, m_running);
}

void ExTimerDial::setRunning(bool running)
{
    setTime(m_remainingMilliseconds, m_totalMilliseconds, running);
}

void ExTimerDial::setFinishTimeVisible(bool visible)
{
    if (m_finishTimeVisible == visible)
        return;

    m_finishTimeVisible = visible;
    emit finishTimeVisibleChanged(m_finishTimeVisible);
    update();
}

void ExTimerDial::paintEvent(QPaintEvent* event)
{
    QProgressBar::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const int side = qMin(width(), height());
    const qreal ringWidth = property(ProgressBarThicknessProperty).toDouble();
    const QRectF ringRect((width() - side) / 2.0 + ringWidth,
                          (height() - side) / 2.0 + ringWidth,
                          side - ringWidth * 2.0,
                          side - ringWidth * 2.0);

    QFont timeFont = font();
    timeFont.setFamily(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(qBound(28, qRound(side * 0.17), 48));
    timeFont.setWeight(QFont::DemiBold);
    painter.setFont(timeFont);
    painter.setPen(palette().color(QPalette::WindowText));

    QRectF textRect = ringRect;
    if (m_running && m_finishTimeVisible)
        textRect.translate(0, -12);
    painter.drawText(textRect,
                     Qt::AlignCenter,
                     formatDuration(m_remainingMilliseconds));

    if (m_running && m_finishTimeVisible)
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

void ExTimerDial::resizeEvent(QResizeEvent* event)
{
    QProgressBar::resizeEvent(event);
    syncRingThickness();
}

void ExTimerDial::changeEvent(QEvent* event)
{
    QProgressBar::changeEvent(event);
    if (!m_syncingTrackPalette
        && (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange))
    {
        syncTrackPalette();
    }
}

void ExTimerDial::syncProgressValue()
{
    const bool showProgress = m_running || m_remainingMilliseconds < m_totalMilliseconds;
    if (!showProgress)
    {
        QProgressBar::setValue(0);
        return;
    }

    const qreal fraction = qBound<qreal>(
        0.0,
        static_cast<qreal>(m_remainingMilliseconds)
            / static_cast<qreal>(m_totalMilliseconds),
        1.0);
    QProgressBar::setValue(qRound(fraction * ProgressResolution));
}

void ExTimerDial::syncRingThickness()
{
    const int side = qMin(width(), height());
    const qreal thickness = qMax<qreal>(10.0, side * 0.055);
    if (!qFuzzyCompare(property(ProgressBarThicknessProperty).toDouble(), thickness))
        setProperty(ProgressBarThicknessProperty, thickness);
}

void ExTimerDial::syncTrackPalette()
{
    const QPalette applicationPalette = QApplication::palette(this);
    QPalette timerPalette = palette();

    const QPalette::ColorGroup groups[] = {
        QPalette::Active,
        QPalette::Inactive,
        QPalette::Disabled
    };
    for (QPalette::ColorGroup group : groups)
    {
        QColor trackColor = applicationPalette.color(group, QPalette::Mid);
        trackColor.setAlpha(TrackAlpha);
        timerPalette.setColor(group, QPalette::Mid, trackColor);
    }

    m_syncingTrackPalette = true;
    setPalette(timerPalette);
    m_syncingTrackPalette = false;
}
