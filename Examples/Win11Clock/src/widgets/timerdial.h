#pragma once

#include <QWidget>

class TimerDial final : public QWidget
{
    Q_OBJECT

public:
    explicit TimerDial(QWidget* parent = nullptr);

    void setTime(qint64 remainingMilliseconds,
                 qint64 totalMilliseconds,
                 bool running);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qint64 m_remainingMilliseconds{60 * 1000};
    qint64 m_totalMilliseconds{60 * 1000};
    bool m_running{false};
};
