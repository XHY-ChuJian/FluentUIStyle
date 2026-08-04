#pragma once

#include <QList>
#include <QStringList>
#include <QTime>
#include <QWidget>

class AlarmCard;
class QGridLayout;

class AlarmPage final : public QWidget
{
    Q_OBJECT

public:
    explicit AlarmPage(QWidget* parent = nullptr);

private:
    void addAlarm(const QTime& time,
                  const QString& name,
                  const QStringList& repeatDays,
                  bool enabled);
    void editAlarm(AlarmCard* alarm);
    void removeAlarm(AlarmCard* alarm);
    void rebuildGrid();

    QWidget* m_gridContainer{nullptr};
    QGridLayout* m_gridLayout{nullptr};
    QList<AlarmCard*> m_alarms;
};
