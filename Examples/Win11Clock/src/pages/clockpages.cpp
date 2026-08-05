#include "pages/clockpages.h"

#include "common/fluenthelpers.h"
#include "widgets/cardwidget.h"

#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVariant>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

FocusPage::FocusPage(QWidget* parent)
    : QWidget(parent)
{
    auto* pageLayout = ClockUi::createPageLayout(this);
    pageLayout->addWidget(ClockUi::createPageTitle(tr("专注时段"), this));

    auto* card = new CardWidget(this);
    card->setMaximumWidth(720);
    card->setMinimumHeight(460);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 32, 40, 32);
    cardLayout->setSpacing(18);
    cardLayout->setAlignment(Qt::AlignCenter);

    auto* heading = ClockUi::createSectionTitle(tr("准备好专注了吗？"), card);
    heading->setAlignment(Qt::AlignCenter);
    m_statusLabel = new QLabel(tr("选择时长，然后开始一个不被打扰的专注时段。"), card);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);

    m_timeLabel = new QLabel(card);
    QFont timeFont(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(64);
    timeFont.setWeight(QFont::Light);
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setAlignment(Qt::AlignCenter);

    m_minutesSpin = new QSpinBox(card);
    m_minutesSpin->setRange(1, 180);
    m_minutesSpin->setValue(25);
    m_minutesSpin->setSuffix(tr(" 分钟"));
    m_minutesSpin->setAlignment(Qt::AlignCenter);
    m_minutesSpin->setMinimumWidth(180);

    auto* controls = new QHBoxLayout;
    controls->setAlignment(Qt::AlignCenter);
    controls->setSpacing(12);
    m_startButton = new FluentRoundButton(QStringLiteral("\uE768"), true, card);
    auto* resetButton = new FluentRoundButton(QStringLiteral("\uE777"), false, card);
    controls->addWidget(m_startButton);
    controls->addWidget(resetButton);

    cardLayout->addWidget(heading);
    cardLayout->addWidget(m_statusLabel);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(m_timeLabel);
    cardLayout->addWidget(m_minutesSpin, 0, Qt::AlignHCenter);
    cardLayout->addLayout(controls);

    auto* cardRow = new QHBoxLayout;
    cardRow->addStretch();
    cardRow->addWidget(card, 1);
    cardRow->addStretch();
    pageLayout->addLayout(cardRow, 1);

    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    m_timer->setTimerType(Qt::PreciseTimer);

    connect(m_startButton, &QAbstractButton::clicked, this, &FocusPage::toggleSession);
    connect(resetButton, &QAbstractButton::clicked, this, &FocusPage::resetSession);
    connect(m_timer, &QTimer::timeout, this, &FocusPage::updateSession);
    connect(m_minutesSpin,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [this](int minutes)
            {
                if (m_running)
                    return;
                m_totalMilliseconds = static_cast<qint64>(minutes) * 60 * 1000;
                m_remainingMilliseconds = m_totalMilliseconds;
                refreshUi();
            });
    refreshUi();
}

void FocusPage::toggleSession()
{
    if (m_running)
    {
        updateSession();
        m_running = false;
        m_timer->stop();
    }
    else
    {
        if (m_remainingMilliseconds <= 0)
            m_remainingMilliseconds = m_totalMilliseconds;
        m_startedWithRemaining = m_remainingMilliseconds;
        m_elapsed.restart();
        m_running = true;
        m_timer->start();
    }
    refreshUi();
}

void FocusPage::resetSession()
{
    m_timer->stop();
    m_running = false;
    m_totalMilliseconds = static_cast<qint64>(m_minutesSpin->value()) * 60 * 1000;
    m_remainingMilliseconds = m_totalMilliseconds;
    refreshUi();
}

void FocusPage::updateSession()
{
    if (!m_running)
        return;

    m_remainingMilliseconds = qMax<qint64>(
        0,
        m_startedWithRemaining - m_elapsed.elapsed());
    if (m_remainingMilliseconds == 0)
    {
        m_running = false;
        m_timer->stop();
        QApplication::beep();
    }
    refreshUi();
}

void FocusPage::refreshUi()
{
    m_timeLabel->setText(ClockUi::formatDuration(m_remainingMilliseconds));
    m_statusLabel->setText(m_running ? tr("保持专注，你正在做得很好。")
                                     : tr("选择时长，然后开始一个不被打扰的专注时段。"));
    m_minutesSpin->setEnabled(!m_running);
    m_startButton->setGlyph(m_running ? QStringLiteral("\uE769")
                                      : QStringLiteral("\uE768"));
}

