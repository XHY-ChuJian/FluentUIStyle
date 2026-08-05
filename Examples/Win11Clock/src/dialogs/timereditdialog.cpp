#include "dialogs/timereditdialog.h"

#include "common/fluenthelpers.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TimerEditDialog::TimerEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("编辑计时器"));
    setModal(true);
    setMinimumWidth(430);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(20);
    rootLayout->addWidget(ClockUi::createSectionTitle(tr("编辑计时器"), this));

    auto createTimeSpin = [this](int maximum)
    {
        auto* spin = new QSpinBox(this);
        spin->setRange(0, maximum);
        spin->setAlignment(Qt::AlignCenter);
        spin->setProperty("spinBoxButtonLayout", 2);
        QFont font = spin->font();
        font.setPixelSize(30);
        font.setWeight(QFont::DemiBold);
        spin->setFont(font);
        spin->setMinimumSize(200, 72);
        return spin;
    };

    m_hours = createTimeSpin(99);
    m_minutes = createTimeSpin(59);
    m_seconds = createTimeSpin(59);

    auto* timeLayout = new QHBoxLayout;
    timeLayout->setSpacing(10);
    auto* firstSeparator = new QLabel(QStringLiteral(":"), this);
    auto* secondSeparator = new QLabel(QStringLiteral(":"), this);
    QFont separatorFont = firstSeparator->font();
    separatorFont.setPixelSize(28);
    firstSeparator->setFont(separatorFont);
    secondSeparator->setFont(separatorFont);
    timeLayout->addWidget(m_hours);
    timeLayout->addWidget(firstSeparator);
    timeLayout->addWidget(m_minutes);
    timeLayout->addWidget(secondSeparator);
    timeLayout->addWidget(m_seconds);
    rootLayout->addLayout(timeLayout);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("计时器名称"));
    m_nameEdit->setClearButtonEnabled(true);
    rootLayout->addWidget(m_nameEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save
                                             | QDialogButtonBox::Cancel,
                                         this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    rootLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]
    {
        if (durationMilliseconds() <= 0)
        {
            QMessageBox::information(this,
                                     tr("计时器"),
                                     tr("计时时长至少需要 1 秒。"));
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TimerEditDialog::setTimer(const QString& name, qint64 durationMilliseconds)
{
    const qint64 totalSeconds = qMax<qint64>(0, durationMilliseconds / 1000);
    m_hours->setValue(static_cast<int>(totalSeconds / 3600));
    m_minutes->setValue(static_cast<int>((totalSeconds / 60) % 60));
    m_seconds->setValue(static_cast<int>(totalSeconds % 60));
    m_nameEdit->setText(name);
}

QString TimerEditDialog::timerName() const
{
    const QString name = m_nameEdit->text().trimmed();
    return name.isEmpty() ? tr("计时器") : name;
}

qint64 TimerEditDialog::durationMilliseconds() const
{
    const qint64 seconds = static_cast<qint64>(m_hours->value()) * 3600
                           + static_cast<qint64>(m_minutes->value()) * 60
                           + m_seconds->value();
    return seconds * 1000;
}
