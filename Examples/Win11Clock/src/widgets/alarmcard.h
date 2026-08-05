#pragma once

#include "widgets/cardwidget.h"

#include <QStringList>
#include <QTime>

class QCheckBox;
class QLabel;
class QToolButton;

class AlarmCard final : public CardWidget
{
    Q_OBJECT

public:
    explicit AlarmCard(const QTime& time,
                       const QString& name,
                       const QStringList& repeatDays,
                       bool enabled,
                       QWidget* parent = nullptr);

    QTime alarmTime() const;
    QString alarmName() const;
    QStringList repeatDays() const;
    bool isAlarmEnabled() const;

    void setAlarm(const QTime& time,
                  const QString& name,
                  const QStringList& repeatDays,
                  bool enabled);

signals:
    void editRequested(AlarmCard* alarm);
    void removeRequested(AlarmCard* alarm);
    void enabledChanged(bool enabled);

private:
    void refreshUi();

    QTime m_time;
    QString m_name;
    QStringList m_repeatDays;
    QLabel* m_timeLabel{nullptr};
    QLabel* m_nextLabel{nullptr};
    QLabel* m_nameLabel{nullptr};
    QLabel* m_daysLabel{nullptr};
    QCheckBox* m_enabledSwitch{nullptr};
    QToolButton* m_editButton{nullptr};
    QToolButton* m_deleteButton{nullptr};
};
