#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QWidget>

class FluentRoundButton;
class QLabel;
class QListWidget;
class QSpinBox;
class QTimer;
class QVBoxLayout;

class FocusPage final : public QWidget
{
    Q_OBJECT

public:
    explicit FocusPage(QWidget* parent = nullptr);

private:
    void toggleSession();
    void resetSession();
    void updateSession();
    void refreshUi();

    QLabel* m_timeLabel{nullptr};
    QLabel* m_statusLabel{nullptr};
    QSpinBox* m_minutesSpin{nullptr};
    FluentRoundButton* m_startButton{nullptr};
    QTimer* m_timer{nullptr};
    QElapsedTimer m_elapsed;
    qint64 m_totalMilliseconds{25 * 60 * 1000};
    qint64 m_remainingMilliseconds{25 * 60 * 1000};
    qint64 m_startedWithRemaining{25 * 60 * 1000};
    bool m_running{false};
};

class StopwatchPage final : public QWidget
{
    Q_OBJECT

public:
    explicit StopwatchPage(QWidget* parent = nullptr);

private:
    void toggleRunning();
    void reset();
    void addLap();
    void refreshTime();

    QLabel* m_timeLabel{nullptr};
    QLabel* m_lapLabel{nullptr};
    QListWidget* m_laps{nullptr};
    FluentRoundButton* m_startButton{nullptr};
    FluentRoundButton* m_lapButton{nullptr};
    QTimer* m_timer{nullptr};
    QElapsedTimer m_elapsed;
    qint64 m_accumulatedMilliseconds{0};
    bool m_running{false};
};

class WorldClockPage final : public QWidget
{
    Q_OBJECT

public:
    explicit WorldClockPage(QWidget* parent = nullptr);

private:
    struct CityClock
    {
        QByteArray timeZoneId;
        QLabel* timeLabel{nullptr};
        QLabel* dateLabel{nullptr};
    };

    void addCity(QVBoxLayout* layout,
                 const QString& city,
                 const QByteArray& timeZoneId);
    void refreshTimes();

    QList<CityClock> m_cities;
};

class AccountPage final : public QWidget
{
    Q_OBJECT

public:
    explicit AccountPage(QWidget* parent = nullptr);
};
