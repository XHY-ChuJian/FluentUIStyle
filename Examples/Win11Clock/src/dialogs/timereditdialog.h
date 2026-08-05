#pragma once

#include <QDialog>

class QLineEdit;
class QSpinBox;

class TimerEditDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit TimerEditDialog(QWidget* parent = nullptr);

    void setTimer(const QString& name, qint64 durationMilliseconds);
    QString timerName() const;
    qint64 durationMilliseconds() const;

private:
    QSpinBox* m_hours{nullptr};
    QSpinBox* m_minutes{nullptr};
    QSpinBox* m_seconds{nullptr};
    QLineEdit* m_nameEdit{nullptr};
};
