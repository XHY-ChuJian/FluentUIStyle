#pragma once

#include "widgets/cardwidget.h"

#include <QElapsedTimer>

class FluentRoundButton;
class QLabel;
class QTimer;
class QToolButton;
class TimerDial;

class TimerCard final : public CardWidget
{
    Q_OBJECT

public:
    explicit TimerCard(const QString& name,
                       qint64 durationMilliseconds,
                       QWidget* parent = nullptr);

    QString timerName() const;
    qint64 durationMilliseconds() const;
    bool isRunning() const;

    void setTimer(const QString& name, qint64 durationMilliseconds);

signals:
    void editRequested(TimerCard* timer);
    void removeRequested(TimerCard* timer);
    void runningStateChanged(bool running);

private:
    void toggleRunning();
    void resetTimer();
    void updateRemainingTime();
    void refreshUi();

    QLabel* m_nameLabel{nullptr};
    QToolButton* m_editButton{nullptr};
    QToolButton* m_deleteButton{nullptr};
    TimerDial* m_dial{nullptr};
    FluentRoundButton* m_startButton{nullptr};
    FluentRoundButton* m_resetButton{nullptr};
    QTimer* m_tickTimer{nullptr};
    QElapsedTimer m_elapsedTimer;
    qint64 m_durationMilliseconds{60 * 1000};
    qint64 m_remainingMilliseconds{60 * 1000};
    qint64 m_startedWithRemaining{60 * 1000};
    bool m_running{false};
};
