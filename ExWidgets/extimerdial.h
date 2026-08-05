#pragma once

#include <QProgressBar>

#include "exwidgets_global.h"

class QResizeEvent;
class QEvent;

class EXWIDGETS_EXPORT ExTimerDial final : public QProgressBar
{
    Q_OBJECT
    Q_PROPERTY(qint64 remainingMilliseconds READ remainingMilliseconds WRITE setRemainingMilliseconds NOTIFY remainingMillisecondsChanged)
    Q_PROPERTY(qint64 totalMilliseconds READ totalMilliseconds WRITE setTotalMilliseconds NOTIFY totalMillisecondsChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool finishTimeVisible READ isFinishTimeVisible WRITE setFinishTimeVisible NOTIFY finishTimeVisibleChanged)

public:
    explicit ExTimerDial(QWidget* parent = nullptr);

    qint64 remainingMilliseconds() const;
    qint64 totalMilliseconds() const;
    bool isRunning() const;
    bool isFinishTimeVisible() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setTime(qint64 remainingMilliseconds,
                 qint64 totalMilliseconds,
                 bool running);
    void setRemainingMilliseconds(qint64 milliseconds);
    void setTotalMilliseconds(qint64 milliseconds);
    void setRunning(bool running);
    void setFinishTimeVisible(bool visible);

signals:
    void remainingMillisecondsChanged(qint64 milliseconds);
    void totalMillisecondsChanged(qint64 milliseconds);
    void runningChanged(bool running);
    void finishTimeVisibleChanged(bool visible);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void syncProgressValue();
    void syncRingThickness();
    void syncTrackPalette();

    qint64 m_remainingMilliseconds{60 * 1000};
    qint64 m_totalMilliseconds{60 * 1000};
    bool m_running{false};
    bool m_finishTimeVisible{true};
    bool m_syncingTrackPalette{false};
};
