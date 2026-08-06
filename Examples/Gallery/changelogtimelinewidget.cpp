#include "changelogtimelinewidget.h"

#include <extimeline.h>

#include <QDateTime>
#include <QFile>
#include <QFont>
#include <QLabel>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>

#include <utility>

namespace
{
struct ChangelogEntry
{
    QDateTime timestamp;
    QString title;
    QString description;
};

QList<ChangelogEntry> loadChangelogEntries()
{
    QFile file( QStringLiteral( ":/changelog.txt" ) );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        return {};
    }

    QTextStream stream( &file );
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    stream.setCodec( "UTF-8" );
#endif

    const QRegularExpression headerExpression(
        QStringLiteral( "^【更新内容(\\d{4})-(\\d{1,2})-(\\d{1,2})】$" ) );
    const QRegularExpression itemPrefixExpression(
        QStringLiteral( "^\\s*\\d+[\\.、]?\\s*" ) );
    QList<ChangelogEntry> entries;
    QDate currentDate;
    QStringList changes;

    const auto appendCurrentEntry = [&]
    {
        if ( !currentDate.isValid() || changes.isEmpty() )
        {
            return;
        }

        QStringList normalizedChanges;
        normalizedChanges.reserve( changes.size() );
        for ( QString change : std::as_const( changes ) )
        {
            change.remove( itemPrefixExpression );
            if ( !change.isEmpty() )
            {
                normalizedChanges.append( change );
            }
        }
        if ( normalizedChanges.isEmpty() )
        {
            return;
        }

        ChangelogEntry entry;
        entry.timestamp = QDateTime( currentDate, QTime( 0, 0 ) );
        entry.title = normalizedChanges.takeFirst();
        for ( QString& change : normalizedChanges )
        {
            change.prepend( QStringLiteral( "• " ) );
        }
        entry.description = normalizedChanges.join( QLatin1Char( '\n' ) );
        entries.append( entry );
    };

    while ( !stream.atEnd() )
    {
        const QString line = stream.readLine().trimmed();
        const QRegularExpressionMatch match = headerExpression.match( line );
        if ( match.hasMatch() )
        {
            appendCurrentEntry();
            currentDate = QDate( match.captured( 1 ).toInt(),
                                 match.captured( 2 ).toInt(),
                                 match.captured( 3 ).toInt() );
            changes.clear();
        }
        else if ( currentDate.isValid() && !line.isEmpty() )
        {
            changes.append( line );
        }
    }
    appendCurrentEntry();
    return entries;
}
}

ChangelogTimelineWidget::ChangelogTimelineWidget( QWidget* parent )
    : QFrame( parent )
{
    setFrameShape( QFrame::StyledPanel );

    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 16, 16, 16, 16 );
    layout->setSpacing( 10 );

    auto* title = new QLabel( tr( "更新日志" ), this );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    layout->addWidget( title );

    auto* description = new QLabel( tr( "使用 ExTimeline 按时间展示项目的主要更新。" ), this );
    description->setProperty( "isSecondaryText", true );
    layout->addWidget( description );

    auto* card = new QFrame( this );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    auto* cardLayout = new QVBoxLayout( card );
    cardLayout->setContentsMargins( 16, 16, 16, 16 );

    auto* timeline = new ExTimeline( card );
    timeline->setObjectName( QStringLiteral( "changelogTimeline" ) );
    timeline->setLayoutMode( ExTimeline::ContentOnRight );
    timeline->setTimestampFormat( QStringLiteral( "yyyy-MM-dd" ) );
    timeline->setTimestampWidth( 100 );

    const QList<ChangelogEntry> entries = loadChangelogEntries();
    for ( int index = 0; index < entries.size(); ++index )
    {
        const ChangelogEntry& entry = entries.at( index );
        timeline->addEvent( entry.timestamp,
                            entry.title,
                            entry.description,
                            index == 0 ? ExTimelineEvent::Current : ExTimelineEvent::Completed );
    }
    if ( entries.isEmpty() )
    {
        timeline->addEvent( QDateTime(),
                            tr( "无法读取 changelog.txt" ),
                            tr( "请检查 Gallery 资源文件配置。" ),
                            ExTimelineEvent::Error );
    }

    cardLayout->addWidget( timeline );
    layout->addWidget( card, 1 );
}
