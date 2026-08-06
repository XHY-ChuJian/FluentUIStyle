#include "extimeline.h"

#include <QAbstractAnimation>
#include <QAbstractListModel>
#include <QEasingCurve>
#include <QEvent>
#include <QFontMetricsF>
#include <QHideEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QVariantAnimation>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr qreal TextGap            = 4.0;
constexpr qreal AxisGap            = 12.0;
constexpr qreal MinimumEventHeight = 44.0;

QPalette::ColorRole accentRole()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return QPalette::Accent;
#else
    return QPalette::Highlight;
#endif
}

QColor withAlpha( QColor color, int alpha )
{
    color.setAlpha( qBound( 0, alpha, 255 ) );
    return color;
}
}  // namespace

class ExTimelineModel final : public QAbstractListModel
{
public:
    explicit ExTimelineModel( QObject* parent = nullptr )
        : QAbstractListModel( parent )
    {
    }

    int rowCount( const QModelIndex& parent = QModelIndex() ) const override { return parent.isValid() ? 0 : m_events.size(); }

    QVariant data( const QModelIndex& index, int role ) const override
    {
        ExTimelineEvent* event = eventAt( index.row() );
        if ( !event )
        {
            return {};
        }
        if ( role == Qt::DisplayRole )
        {
            return event->title();
        }
        if ( role == Qt::ToolTipRole )
        {
            return event->description();
        }
        if ( role == Qt::AccessibleTextRole )
        {
            return event->description().isEmpty() ? event->title() : event->title() + QStringLiteral( ", " ) + event->description();
        }
        return {};
    }

    Qt::ItemFlags flags( const QModelIndex& index ) const override
    {
        return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags;
    }

    QList<ExTimelineEvent*> events() const { return m_events; }

    ExTimelineEvent* eventAt( int visualIndex ) const
    {
        if ( visualIndex < 0 || visualIndex >= m_events.size() )
        {
            return nullptr;
        }
        const int sourceIndex = m_reverse ? m_events.size() - 1 - visualIndex : visualIndex;
        return m_events.at( sourceIndex );
    }

    bool contains( const ExTimelineEvent* event ) const { return m_events.contains( const_cast<ExTimelineEvent*>( event ) ); }

    void appendEvent( ExTimelineEvent* event )
    {
        if ( !event || m_events.contains( event ) )
        {
            return;
        }
        const int visualRow = m_reverse ? 0 : m_events.size();
        beginInsertRows( QModelIndex(), visualRow, visualRow );
        m_events.append( event );
        endInsertRows();
    }

    bool takeEvent( ExTimelineEvent* event )
    {
        const int sourceIndex = m_events.indexOf( event );
        if ( sourceIndex < 0 )
        {
            return false;
        }
        const int visualRow = m_reverse ? m_events.size() - 1 - sourceIndex : sourceIndex;
        beginRemoveRows( QModelIndex(), visualRow, visualRow );
        m_events.removeAt( sourceIndex );
        endRemoveRows();
        return true;
    }

    QList<ExTimelineEvent*> takeAllEvents()
    {
        if ( m_events.isEmpty() )
        {
            return {};
        }
        beginResetModel();
        const QList<ExTimelineEvent*> oldEvents = m_events;
        m_events.clear();
        endResetModel();
        return oldEvents;
    }

    void notifyEventChanged( ExTimelineEvent* event )
    {
        const int sourceIndex = m_events.indexOf( event );
        if ( sourceIndex < 0 )
        {
            return;
        }
        const int visualRow            = m_reverse ? m_events.size() - 1 - sourceIndex : sourceIndex;
        const QModelIndex changedIndex = index( visualRow, 0 );
        emit dataChanged( changedIndex, changedIndex );
    }

    void setReverse( bool reverse )
    {
        if ( m_reverse == reverse )
        {
            return;
        }
        beginResetModel();
        m_reverse = reverse;
        endResetModel();
    }

private:
    QList<ExTimelineEvent*> m_events;
    bool m_reverse = false;
};

class ExTimelineDelegate final : public QStyledItemDelegate
{
public:
    ExTimelineDelegate( ExTimeline* timeline, ExTimelineModel* model )
        : QStyledItemDelegate( timeline )
        , m_timeline( timeline )
        , m_model( model )
    {
    }

    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        const ExTimelineEvent* event = m_model ? m_model->eventAt( index.row() ) : nullptr;
        if ( !m_timeline || !event )
        {
            return QStyledItemDelegate::sizeHint( option, index );
        }

        if ( m_timeline->orientation() == Qt::Horizontal )
        {
            const int availableHeight = qMax( 180, m_timeline->viewport()->height() );
            return QSize( m_timeline->horizontalItemWidth(), availableHeight );
        }

