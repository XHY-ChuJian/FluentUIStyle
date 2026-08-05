#include "widgets/timercard.h"

#include "common/fluenthelpers.h"
#include "widgets/timerdial.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

TimerCard::TimerCard(const QString& name,
                     qint64 durationMilliseconds,
                     QWidget* parent)
    : CardWidget(parent)
    , m_durationMilliseconds(qMax<qint64>(1000, durationMilliseconds))
    , m_remainingMilliseconds(m_durationMilliseconds)
    , m_startedWithRemaining(m_durationMilliseconds)
{
    setMinimumSize(330, 340);
    setMaximumWidth(430);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 14, 18, 18);
    rootLayout->setSpacing(8);

    auto* headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(6);
    m_nameLabel = new QLabel(name, this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPixelSize(16);
    nameFont.setWeight(QFont::DemiBold);
    m_nameLabel->setFont(nameFont);

    m_editButton = new QToolButton(this);
    m_editButton->setText(QStringLiteral("\uE70F"));
    m_editButton->setFont(ClockUi::fluentIconFont());
    m_editButton->setToolTip(tr("编辑计时器"));

    m_deleteButton = new QToolButton(this);
    m_deleteButton->setText(QStringLiteral("\uE74D"));
    m_deleteButton->setFont(ClockUi::fluentIconFont());
    m_deleteButton->setToolTip(tr("删除计时器"));

    headerLayout->addWidget(m_nameLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_editButton);
    headerLayout->addWidget(m_deleteButton);

    m_dial = new TimerDial(this);

    auto* controlsLayout = new QHBoxLayout;
    controlsLayout->setAlignment(Qt::AlignCenter);
    controlsLayout->setSpacing(12);
    m_startButton = new FluentRoundButton(QStringLiteral("\uE768"), true, this);
    m_startButton->setToolTip(tr("开始"));
    m_resetButton = new FluentRoundButton(QStringLiteral("\uE777"), false, this);
    m_resetButton->setToolTip(tr("重置"));
    controlsLayout->addWidget(m_startButton);
    controlsLayout->addWidget(m_resetButton);

    rootLayout->addLayout(headerLayout);
    rootLayout->addWidget(m_dial, 1);
    rootLayout->addLayout(controlsLayout);

    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(100);
    m_tickTimer->setTimerType(Qt::PreciseTimer);

    connect(m_startButton, &QAbstractButton::clicked, this, &TimerCard::toggleRunning);
    connect(m_resetButton, &QAbstractButton::clicked, this, &TimerCard::resetTimer);
    connect(m_editButton, &QToolButton::clicked, this, [this] { emit editRequested(this); });
    connect(m_deleteButton, &QToolButton::clicked, this, [this] { emit removeRequested(this); });
    connect(m_tickTimer, &QTimer::timeout, this, &TimerCard::updateRemainingTime);

    refreshUi();
}

QString TimerCard::timerName() const
{
    return m_nameLabel->text();
}

qint64 TimerCard::durationMilliseconds() const
{
    return m_durationMilliseconds;
}

bool TimerCard::isRunning() const
{
    return m_running;
}

void TimerCard::setTimer(const QString& name, qint64 durationMilliseconds)
{
    m_running = false;
    m_tickTimer->stop();
    m_durationMilliseconds = qMax<qint64>(1000, durationMilliseconds);
    m_remainingMilliseconds = m_durationMilliseconds;
    m_startedWithRemaining = m_durationMilliseconds;
    m_nameLabel->setText(name.trimmed().isEmpty() ? tr("计时器") : name.trimmed());
    refreshUi();
    emit runningStateChanged(false);
}

void TimerCard::toggleRunning()
{
    if (m_running)
    {
        updateRemainingTime();
        m_running = false;
        m_tickTimer->stop();
    }
    else
    {
        if (m_remainingMilliseconds <= 0)
            m_remainingMilliseconds = m_durationMilliseconds;
        m_startedWithRemaining = m_remainingMilliseconds;
        m_elapsedTimer.restart();
        m_running = true;
        m_tickTimer->start();
    }

    refreshUi();
    emit runningStateChanged(m_running);
}

void TimerCard::resetTimer()
{
    m_tickTimer->stop();
    m_running = false;
    m_remainingMilliseconds = m_durationMilliseconds;
    m_startedWithRemaining = m_durationMilliseconds;
    refreshUi();
    emit runningStateChanged(false);
}

void TimerCard::updateRemainingTime()
{
    if (!m_running)
        return;

    m_remainingMilliseconds = qMax<qint64>(
        0,
        m_startedWithRemaining - m_elapsedTimer.elapsed());

    if (m_remainingMilliseconds == 0)
    {
        m_running = false;
        m_tickTimer->stop();
        QApplication::beep();
        emit runningStateChanged(false);
    }
    refreshUi();
}

void TimerCard::refreshUi()
{
    m_startButton->setGlyph(m_running ? QStringLiteral("\uE769")
                                      : QStringLiteral("\uE768"));
    m_startButton->setToolTip(m_running ? tr("暂停") : tr("开始"));
    m_resetButton->setEnabled(m_remainingMilliseconds != m_durationMilliseconds);
    m_dial->setTime(m_remainingMilliseconds,
                    m_durationMilliseconds,
                    m_running);
}
