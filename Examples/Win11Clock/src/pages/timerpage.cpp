#include "pages/timerpage.h"

#include "common/fluenthelpers.h"
#include "dialogs/timereditdialog.h"
#include "widgets/timercard.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <exmessagebox.h>
#include <QVariant>

TimerPage::TimerPage(QWidget* parent)
    : QWidget(parent)
{
    auto* pageLayout = ClockUi::createPageLayout(this);

    auto* headerLayout = new QHBoxLayout;
    headerLayout->addWidget(ClockUi::createPageTitle(tr("计时器"), this));
    headerLayout->addStretch();
    auto* addButton = new QPushButton(tr("＋  新建计时器"), this);
    addButton->setProperty("accent", true);
    headerLayout->addWidget(addButton);
    pageLayout->addLayout(headerLayout);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_gridContainer = new QWidget(scrollArea);
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 8, 12);
    m_gridLayout->setHorizontalSpacing(16);
    m_gridLayout->setVerticalSpacing(16);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scrollArea->setWidget(m_gridContainer);

    m_gridContainer->setAutoFillBackground(false);
    scrollArea->setAutoFillBackground( false );
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->viewport()->setAutoFillBackground(false);

    pageLayout->addWidget(scrollArea, 1);

    connect(addButton,
            &QPushButton::clicked,
            this,
            [this]
            {
                TimerEditDialog dialog(this);
                dialog.setTimer(tr("新计时器"), 5 * 60 * 1000);
                if (dialog.exec() == QDialog::Accepted)
                    addTimer(dialog.timerName(), dialog.durationMilliseconds());
            });

    addTimer(tr("1 分钟"), 60 * 1000);
    addTimer(tr("3 分钟"), 3 * 60 * 1000);
    addTimer(tr("5 分钟"), 5 * 60 * 1000);
    addTimer(tr("10 分钟"), 10 * 60 * 1000);
}

void TimerPage::addTimer(const QString& name, qint64 durationMilliseconds)
{
    auto* timer = new TimerCard(name, durationMilliseconds, m_gridContainer);
    m_timers.append(timer);
    connect(timer, &TimerCard::editRequested, this, &TimerPage::editTimer);
    connect(timer, &TimerCard::removeRequested, this, &TimerPage::removeTimer);
    rebuildGrid();
}

void TimerPage::editTimer(TimerCard* timer)
{
    if (!timer)
        return;

    TimerEditDialog dialog(this);
    dialog.setTimer(timer->timerName(), timer->durationMilliseconds());
    if (dialog.exec() == QDialog::Accepted)
        timer->setTimer(dialog.timerName(), dialog.durationMilliseconds());
}

void TimerPage::removeTimer(TimerCard* timer)
{
    if (!timer)
        return;

    const QMessageBox::StandardButton answer = ExMessageBox::question(
        this,
        tr("删除计时器"),
        tr("确定删除“%1”吗？").arg(timer->timerName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    m_timers.removeAll(timer);
    delete timer;
    rebuildGrid();
}

void TimerPage::rebuildGrid()
{
    while (QLayoutItem* item = m_gridLayout->takeAt(0))
        delete item;

    constexpr int columnCount = 3;
    for (int index = 0; index < m_timers.size(); ++index)
        m_gridLayout->addWidget(m_timers.at(index), index / columnCount, index % columnCount);

    for (int column = 0; column < columnCount; ++column)
        m_gridLayout->setColumnStretch(column, 1);
}
