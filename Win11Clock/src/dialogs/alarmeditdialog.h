#pragma once

#include <QDialog>
#include <QList>
#include <QStringList>
#include <QTime>

class QCheckBox;
class QLineEdit;
class QSpinBox;

class AlarmEditDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AlarmEditDialog(QWidget* parent = nullptr);

    void setAlarm(const QTime& time,
                  const QString& name,
                  const QStringList& repeatDays,
                  bool enabled);

    QTime alarmTime() const;
    QString alarmName() const;
    QStringList repeatDays() const;
    bool alarmEnabled() const;

private:
    QSpinBox* m_hour{nullptr};
    QSpinBox* m_minute{nullptr};
    QLineEdit* m_nameEdit{nullptr};
    QCheckBox* m_enabledSwitch{nullptr};
    QList<QCheckBox*> m_dayChecks;
};
