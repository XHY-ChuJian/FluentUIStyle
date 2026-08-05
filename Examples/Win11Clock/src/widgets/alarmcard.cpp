#include "widgets/alarmcard.h"

#include "common/fluenthelpers.h"

#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QVariant>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

AlarmCard::AlarmCard(const QTime& time,
                     const QString& name,
                     const QStringList& repeatDays,
                     bool enabled,
                     QWidget* parent)
    : CardWidget(parent)
    , m_time(time)
    , m_name(name)
    , m_repeatDays(repeatDays)
{
    setMinimumSize(360, 220);
    setMaximumWidth(520);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 14, 20, 18);
    rootLayout->setSpacing(8);

    auto* topLayout = new QHBoxLayout;
    topLayout->setSpacing(8);

    m_timeLabel = new QLabel(this);
    QFont timeFont(QStringLiteral("Segoe UI Variable Display"));
    timeFont.setPixelSize(58);
    timeFont.setWeight(QFont::Light);
    m_timeLabel->setFont(timeFont);

    auto* actionLayout = new QHBoxLayout;
    m_editButton = new QToolButton(this);
    m_editButton->setText(QStringLiteral("\uE70F"));
    m_editButton->setFont(ClockUi::fluentIconFont());
    m_editButton->setToolTip(tr("编辑闹钟"));
    m_deleteButton = new QToolButton(this);
    m_deleteButton->setText(QStringLiteral("\uE74D"));
    m_deleteButton->setFont(ClockUi::fluentIconFont());
    m_deleteButton->setToolTip(tr("删除闹钟"));
    actionLayout->addWidget(m_editButton);
    actionLayout->addWidget(m_deleteButton);

    m_enabledSwitch = new QCheckBox(this);
    m_enabledSwitch->setProperty("isSwitchButton", QVariant(true));
    m_enabledSwitch->setChecked(enabled);
    m_enabledSwitch->setToolTip(tr("启用闹钟"));

    topLayout->addWidget(m_timeLabel);
    topLayout->addStretch();
    topLayout->addLayout(actionLayout);
    topLayout->addWidget(m_enabledSwitch);

    m_nextLabel = new QLabel(this);
    m_nextLabel->setProperty("subtitle", QVariant(true));
    m_nameLabel = new QLabel(this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPixelSize(22);
    m_nameLabel->setFont(nameFont);
    m_daysLabel = new QLabel(this);

    rootLayout->addLayout(topLayout);
    rootLayout->addWidget(m_nextLabel);
    rootLayout->addWidget(m_nameLabel);
    rootLayout->addWidget(m_daysLabel);
    rootLayout->addStretch();

    connect(m_editButton, &QToolButton::clicked, this, [this] { emit editRequested(this); });
    connect(m_deleteButton, &QToolButton::clicked, this, [this] { emit removeRequested(this); });
    connect(m_enabledSwitch,
            &QCheckBox::toggled,
            this,
            [this](bool checked)
            {
                refreshUi();
                emit enabledChanged(checked);
            });

    refreshUi();
}

QTime AlarmCard::alarmTime() const
{
    return m_time;
}

QString AlarmCard::alarmName() const
{
    return m_name;
}

QStringList AlarmCard::repeatDays() const
{
    return m_repeatDays;
}

bool AlarmCard::isAlarmEnabled() const
{
    return m_enabledSwitch->isChecked();
}

void AlarmCard::setAlarm(const QTime& time,
                         const QString& name,
                         const QStringList& repeatDays,
                         bool enabled)
{
    m_time = time.isValid() ? time : QTime(7, 0);
    m_name = name.trimmed().isEmpty() ? tr("闹钟") : name.trimmed();
    m_repeatDays = repeatDays;
    m_enabledSwitch->setChecked(enabled);
    refreshUi();
}

void AlarmCard::refreshUi()
{
    m_timeLabel->setText(m_time.toString(QStringLiteral("H:mm")));
    m_nameLabel->setText(m_name);

    const bool enabled = m_enabledSwitch->isChecked();
    m_nextLabel->setText(enabled ? tr("\uE7ED  将在下一设定时间响铃")
                                 : tr("闹钟已关闭"));
    m_nextLabel->setEnabled(enabled);
    m_timeLabel->setEnabled(enabled);
    m_nameLabel->setEnabled(enabled);
    m_daysLabel->setText(m_repeatDays.isEmpty()
                             ? tr("仅一次")
                             : m_repeatDays.join(QStringLiteral("  ")));
}