        const int availableWidth    = option.rect.width() > 0 ? option.rect.width() : qMax( 240, m_timeline->viewport()->width() );
        const qreal sideWidth       = contentSideWidth( availableWidth );
        const QFont titleFont       = resolvedTitleFont( option );
        const QFont descriptionFont = resolvedDescriptionFont( option );
        const QFont timestampFont   = resolvedTimestampFont( option );
        const QFontMetricsF titleMetrics( titleFont );
        const QFontMetricsF descriptionMetrics( descriptionFont );
        const QFontMetricsF timestampMetrics( timestampFont );

        qreal contentHeight = titleMetrics.height();
        if ( m_timeline->isDescriptionVisible() && !event->description().isEmpty() )
        {
            const QRectF bounds = descriptionMetrics.boundingRect( QRectF( 0.0, 0.0, qMax( 40.0, sideWidth ), 10000.0 ),
                                                                   Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                                                   event->description() );
            contentHeight += TextGap + bounds.height();
        }
        qreal timestampHeight = 0.0;
        if ( m_timeline->isTimestampVisible() && !m_timeline->formattedTimestamp( event ).isEmpty() )
        {
            timestampHeight = timestampMetrics.height();
        }
        const qreal bodyHeight = std::max( { contentHeight, timestampHeight, static_cast<qreal>( m_timeline->nodeSize() ) } );
        return QSize( availableWidth, qCeil( qMax( MinimumEventHeight, bodyHeight ) + m_timeline->itemSpacing() ) );
    }

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        const ExTimelineEvent* event = m_model ? m_model->eventAt( index.row() ) : nullptr;
        if ( !m_timeline || !event || !painter )
        {
            return;
        }

        if ( m_timeline->orientation() == Qt::Horizontal )
        {
            paintHorizontal( painter, option, index, event );
            return;
        }

        painter->save();
        painter->setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

        const QPalette::ColorGroup colorGroup =
            option.state.testFlag( QStyle::State_Enabled )
                ? ( option.state.testFlag( QStyle::State_Active ) ? QPalette::Active : QPalette::Inactive )
                : QPalette::Disabled;
        const QRectF rowRect       = option.rect;
        const QRectF highlightRect = rowRect.adjusted( 2.0, 1.0, -2.0, -2.0 );
        if ( option.state.testFlag( QStyle::State_Selected ) || option.state.testFlag( QStyle::State_MouseOver ) )
        {
            QColor background = option.palette.color( colorGroup, QPalette::Highlight );
            background.setAlpha( option.state.testFlag( QStyle::State_Selected ) ? 34 : 18 );
            painter->setPen( Qt::NoPen );
            painter->setBrush( background );
            painter->drawRoundedRect( highlightRect, 6.0, 6.0 );
        }

        const bool contentOnRight   = isContentOnRight( event, index.row() );
        const qreal axisX           = axisPosition( rowRect );
        const QFont titleFont       = resolvedTitleFont( option );
        const QFont descriptionFont = resolvedDescriptionFont( option );
        const QFont timestampFont   = resolvedTimestampFont( option );
        const QFontMetricsF titleMetrics( titleFont );
        const qreal top         = rowRect.top() + qMax( 4.0, m_timeline->itemSpacing() * 0.25 );
        const qreal nodeCenterY = top + qMax( titleMetrics.height(), static_cast<qreal>( m_timeline->nodeSize() ) ) * 0.5;
        const qreal gap         = m_timeline->nodeSize() * 0.5 + AxisGap;
        const qreal left        = rowRect.left() + m_timeline->contentPadding();
        const qreal right       = rowRect.right() - m_timeline->contentPadding();
        QRectF contentRect;
        QRectF timestampRect;
        if ( contentOnRight )
        {
            contentRect   = QRectF( axisX + gap, top, qMax( 0.0, right - axisX - gap ), rowRect.bottom() - top );
            timestampRect = QRectF( left, top, qMax( 0.0, axisX - gap - left ), titleMetrics.height() );
        }
        else
        {
            contentRect   = QRectF( left, top, qMax( 0.0, axisX - gap - left ), rowRect.bottom() - top );
            timestampRect = QRectF( axisX + gap, top, qMax( 0.0, right - axisX - gap ), titleMetrics.height() );
        }
        if ( m_timeline->layoutMode() == ExTimeline::Alternating || m_timeline->layoutMode() == ExTimeline::AlternatingReverse )
        {
            const qreal timestampWidth = qMin( timestampRect.width(), static_cast<qreal>( m_timeline->timestampWidth() ) );
            if ( contentOnRight )
            {
                timestampRect.setLeft( timestampRect.right() - timestampWidth );
            }
            else
            {
                timestampRect.setWidth( timestampWidth );
            }
        }

        QColor railColor = m_timeline->lineColor().isValid() ? m_timeline->lineColor() : option.palette.color( colorGroup, QPalette::Mid );
        if ( !m_timeline->lineColor().isValid() )
        {
            railColor.setAlpha( colorGroup == QPalette::Disabled ? 70 : 130 );
        }
        painter->setPen( QPen( railColor, m_timeline->lineWidth(), Qt::SolidLine, Qt::FlatCap ) );
        if ( index.row() > 0 )
        {
            painter->drawLine( QPointF( axisX, rowRect.top() ), QPointF( axisX, nodeCenterY ) );
        }
        if ( index.row() + 1 < m_model->rowCount() )
        {
            painter->drawLine( QPointF( axisX, nodeCenterY ), QPointF( axisX, rowRect.bottom() ) );
        }

        drawNode( painter,
                  event,
                  QPointF( axisX, nodeCenterY ),
                  m_timeline->resolvedEventColor( event, colorGroup ),
                  option.palette,
                  colorGroup );

        painter->setFont( titleFont );
        painter->setPen( option.palette.color( colorGroup, QPalette::Text ) );
        const QString titleText = QFontMetricsF( titleFont ).elidedText( event->title(), Qt::ElideRight, qRound( contentRect.width() ) );
        painter->drawText( QRectF( contentRect.left(), top, contentRect.width(), titleMetrics.height() ),
                           ( contentOnRight ? Qt::AlignLeft : Qt::AlignRight ) | Qt::AlignVCenter,
                           titleText );

        if ( m_timeline->isDescriptionVisible() && !event->description().isEmpty() )
        {
            QColor descriptionColor = option.palette.color( colorGroup, QPalette::Text );
            descriptionColor.setAlpha( colorGroup == QPalette::Disabled ? 100 : 170 );
            painter->setFont( descriptionFont );
            painter->setPen( descriptionColor );
            const QRectF descriptionRect( contentRect.left(),
                                          top + titleMetrics.height() + TextGap,
                                          contentRect.width(),
                                          qMax( 0.0, rowRect.bottom() - top - titleMetrics.height() - TextGap ) );
            painter->drawText( descriptionRect,
                               ( contentOnRight ? Qt::AlignLeft : Qt::AlignRight ) | Qt::AlignTop | Qt::TextWordWrap,
                               event->description() );
        }

        const QString timestamp = m_timeline->formattedTimestamp( event );
        if ( m_timeline->isTimestampVisible() && !timestamp.isEmpty() )
        {
            QColor timestampColor = option.palette.color( colorGroup, QPalette::Text );
            timestampColor.setAlpha( colorGroup == QPalette::Disabled ? 90 : 150 );
            painter->setFont( timestampFont );
            painter->setPen( timestampColor );
            painter->drawText( timestampRect,
                               ( contentOnRight ? Qt::AlignRight : Qt::AlignLeft ) | Qt::AlignVCenter,
                               QFontMetricsF( timestampFont ).elidedText( timestamp, Qt::ElideRight, qRound( timestampRect.width() ) ) );
        }

        painter->restore();
    }

