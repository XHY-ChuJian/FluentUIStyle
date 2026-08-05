#include "dialogs/alarmeditdialog.h"

#include "common/fluenthelpers.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

AlarmEditDialog::AlarmEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("编辑闹钟"));
    setModal(true);
    setMinimumWidth(430);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(18);

    auto* titleLayout = new QHBoxLayout;
    titleLayout->addWidget(ClockUi::createSectionTitle(tr("编辑闹钟"), this));
    titleLayout->addStretch();
    m_enabledSwitch = new QCheckBox(this);
    m_enabledSwitch->setProperty("isSwitchButton", true);
    m_enabledSwitch->setChecked(true);
    titleLayout->addWidget(m_enabledSwitch);
    rootLayout->addLayout(titleLayout);

    auto createTimeSpin = [this](int maximum)
    {
        auto* spin = new QSpinBox(this);
        spin->setRange(0, maximum);
        spin->setAlignment(Qt::AlignCenter);
        spin->setProperty("spinBoxButtonLayout", 2);
        QFont font = spin->font();
        font.setPixelSize(34);
        font.setWeight(QFont::DemiBold);
        spin->setFont(font);
        spin->setMinimumSize(200, 78);
        return spin;
    };

    m_hour = createTimeSpin(23);
    m_minute = createTimeSpin(59);
    auto* separator = new QLabel(QStringLiteral(":"), this);
    QFont separatorFont = separator->font();
    separatorFont.setPixelSize(32);
    separator->setFont(separatorFont);

    auto* timeLayout = new QHBoxLayout;
    timeLayout->addStretch();
    timeLayout->addWidget(m_hour);
    timeLayout->addWidget(separator);
    timeLayout->addWidget(m_minute);
    timeLayout->addStretch();
    rootLayout->addLayout(timeLayout);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("闹钟名称"));
    m_nameEdit->setClearButtonEnabled(true);
    rootLayout->addWidget(m_nameEdit);

    auto* repeatCheck = new QCheckBox(tr("重复闹钟"), this);
    repeatCheck->setChecked(true);
    rootLayout->addWidget(repeatCheck);

    const QStringList dayNames = {
        tr("一"), tr("二"), tr("三"), tr("四"), tr("五"), tr("六"), tr("日")};
    auto* daysLayout = new QHBoxLayout;
    for (const QString& dayName : dayNames)
    {
        auto* check = new QCheckBox(dayName, this);
        check->setChecked(true);
        m_dayChecks.append(check);
        daysLayout->addWidget(check);
    }
    rootLayout->addLayout(daysLayout);

    connect(repeatCheck,
            &QCheckBox::toggled,
            this,
            [this](bool checked)
            {
                for (QCheckBox* dayCheck : m_dayChecks)
                {
                    dayCheck->setEnabled(checked);
                    if (!checked)
                        dayCheck->setChecked(false);
                }
            });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save
                                             | QDialogButtonBox::Cancel,
                                         this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    rootLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AlarmEditDialog::setAlarm(const QTime& time,
                               const QString& name,
                               const QStringList& repeatDays,
                               bool enabled)
{
    const QTime safeTime = time.isValid() ? time : QTime(7, 0);
    m_hour->setValue(safeTime.hour());
    m_minute->setValue(safeTime.minute());
    m_nameEdit->setText(name);
    m_enabledSwitch->setChecked(enabled);

    for (QCheckBox* check : m_dayChecks)
        check->setChecked(repeatDays.contains(check->text()));
}

QTime AlarmEditDialog::alarmTime() const
{
    return QTime(m_hour->value(), m_minute->value());
}

QString AlarmEditDialog::alarmName() const
{
    const QString name = m_nameEdit->text().trimmed();
    return name.isEmpty() ? tr("闹钟") : name;
}

QStringList AlarmEditDialog::repeatDays() const
{
    QStringList days;
    for (QCheckBox* check : m_dayChecks)
    {
        if (check->isChecked())
            days.append(check->text());
    }
    return days;
}

bool AlarmEditDialog::alarmEnabled() const
{
    return m_enabledSwitch->isChecked();
}
