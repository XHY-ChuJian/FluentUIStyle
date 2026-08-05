#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class QGridLayout;
class TimerCard;

class TimerPage final : public QWidget
{
    Q_OBJECT

public:
    explicit TimerPage(QWidget* parent = nullptr);

private:
    void addTimer(const QString& name, qint64 durationMilliseconds);
    void editTimer(TimerCard* timer);
    void removeTimer(TimerCard* timer);
    void rebuildGrid();

    QWidget* m_gridContainer{nullptr};
    QGridLayout* m_gridLayout{nullptr};
    QList<TimerCard*> m_timers;
};