private:
    void paintHorizontal( QPainter* painter,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index,
                          const ExTimelineEvent* event ) const
    {
        painter->save();
        painter->setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

        const QPalette::ColorGroup colorGroup =
            option.state.testFlag( QStyle::State_Enabled )
                ? ( option.state.testFlag( QStyle::State_Active ) ? QPalette::Active : QPalette::Inactive )
                : QPalette::Disabled;
        const QRectF itemRect       = option.rect;
        const QRectF highlightRect  = itemRect.adjusted( 2.0, 2.0, -2.0, -2.0 );
        if ( option.state.testFlag( QStyle::State_Selected ) || option.state.testFlag( QStyle::State_MouseOver ) )
        {
            QColor background = option.palette.color( colorGroup, QPalette::Highlight );
            background.setAlpha( option.state.testFlag( QStyle::State_Selected ) ? 34 : 18 );
            painter->setPen( Qt::NoPen );
            painter->setBrush( background );
            painter->drawRoundedRect( highlightRect, 6.0, 6.0 );
        }

        const bool contentBelow     = isContentOnRight( event, index.row() );
        const bool alternating      = m_timeline->layoutMode() == ExTimeline::Alternating
                                      || m_timeline->layoutMode() == ExTimeline::AlternatingReverse;
        const QFont titleFont       = resolvedTitleFont( option );
        const QFont descriptionFont = resolvedDescriptionFont( option );
        const QFont timestampFont   = resolvedTimestampFont( option );
        const QFontMetricsF titleMetrics( titleFont );
        const QFontMetricsF descriptionMetrics( descriptionFont );
        const QFontMetricsF timestampMetrics( timestampFont );
        const qreal radius = m_timeline->nodeSize() * 0.5;
        const qreal gap    = radius + AxisGap;
        const qreal verticalPadding = qMin( static_cast<qreal>( m_timeline->contentPadding() ),
                                            qMax( 0.0, ( itemRect.height() - m_timeline->nodeSize() ) * 0.5 ) );
        const qreal top    = itemRect.top() + verticalPadding;
        const qreal bottom = itemRect.bottom() - verticalPadding;
        qreal axisY        = itemRect.center().y();
        if ( !alternating )
        {
            const qreal timestampHeight = m_timeline->isTimestampVisible() ? timestampMetrics.height() : 0.0;
            axisY = contentBelow ? top + timestampHeight + AxisGap + radius : bottom - timestampHeight - AxisGap - radius;
        }
        axisY = bottom - top >= radius * 2.0 ? qBound( top + radius, axisY, bottom - radius ) : itemRect.center().y();
        const QPointF nodeCenter( itemRect.center().x(), axisY );

        QColor railColor = m_timeline->lineColor().isValid() ? m_timeline->lineColor()
                                                              : option.palette.color( colorGroup, QPalette::Mid );
        if ( !m_timeline->lineColor().isValid() )
        {
            railColor.setAlpha( colorGroup == QPalette::Disabled ? 70 : 130 );
        }
        painter->setPen( QPen( railColor, m_timeline->lineWidth(), Qt::SolidLine, Qt::FlatCap ) );
        if ( index.row() > 0 )
        {
            painter->drawLine( QPointF( itemRect.left(), axisY ), nodeCenter );
        }
        if ( index.row() + 1 < m_model->rowCount() )
        {
            painter->drawLine( nodeCenter, QPointF( itemRect.right(), axisY ) );
        }

        drawNode( painter,
                  event,
                  nodeCenter,
                  m_timeline->resolvedEventColor( event, colorGroup ),
                  option.palette,
                  colorGroup );

        const qreal textLeft  = itemRect.left() + m_timeline->contentPadding();
        const qreal textWidth = qMax( 0.0, itemRect.width() - m_timeline->contentPadding() * 2.0 );
        const QRectF contentRect( textLeft,
                                  contentBelow ? axisY + gap : top,
                                  textWidth,
                                  contentBelow ? qMax( 0.0, bottom - axisY - gap ) : qMax( 0.0, axisY - gap - top ) );

        qreal descriptionHeight = 0.0;
        if ( m_timeline->isDescriptionVisible() && !event->description().isEmpty() )
        {
            descriptionHeight = descriptionMetrics
                                    .boundingRect( QRectF( 0.0, 0.0, qMax( 40.0, textWidth ), 10000.0 ),
                                                   Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                                                   event->description() )
                                    .height();
        }
        const qreal contentHeight = titleMetrics.height() + ( descriptionHeight > 0.0 ? TextGap + descriptionHeight : 0.0 );
        const qreal contentTop = contentBelow ? contentRect.top() : qMax( contentRect.top(), contentRect.bottom() - contentHeight );

        painter->setFont( titleFont );
        painter->setPen( option.palette.color( colorGroup, QPalette::Text ) );
        painter->drawText( QRectF( contentRect.left(), contentTop, contentRect.width(), titleMetrics.height() ),
                           Qt::AlignHCenter | Qt::AlignVCenter,
                           QFontMetricsF( titleFont ).elidedText( event->title(), Qt::ElideRight, qRound( contentRect.width() ) ) );

        if ( descriptionHeight > 0.0 )
        {
            QColor descriptionColor = option.palette.color( colorGroup, QPalette::Text );
            descriptionColor.setAlpha( colorGroup == QPalette::Disabled ? 100 : 170 );
            painter->setFont( descriptionFont );
            painter->setPen( descriptionColor );
            painter->drawText( QRectF( contentRect.left(),
                                       contentTop + titleMetrics.height() + TextGap,
                                       contentRect.width(),
                                       qMax( 0.0,
                                             contentRect.bottom() - contentTop - titleMetrics.height() - TextGap ) ),
                               Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                               event->description() );
        }

        const QString timestamp = m_timeline->formattedTimestamp( event );
        if ( m_timeline->isTimestampVisible() && !timestamp.isEmpty() )
        {
            const qreal timestampWidth = qMin( qMax( 0.0, itemRect.width() - m_timeline->contentPadding() * 2.0 ),
                                                static_cast<qreal>( m_timeline->timestampWidth() ) );
            const qreal timestampTop = contentBelow ? axisY - gap - timestampMetrics.height() : axisY + gap;
            const QRectF timestampRect( nodeCenter.x() - timestampWidth * 0.5,
                                        timestampTop,
                                        timestampWidth,
                                        timestampMetrics.height() );
            QColor timestampColor = option.palette.color( colorGroup, QPalette::Text );
            timestampColor.setAlpha( colorGroup == QPalette::Disabled ? 90 : 150 );
            painter->setFont( timestampFont );
            painter->setPen( timestampColor );
            painter->drawText( timestampRect,
                               Qt::AlignCenter,
                               QFontMetricsF( timestampFont ).elidedText( timestamp,
                                                                         Qt::ElideRight,
                                                                         qRound( timestampRect.width() ) ) );
        }

        painter->restore();
    }

    qreal contentSideWidth( int totalWidth ) const
    {
        if ( !m_timeline )
        {
            return totalWidth;
        }
        const qreal available = qMax( 40, totalWidth - m_timeline->contentPadding() * 2 );
        if ( m_timeline->layoutMode() == ExTimeline::Alternating || m_timeline->layoutMode() == ExTimeline::AlternatingReverse )
        {
            return available * 0.5 - m_timeline->nodeSize() * 0.5 - AxisGap;
        }
        return available - ( m_timeline->isTimestampVisible() ? m_timeline->timestampWidth() : 0 ) - m_timeline->nodeSize() - AxisGap * 2.0;
    }

    bool isContentOnRight( const ExTimelineEvent* event, int row ) const
    {
        const bool alternating = m_timeline->layoutMode() == ExTimeline::Alternating
                                 || m_timeline->layoutMode() == ExTimeline::AlternatingReverse;
        if ( alternating && event->placement() == ExTimelineEvent::LeftSide )
        {
            return false;
        }
        if ( alternating && event->placement() == ExTimelineEvent::RightSide )
        {
            return true;
        }
        switch ( m_timeline->layoutMode() )
        {
            case ExTimeline::ContentOnLeft :
                return false;
            case ExTimeline::Alternating :
                return row % 2 == 0;
            case ExTimeline::AlternatingReverse :
                return row % 2 != 0;
            case ExTimeline::ContentOnRight :
            default :
                return true;
        }
    }

    qreal axisPosition( const QRectF& rect ) const
    {
        const qreal left  = rect.left() + m_timeline->contentPadding();
        const qreal right = rect.right() - m_timeline->contentPadding();
        if ( m_timeline->layoutMode() == ExTimeline::Alternating || m_timeline->layoutMode() == ExTimeline::AlternatingReverse )
        {
            return ( left + right ) * 0.5;
        }
        const qreal timestampSpace = m_timeline->isTimestampVisible() ? m_timeline->timestampWidth() : 0;
        return m_timeline->layoutMode() == ExTimeline::ContentOnRight ? left + timestampSpace + AxisGap + m_timeline->nodeSize() * 0.5
                                                                      : right - timestampSpace - AxisGap - m_timeline->nodeSize() * 0.5;
    }

    QFont resolvedTitleFont( const QStyleOptionViewItem& option ) const
    {
        QFont font = option.font;
        if ( m_timeline->titleFontPixelSize() > 0 )
        {
            font.setPixelSize( m_timeline->titleFontPixelSize() );
        }
        font.setWeight( QFont::DemiBold );
        return font;
    }

    QFont resolvedDescriptionFont( const QStyleOptionViewItem& option ) const
    {
        QFont font = option.font;
        font.setWeight( QFont::Normal );
        if ( m_timeline->descriptionFontPixelSize() > 0 )
        {
            font.setPixelSize( m_timeline->descriptionFontPixelSize() );
        }
        else if ( font.pixelSize() > 0 )
        {
            font.setPixelSize( qMax( 9, font.pixelSize() - 1 ) );
        }
        return font;
    }

    QFont resolvedTimestampFont( const QStyleOptionViewItem& option ) const
    {
        QFont font = resolvedDescriptionFont( option );
        if ( m_timeline->timestampFontPixelSize() > 0 )
        {
            font.setPixelSize( m_timeline->timestampFontPixelSize() );
        }
        return font;
    }

    void drawNode( QPainter* painter,
                   const ExTimelineEvent* event,
                   const QPointF& center,
                   const QColor& color,
                   const QPalette& palette,
                   QPalette::ColorGroup colorGroup ) const
    {
        const qreal radius = m_timeline->nodeSize() * 0.5;
        if ( event->status() == ExTimelineEvent::Current && m_timeline->isAnimationEnabled() )
        {
            const qreal progress = m_timeline->pulseProgress();
            painter->setPen( QPen( withAlpha( color, qRound( 105.0 * ( 1.0 - progress ) ) ), 1.5 ) );
            painter->setBrush( Qt::NoBrush );
            painter->drawEllipse( center, radius + 2.0 + progress * 6.0, radius + 2.0 + progress * 6.0 );
        }

        const bool outlined = event->status() == ExTimelineEvent::Pending;
        painter->setPen( QPen( color, outlined ? 2.0 : 1.0 ) );
        painter->setBrush( outlined ? palette.color( colorGroup, QPalette::Base ) : color );
        painter->drawEllipse( center, radius, radius );

        QColor symbolColor = outlined ? color : QColor( Qt::white );
        if ( !event->icon().isEmpty() )
        {
            QFont iconFont( QStringLiteral( "Segoe Fluent Icons" ) );
            iconFont.setPixelSize( qMax( 7, qRound( m_timeline->nodeSize() * 0.62 ) ) );
            painter->setFont( iconFont );
            painter->setPen( symbolColor );
            painter->drawText(
                QRectF( center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0 ), Qt::AlignCenter, event->icon() );
            return;
        }

        painter->setPen( QPen( symbolColor, qMax( 1.2, m_timeline->nodeSize() * 0.11 ), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        if ( event->status() == ExTimelineEvent::Completed )
        {
            QPainterPath check;
            check.moveTo( center + QPointF( -radius * 0.42, 0.0 ) );
            check.lineTo( center + QPointF( -radius * 0.10, radius * 0.30 ) );
            check.lineTo( center + QPointF( radius * 0.46, -radius * 0.34 ) );
            painter->drawPath( check );
        }
        else if ( event->status() == ExTimelineEvent::Error )
        {
            painter->drawLine( center + QPointF( -radius * 0.30, -radius * 0.30 ), center + QPointF( radius * 0.30, radius * 0.30 ) );
            painter->drawLine( center + QPointF( radius * 0.30, -radius * 0.30 ), center + QPointF( -radius * 0.30, radius * 0.30 ) );
        }
        else if ( event->status() == ExTimelineEvent::Warning )
        {
            painter->drawLine( center + QPointF( 0.0, -radius * 0.36 ), center + QPointF( 0.0, radius * 0.10 ) );
            painter->drawPoint( center + QPointF( 0.0, radius * 0.36 ) );
        }
    }

    ExTimeline* m_timeline   = nullptr;
    ExTimelineModel* m_model = nullptr;
};

ExTimelineEvent::ExTimelineEvent( QObject* parent )
    : QObject( parent )
{
}

ExTimelineEvent::ExTimelineEvent( const QDateTime& timestamp,
                                  const QString& title,
                                  const QString& description,
                                  Status status,
                                  QObject* parent )
    : QObject( parent )
    , m_timestamp( timestamp )
    , m_title( title )
    , m_description( description )
    , m_status( status )
{
}

#define EX_TIMELINE_EVENT_SETTER( Type, Property, Setter ) \
    void ExTimelineEvent::Setter( Type value )             \
    {                                                      \
        if ( m_##Property == value )                       \
        {                                                  \
            return;                                        \
        }                                                  \
        m_##Property = std::move( value );                 \
        emit Property##Changed( m_##Property );            \
        emit itemChanged();                                \
    }

EX_TIMELINE_EVENT_SETTER( QDateTime, timestamp, setTimestamp )
EX_TIMELINE_EVENT_SETTER( QString, timeText, setTimeText )
EX_TIMELINE_EVENT_SETTER( QString, title, setTitle )
EX_TIMELINE_EVENT_SETTER( QString, description, setDescription )
EX_TIMELINE_EVENT_SETTER( QColor, color, setColor )
EX_TIMELINE_EVENT_SETTER( QString, icon, setIcon )

#undef EX_TIMELINE_EVENT_SETTER

void ExTimelineEvent::setStatus( Status status )
{
    if ( status < Normal || status > Error || m_status == status )
    {
        return;
    }
    m_status = status;
    emit statusChanged( status );
    emit itemChanged();
}

void ExTimelineEvent::setPlacement( Placement placement )
{
    if ( placement < Automatic || placement > RightSide || m_placement == placement )
    {
        return;
    }
    m_placement = placement;
    emit placementChanged( placement );
    emit itemChanged();
}

ExTimeline::ExTimeline( QWidget* parent )
    : QListView( parent )
{
    m_timelineModel    = new ExTimelineModel( this );
    m_timelineDelegate = new ExTimelineDelegate( this, m_timelineModel );
    QListView::setModel( m_timelineModel );
    QListView::setItemDelegate( m_timelineDelegate );

    setFrameShape( QFrame::NoFrame );
    setAutoFillBackground( false );
    viewport()->setAutoFillBackground( false );
    setAttribute( Qt::WA_StyledBackground, false );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );
    setResizeMode( QListView::Adjust );
    setSelectionMode( QAbstractItemView::NoSelection );
    setMouseTracking( true );
    setSpacing( 0 );
    setUniformItemSizes( false );
    applyViewOrientation();

    m_pulseAnimation = new QVariantAnimation( this );
    m_pulseAnimation->setStartValue( 0.0 );
    m_pulseAnimation->setEndValue( 1.0 );
    m_pulseAnimation->setLoopCount( -1 );
    m_pulseAnimation->setEasingCurve( QEasingCurve::InOutSine );
    m_pulseAnimation->setDuration( m_animationDuration );
    connect( m_pulseAnimation,
             &QVariantAnimation::valueChanged,
             this,
             [ this ]( const QVariant& value )
             {
                 m_pulseProgress = value.toReal();
                 viewport()->update();
             } );

    connect( this,
             &QListView::clicked,
             this,
             [ this ]( const QModelIndex& index )
             {
                 if ( ExTimelineEvent* event = eventAt( index.row() ) )
                 {
                     emit eventClicked( event );
                 }
             } );
    connect( this,
             &QListView::activated,
             this,
             [ this ]( const QModelIndex& index )
             {
                 if ( ExTimelineEvent* event = eventAt( index.row() ) )
                 {
                     emit eventActivated( event );
                 }
             } );
}

ExTimeline::~ExTimeline() = default;

#define EX_TIMELINE_LAYOUT_SETTER( Type, Property, Setter ) \
    void ExTimeline::Setter( Type value )                   \
    {                                                       \
        if ( m_##Property == value )                        \
        {                                                   \
            return;                                         \
        }                                                   \
        m_##Property = std::move( value );                  \
        refreshItemLayout();                                \
        emit Property##Changed( m_##Property );             \
    }

EX_TIMELINE_LAYOUT_SETTER( bool, timestampVisible, setTimestampVisible )
EX_TIMELINE_LAYOUT_SETTER( bool, descriptionVisible, setDescriptionVisible )
EX_TIMELINE_LAYOUT_SETTER( QString, timestampFormat, setTimestampFormat )
EX_TIMELINE_LAYOUT_SETTER( QColor, lineColor, setLineColor )

#undef EX_TIMELINE_LAYOUT_SETTER

void ExTimeline::setOrientation( Qt::Orientation orientation )
{
    if ( ( orientation != Qt::Horizontal && orientation != Qt::Vertical ) || m_orientation == orientation )
    {
        return;
    }
    m_orientation = orientation;
    applyViewOrientation();
    refreshItemLayout();
    emit orientationChanged( orientation );
}

void ExTimeline::setLayoutMode( LayoutMode mode )
{
    if ( mode < ContentOnRight || mode > AlternatingReverse || m_layoutMode == mode )
    {
        return;
    }
    m_layoutMode = mode;
    refreshItemLayout();
    emit layoutModeChanged( mode );
}

void ExTimeline::setReverse( bool reverse )
{
    if ( m_reverse == reverse )
    {
        return;
    }
    m_reverse = reverse;
    m_timelineModel->setReverse( reverse );
    refreshItemLayout();
    emit reverseChanged( reverse );
}

#define EX_TIMELINE_INT_SETTER( Property, Setter, Minimum, Maximum ) \
    void ExTimeline::Setter( int value )                             \
    {                                                                \
        value = qBound( Minimum, value, Maximum );                   \
        if ( m_##Property == value )                                 \
        {                                                            \
            return;                                                  \
        }                                                            \
        m_##Property = value;                                        \
        refreshItemLayout();                                         \
        emit Property##Changed( value );                             \
    }

EX_TIMELINE_INT_SETTER( timestampWidth, setTimestampWidth, 32, 400 )
EX_TIMELINE_INT_SETTER( nodeSize, setNodeSize, 6, 64 )
EX_TIMELINE_INT_SETTER( itemSpacing, setItemSpacing, 0, 160 )
EX_TIMELINE_INT_SETTER( horizontalItemWidth, setHorizontalItemWidth, 120, 640 )
EX_TIMELINE_INT_SETTER( contentPadding, setContentPadding, 0, 160 )
EX_TIMELINE_INT_SETTER( titleFontPixelSize, setTitleFontPixelSize, 0, 96 )
EX_TIMELINE_INT_SETTER( descriptionFontPixelSize, setDescriptionFontPixelSize, 0, 96 )
EX_TIMELINE_INT_SETTER( timestampFontPixelSize, setTimestampFontPixelSize, 0, 96 )

#undef EX_TIMELINE_INT_SETTER

void ExTimeline::setLineWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }
    width = qBound( 0.5, width, 24.0 );
    if ( qFuzzyCompare( m_lineWidth + 1.0, width + 1.0 ) )
    {
        return;
    }
    m_lineWidth = width;
    viewport()->update();
    emit lineWidthChanged( width );
}

void ExTimeline::setAnimationEnabled( bool enabled )
{
    if ( m_animationEnabled == enabled )
    {
        return;
    }
    m_animationEnabled = enabled;
    updateAnimationState();
    emit animationEnabledChanged( enabled );
}

void ExTimeline::setAnimationDuration( int duration )
{
    duration = qBound( 200, duration, 10000 );
    if ( m_animationDuration == duration )
    {
        return;
    }
    m_animationDuration = duration;
    m_pulseAnimation->setDuration( duration );
    emit animationDurationChanged( duration );
}

QList<ExTimelineEvent*> ExTimeline::events() const
{
    return m_timelineModel->events();
}

ExTimelineEvent* ExTimeline::eventAt( int visualIndex ) const
{
    return m_timelineModel->eventAt( visualIndex );
}

ExTimelineEvent* ExTimeline::addEvent( const QDateTime& timestamp,
                                       const QString& title,
                                       const QString& description,
                                       ExTimelineEvent::Status status )
{
    auto* event = new ExTimelineEvent( timestamp, title, description, status, this );
    addEvent( event );
    return event;
}

void ExTimeline::addEvent( ExTimelineEvent* event )
{
    if ( !event || m_timelineModel->contains( event ) )
    {
        return;
    }
    if ( auto* previousTimeline = qobject_cast<ExTimeline*>( event->parent() ) )
    {
        if ( previousTimeline != this )
        {
            previousTimeline->takeEvent( event );
        }
    }
    event->setParent( this );
    m_timelineModel->appendEvent( event );
    connectEvent( event );
    refreshItemLayout();
    updateAnimationState();
    emit eventsChanged();
}

ExTimelineEvent* ExTimeline::takeEvent( ExTimelineEvent* event )
{
    if ( !event || !m_timelineModel->takeEvent( event ) )
    {
        return nullptr;
    }
    disconnect( event, nullptr, this, nullptr );
    event->setParent( nullptr );
    refreshItemLayout();
    updateAnimationState();
    emit eventsChanged();
    return event;
}

void ExTimeline::removeEvent( ExTimelineEvent* event )
{
    if ( ExTimelineEvent* taken = takeEvent( event ) )
    {
        taken->deleteLater();
    }
}

void ExTimeline::clearEvents()
{
    const QList<ExTimelineEvent*> oldEvents = m_timelineModel->takeAllEvents();
    if ( oldEvents.isEmpty() )
    {
        return;
    }
    for ( ExTimelineEvent* event : oldEvents )
    {
        disconnect( event, nullptr, this, nullptr );
        event->deleteLater();
    }
    refreshItemLayout();
    updateAnimationState();
    emit eventsChanged();
}

QSize ExTimeline::sizeHint() const
{
    return m_orientation == Qt::Horizontal ? QSize( 720, 280 ) : QSize( 640, 420 );
}

QSize ExTimeline::minimumSizeHint() const
{
    return m_orientation == Qt::Horizontal ? QSize( 320, 180 ) : QSize( 280, 160 );
}

void ExTimeline::showEvent( QShowEvent* event )
{
    QListView::showEvent( event );
    updateAnimationState();
}

void ExTimeline::hideEvent( QHideEvent* event )
{
    QListView::hideEvent( event );
    updateAnimationState();
}

void ExTimeline::changeEvent( QEvent* event )
{
    QListView::changeEvent( event );
    if ( event
         && ( event->type() == QEvent::PaletteChange || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange ) )
    {
        refreshItemLayout();
    }
    if ( event && event->type() == QEvent::EnabledChange )
    {
        updateAnimationState();
    }
}

QString ExTimeline::formattedTimestamp( const ExTimelineEvent* event ) const
{
    if ( !event )
    {
        return {};
    }
    if ( !event->timeText().isEmpty() )
    {
        return event->timeText();
    }
    return event->timestamp().isValid() ? event->timestamp().toString( m_timestampFormat ) : QString();
}

QColor ExTimeline::resolvedEventColor( const ExTimelineEvent* event, QPalette::ColorGroup group ) const
{
    if ( event && event->color().isValid() && group != QPalette::Disabled )
    {
        return event->color();
    }
    if ( group == QPalette::Disabled )
    {
        return palette().color( QPalette::Disabled, accentRole() );
    }
    if ( !event )
    {
        return palette().color( group, accentRole() );
    }
    switch ( event->status() )
    {
        case ExTimelineEvent::Completed :
            return QColor( QStringLiteral( "#107C10" ) );
        case ExTimelineEvent::Pending :
            return palette().color( group, QPalette::Mid );
        case ExTimelineEvent::Warning :
            return QColor( QStringLiteral( "#F2A900" ) );
        case ExTimelineEvent::Error :
            return QColor( QStringLiteral( "#D13438" ) );
        case ExTimelineEvent::Current :
        case ExTimelineEvent::Normal :
        default :
            return palette().color( group, accentRole() );
    }
}

qreal ExTimeline::pulseProgress() const
{
    return m_pulseProgress;
}

void ExTimeline::applyViewOrientation()
{
    const bool horizontal = m_orientation == Qt::Horizontal;
    setFlow( horizontal ? QListView::LeftToRight : QListView::TopToBottom );
    setWrapping( false );
    setHorizontalScrollBarPolicy( horizontal ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( horizontal ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded );
}

void ExTimeline::connectEvent( ExTimelineEvent* event )
{
    connect( event,
             &ExTimelineEvent::itemChanged,
             this,
             [ this, event ]
             {
                 m_timelineModel->notifyEventChanged( event );
                 refreshItemLayout();
                 updateAnimationState();
                 emit eventsChanged();
             } );
    connect( event,
             &QObject::destroyed,
             this,
             [ this, event ]
             {
                 if ( m_timelineModel->takeEvent( event ) )
                 {
                     refreshItemLayout();
                     updateAnimationState();
                     emit eventsChanged();
                 }
             } );
}

void ExTimeline::refreshItemLayout()
{
    scheduleDelayedItemsLayout();
    updateGeometry();
    viewport()->update();
}

void ExTimeline::updateAnimationState()
{
    bool hasCurrentEvent = false;
    for ( const ExTimelineEvent* event : m_timelineModel->events() )
    {
        if ( event && event->status() == ExTimelineEvent::Current )
        {
            hasCurrentEvent = true;
            break;
        }
    }
    const bool shouldRun = m_animationEnabled && isVisible() && isEnabled() && hasCurrentEvent;
    if ( shouldRun )
    {
        if ( m_pulseAnimation->state() != QAbstractAnimation::Running )
        {
            m_pulseAnimation->start();
        }
    }
    else
    {
        m_pulseAnimation->stop();
        m_pulseProgress = 0.0;
        viewport()->update();
    }
}