StopwatchPage::StopwatchPage(QWidget* parent)
    : QWidget(parent)
{
    auto* pageLayout = ClockUi::createPageLayout(this);
    pageLayout->addWidget(ClockUi::createPageTitle(tr("秒表"), this));

    m_timeLabel = new QLabel(QStringLiteral("00:00:00.00"), this);
    QFont timeFont(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(76);
    timeFont.setWeight(QFont::Light);
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    pageLayout->addWidget(m_timeLabel);

    auto* controls = new QHBoxLayout;
    controls->setAlignment(Qt::AlignCenter);
    controls->setSpacing(14);
    m_startButton = new FluentRoundButton(QStringLiteral("\uE768"), true, this);
    m_lapButton = new FluentRoundButton(QStringLiteral("\uE7C1"), false, this);
    auto* resetButton = new FluentRoundButton(QStringLiteral("\uE777"), false, this);
    controls->addWidget(m_startButton);
    controls->addWidget(m_lapButton);
    controls->addWidget(resetButton);
    pageLayout->addLayout(controls);

    m_lapLabel = ClockUi::createSectionTitle(tr("计次"), this);
    pageLayout->addWidget(m_lapLabel);
    m_laps = new QListWidget(this);
    m_laps->setAlternatingRowColors(true);
    pageLayout->addWidget(m_laps, 1);

    m_timer = new QTimer(this);
    m_timer->setInterval(10);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &StopwatchPage::refreshTime);
    connect(m_startButton, &QAbstractButton::clicked, this, &StopwatchPage::toggleRunning);
    connect(m_lapButton, &QAbstractButton::clicked, this, &StopwatchPage::addLap);
    connect(resetButton, &QAbstractButton::clicked, this, &StopwatchPage::reset);
    refreshTime();
}

void StopwatchPage::toggleRunning()
{
    if (m_running)
    {
        m_accumulatedMilliseconds += m_elapsed.elapsed();
        m_timer->stop();
        m_running = false;
    }
    else
    {
        m_elapsed.restart();
        m_timer->start();
        m_running = true;
    }
    m_startButton->setGlyph(m_running ? QStringLiteral("\uE769")
                                      : QStringLiteral("\uE768"));
    refreshTime();
}

void StopwatchPage::reset()
{
    m_timer->stop();
    m_running = false;
    m_accumulatedMilliseconds = 0;
    m_laps->clear();
    m_startButton->setGlyph(QStringLiteral("\uE768"));
    refreshTime();
}

void StopwatchPage::addLap()
{
    const qint64 elapsed = m_accumulatedMilliseconds
                           + (m_running ? m_elapsed.elapsed() : 0);
    if (elapsed <= 0)
        return;
    m_laps->insertItem(0,
                       tr("计次 %1    %2")
                           .arg(m_laps->count() + 1)
                           .arg(ClockUi::formatDuration(elapsed, true)));
}

void StopwatchPage::refreshTime()
{
    const qint64 elapsed = m_accumulatedMilliseconds
                           + (m_running ? m_elapsed.elapsed() : 0);
    m_timeLabel->setText(ClockUi::formatDuration(elapsed, true));
    m_lapButton->setEnabled(elapsed > 0);
}

WorldClockPage::WorldClockPage(QWidget* parent)
    : QWidget(parent)
{
    auto* pageLayout = ClockUi::createPageLayout(this);
    pageLayout->addWidget(ClockUi::createPageTitle(tr("世界时钟"), this));
    addCity(pageLayout, tr("北京"), QByteArrayLiteral("Asia/Shanghai"));
    addCity(pageLayout, tr("伦敦"), QByteArrayLiteral("Europe/London"));
    addCity(pageLayout, tr("纽约"), QByteArrayLiteral("America/New_York"));
    addCity(pageLayout, tr("东京"), QByteArrayLiteral("Asia/Tokyo"));
    pageLayout->addStretch();

    auto* timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &WorldClockPage::refreshTimes);
    timer->start();
    refreshTimes();
}

void WorldClockPage::addCity(QVBoxLayout* layout,
                             const QString& city,
                             const QByteArray& timeZoneId)
{
    auto* card = new CardWidget(this);
    card->setMinimumHeight(96);
    auto* row = new QHBoxLayout(card);
    row->setContentsMargins(22, 14, 22, 14);

    auto* cityLabel = ClockUi::createSectionTitle(city, card);
    auto* dateLabel = new QLabel(card);
    dateLabel->setProperty("subtitle", QVariant(true));
    auto* timeLabel = new QLabel(card);
    QFont timeFont(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(34);
    timeLabel->setFont(timeFont);

    auto* textColumn = new QVBoxLayout;
    textColumn->addWidget(cityLabel);
    textColumn->addWidget(dateLabel);
    row->addLayout(textColumn);
    row->addStretch();
    row->addWidget(timeLabel);
    layout->addWidget(card);

    m_cities.append({timeZoneId, timeLabel, dateLabel});
}

void WorldClockPage::refreshTimes()
{
    for (const CityClock& city : m_cities)
    {
        const QTimeZone zone(city.timeZoneId);
        const QDateTime now = QDateTime::currentDateTimeUtc().toTimeZone(zone);
        city.timeLabel->setText(now.time().toString(QStringLiteral("HH:mm:ss")));
        city.dateLabel->setText(now.date().toString(tr("yyyy年M月d日 dddd")));
    }
}

AccountPage::AccountPage(QWidget* parent)
    : QWidget(parent)
{
    auto* pageLayout = ClockUi::createPageLayout(this);
    pageLayout->addWidget(ClockUi::createPageTitle(tr("账户"), this));
    pageLayout->addWidget(ClockUi::createEmptyState(
        QStringLiteral("\uE77B"),
        tr("登录以同步时钟设置"),
        tr("使用 Microsoft 帐户可以在设备间同步闹钟和计时器。"),
        this),
        1);
}
