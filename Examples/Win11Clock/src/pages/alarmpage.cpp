#include "pages/alarmpage.h"

#include <ExMessageBox.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTime>
#include <QVariant>
#include <QVBoxLayout>

#include "common/fluenthelpers.h"
#include "dialogs/alarmeditdialog.h"
#include "widgets/alarmcard.h"

AlarmPage::AlarmPage( QWidget* parent )
    : QWidget( parent )
{
    auto* pageLayout = ClockUi::createPageLayout( this );

    auto* headerLayout = new QHBoxLayout;
    headerLayout->addWidget( ClockUi::createPageTitle( tr( "闹钟" ), this ) );
    headerLayout->addStretch();
    auto* addButton = new QPushButton( tr( "＋  新建闹钟" ), this );
    addButton->setProperty( "accent", QVariant( true ) );
    headerLayout->addWidget( addButton );
    pageLayout->addLayout( headerLayout );

    auto* scrollArea = new QScrollArea( this );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_gridContainer = new QWidget( scrollArea );
    m_gridLayout    = new QGridLayout( m_gridContainer );
    m_gridLayout->setContentsMargins( 0, 0, 8, 12 );
    m_gridLayout->setSpacing( 16 );
    m_gridLayout->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    scrollArea->setWidget( m_gridContainer );

    m_gridContainer->setAutoFillBackground( false );
    scrollArea->setAutoFillBackground( false );
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->viewport()->setAutoFillBackground(false);

    pageLayout->addWidget( scrollArea, 1 );

    connect( addButton,
             &QPushButton::clicked,
             this,
             [ this ]
             {
                 AlarmEditDialog dialog( this );
                 dialog.setAlarm( QTime::currentTime().addSecs( 3600 ), tr( "新闹钟" ), {}, true );
                 if ( dialog.exec() == QDialog::Accepted )
                 {
                     addAlarm( dialog.alarmTime(), dialog.alarmName(), dialog.repeatDays(), dialog.alarmEnabled() );
                 }
             } );

    addAlarm( QTime( 7, 0 ), tr( "早上好" ), { tr( "一" ), tr( "二" ), tr( "三" ), tr( "四" ), tr( "五" ) }, true );
    addAlarm( QTime( 8, 30 ), tr( "周末起床" ), { tr( "六" ), tr( "日" ) }, false );
}

void AlarmPage::addAlarm( const QTime& time, const QString& name, const QStringList& repeatDays, bool enabled )
{
    auto* alarm = new AlarmCard( time, name, repeatDays, enabled, m_gridContainer );
    m_alarms.append( alarm );
    connect( alarm, &AlarmCard::editRequested, this, &AlarmPage::editAlarm );
    connect( alarm, &AlarmCard::removeRequested, this, &AlarmPage::removeAlarm );
    rebuildGrid();
}

void AlarmPage::editAlarm( AlarmCard* alarm )
{
    if ( !alarm )
    {
        return;
    }

    AlarmEditDialog dialog( this );
    dialog.setAlarm( alarm->alarmTime(), alarm->alarmName(), alarm->repeatDays(), alarm->isAlarmEnabled() );
    if ( dialog.exec() == QDialog::Accepted )
    {
        alarm->setAlarm( dialog.alarmTime(), dialog.alarmName(), dialog.repeatDays(), dialog.alarmEnabled() );
    }
}

void AlarmPage::removeAlarm( AlarmCard* alarm )
{
    if ( !alarm )
    {
        return;
    }

    const QMessageBox::StandardButton answer = ExMessageBox::question(
        this, tr( "删除闹钟" ), tr( "确定删除“%1”吗？" ).arg( alarm->alarmName() ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
    if ( answer != QMessageBox::Yes )
    {
        return;
    }

    m_alarms.removeAll( alarm );
    delete alarm;
    rebuildGrid();
}

void AlarmPage::rebuildGrid()
{
    while ( QLayoutItem* item = m_gridLayout->takeAt( 0 ) )
    {
        delete item;
    }

    constexpr int columnCount = 2;
    for ( int index = 0; index < m_alarms.size(); ++index )
    {
        m_gridLayout->addWidget( m_alarms.at( index ), index / columnCount, index % columnCount );
    }

    for ( int column = 0; column < columnCount; ++column )
    {
        m_gridLayout->setColumnStretch( column, 1 );
    }
}
