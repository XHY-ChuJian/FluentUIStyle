#include "exmultiradialgauge.h"

#include <QEasingCurve>
#include <QFontMetricsF>
#include <QPainter>
#include <QPalette>
#include <QPolygonF>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOption>
#include <QVariantAnimation>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
constexpr qreal FullCircle = 360.0;
constexpr int MaximumTickCount = 720;
constexpr int MaximumMajorTickCount = 180;

QPointF pointAtAngle( const QPointF& center, qreal radius, qreal angle )
{
    const qreal radians = qDegreesToRadians( angle );
    return QPointF( center.x() + radius * std::sin( radians ),
                    center.y() - radius * std::cos( radians ) );
}

QPalette::ColorRole accentRole()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return QPalette::Accent;
#else
    return QPalette::Highlight;
#endif
}

QColor disabledItemColor( const QColor& color, const QPalette& palette )
{
    if ( !color.isValid() )
    {
        return palette.color( QPalette::Disabled, accentRole() );
    }

    const QColor disabledText = palette.color( QPalette::Disabled, QPalette::Text );
    return QColor( ( color.red() + disabledText.red() ) / 2,
                   ( color.green() + disabledText.green() ) / 2,
                   ( color.blue() + disabledText.blue() ) / 2,
                   color.alpha() );
}
}

ExMultiRadialGaugeItem::ExMultiRadialGaugeItem( QObject* parent )
    : QObject( parent )
{
}

ExMultiRadialGaugeItem::ExMultiRadialGaugeItem( const QString& label,
                                                qreal value,
                                                const QColor& color,
                                                QObject* parent )
    : QObject( parent )
    , m_label( label )
    , m_value( qIsFinite( value ) ? value : 0.0 )
    , m_color( color )
{
}

#define EX_MULTI_RADIAL_GAUGE_ITEM_SETTER( Type, Property, Setter ) \
    void ExMultiRadialGaugeItem::Setter( Type value ) \
    { \
        if ( m_##Property == value ) \
        { \
            return; \
        } \
        m_##Property = std::move( value ); \
        emit Property##Changed( m_##Property ); \
        emit itemChanged(); \
    }

EX_MULTI_RADIAL_GAUGE_ITEM_SETTER( QString, label, setLabel )
EX_MULTI_RADIAL_GAUGE_ITEM_SETTER( QColor, color, setColor )
EX_MULTI_RADIAL_GAUGE_ITEM_SETTER( bool, visible, setVisible )

#undef EX_MULTI_RADIAL_GAUGE_ITEM_SETTER

void ExMultiRadialGaugeItem::setValue( qreal value )
{
    if ( !qIsFinite( value ) || qFuzzyCompare( m_value + 1.0, value + 1.0 ) )
    {
        return;
    }

    m_value = value;
    emit valueChanged( value );
    emit itemChanged();
}

void ExMultiRadialGaugeItem::setTitleOffset( QPointF offset )
{
    if ( !qIsFinite( offset.x() ) || !qIsFinite( offset.y() ) || m_titleOffset == offset )
    {
        return;
    }

    m_titleOffset = offset;
    emit titleOffsetChanged( offset );
    emit itemChanged();
}

void ExMultiRadialGaugeItem::setDetailOffset( QPointF offset )
{
    if ( !qIsFinite( offset.x() ) || !qIsFinite( offset.y() ) || m_detailOffset == offset )
    {
        return;
    }

    m_detailOffset = offset;
    emit detailOffsetChanged( offset );
    emit itemChanged();
}

ExMultiRadialGauge::ExMultiRadialGauge( QWidget* parent )
    : QWidget( parent )
{
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    setAutoFillBackground( false );

    m_valueAnimation = new QVariantAnimation( this );
    m_valueAnimation->setStartValue( 0.0 );
    m_valueAnimation->setEndValue( 1.0 );
    m_valueAnimation->setEasingCurve( QEasingCurve::OutCubic );
    connect( m_valueAnimation, &QVariantAnimation::valueChanged, this, [this]( const QVariant& value )
             {
                 const qreal progress = value.toReal();
                 for ( const ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
                 {
                     const qreal startValue = m_animationStartValues.value( item, item->value() );
                     m_displayedValues.insert( item,
                                               startValue + ( item->value() - startValue ) * progress );
                 }
                 update();
             } );
    connect( m_valueAnimation, &QVariantAnimation::finished, this, [this]
             {
                 synchronizeDisplayedValues();
             } );
}

ExMultiRadialGauge::~ExMultiRadialGauge()
{
    m_valueAnimation->stop();
    for ( ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
    {
        disconnect( item, nullptr, this, nullptr );
    }
    m_items.clear();
    m_displayedValues.clear();
    m_animationStartValues.clear();
}

#define EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( Type, Property, Setter ) \
    void ExMultiRadialGauge::Setter( Type value ) \
    { \
        if ( m_##Property == value ) \
        { \
            return; \
        } \
        m_##Property = std::move( value ); \
        update(); \
        emit Property##Changed( m_##Property ); \
    }

EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, trackVisible, setTrackVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, progressVisible, setProgressVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, progressOverlap, setProgressOverlap )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, labelsVisible, setLabelsVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, hubVisible, setHubVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, titleVisible, setTitleVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, detailVisible, setDetailVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( bool, detailBadgeVisible, setDetailBadgeVisible )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, trackColor, setTrackColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, tickColor, setTickColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, labelColor, setLabelColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, hubColor, setHubColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, titleColor, setTitleColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QColor, detailTextColor, setDetailTextColor )
EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER( QString, valueSuffix, setValueSuffix )

#undef EX_MULTI_RADIAL_GAUGE_SIMPLE_SETTER

#define EX_MULTI_RADIAL_GAUGE_REAL_SETTER( Property, Setter, MinimumValue, MaximumValue ) \
    void ExMultiRadialGauge::Setter( qreal value ) \
    { \
        if ( !qIsFinite( value ) ) \
        { \
            return; \
        } \
        value = qBound<qreal>( MinimumValue, value, MaximumValue ); \
        if ( qFuzzyCompare( m_##Property + 1.0, value + 1.0 ) ) \
        { \
            return; \
        } \
        m_##Property = value; \
        updateGeometry(); \
        update(); \
        emit Property##Changed( value ); \
    }

EX_MULTI_RADIAL_GAUGE_REAL_SETTER( trackWidth, setTrackWidth, 0.5, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( progressWidth, setProgressWidth, 0.5, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( progressSpacing, setProgressSpacing, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( scalePadding, setScalePadding, 0.0, 200.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( tickLength, setTickLength, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( tickWidth, setTickWidth, 0.1, 50.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( majorTickLength, setMajorTickLength, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( majorTickWidth, setMajorTickWidth, 0.1, 50.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( tickPadding, setTickPadding, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( labelPadding, setLabelPadding, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( needleWidth, setNeedleWidth, 0.5, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( needleLength, setNeedleLength, 0.05, 1.2 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( hubRadius, setHubRadius, 0.0, 100.0 )
EX_MULTI_RADIAL_GAUGE_REAL_SETTER( detailBadgePadding, setDetailBadgePadding, 0.0, 100.0 )

#undef EX_MULTI_RADIAL_GAUGE_REAL_SETTER

void ExMultiRadialGauge::setMinimum( qreal minimum )
{
    if ( !qIsFinite( minimum ) )
    {
        return;
    }
    setRange( minimum, qMax( minimum, m_maximum ) );
}

void ExMultiRadialGauge::setMaximum( qreal maximum )
{
    if ( !qIsFinite( maximum ) )
    {
        return;
    }
    setRange( qMin( m_minimum, maximum ), maximum );
}

void ExMultiRadialGauge::setRange( qreal minimum, qreal maximum )
{
    if ( !qIsFinite( minimum ) || !qIsFinite( maximum ) )
    {
        return;
    }
    maximum = qMax( minimum, maximum );
    const bool minimumChanged = !qFuzzyCompare( m_minimum + 1.0, minimum + 1.0 );
    const bool maximumChanged = !qFuzzyCompare( m_maximum + 1.0, maximum + 1.0 );
    if ( !minimumChanged && !maximumChanged )
    {
        return;
    }
    m_minimum = minimum;
    m_maximum = maximum;
    update();
    if ( minimumChanged )
    {
        emit this->minimumChanged( minimum );
    }
    if ( maximumChanged )
    {
        emit this->maximumChanged( maximum );
    }
}

void ExMultiRadialGauge::setMinimumAngle( qreal angle )
{
    if ( !qIsFinite( angle ) || qFuzzyCompare( m_minimumAngle + 1.0, angle + 1.0 ) )
    {
        return;
    }
    m_minimumAngle = angle;
    update();
    emit minimumAngleChanged( angle );
}

void ExMultiRadialGauge::setMaximumAngle( qreal angle )
{
    if ( !qIsFinite( angle ) || qFuzzyCompare( m_maximumAngle + 1.0, angle + 1.0 ) )
    {
        return;
    }
    m_maximumAngle = angle;
    update();
    emit maximumAngleChanged( angle );
}

void ExMultiRadialGauge::setMajorTickCount( int count )
{
    count = qBound( 2, count, MaximumMajorTickCount );
    if ( m_majorTickCount == count )
    {
        return;
    }
    m_majorTickCount = count;
    update();
    emit majorTickCountChanged( count );
}

void ExMultiRadialGauge::setMinorTickCount( int count )
{
    count = qBound( 0, count, MaximumTickCount );
    if ( m_minorTickCount == count )
    {
        return;
    }
    m_minorTickCount = count;
    update();
    emit minorTickCountChanged( count );
}

void ExMultiRadialGauge::setTrackCapStyle( Qt::PenCapStyle style )
{
    if ( style != Qt::FlatCap && style != Qt::SquareCap && style != Qt::RoundCap )
    {
        return;
    }
    if ( m_trackCapStyle == style )
    {
        return;
    }
    m_trackCapStyle = style;
    update();
    emit trackCapStyleChanged( style );
}

void ExMultiRadialGauge::setProgressCapStyle( Qt::PenCapStyle style )
{
    if ( style != Qt::FlatCap && style != Qt::SquareCap && style != Qt::RoundCap )
    {
        return;
    }
    if ( m_progressCapStyle == style )
    {
        return;
    }
    m_progressCapStyle = style;
    update();
    emit progressCapStyleChanged( style );
}

void ExMultiRadialGauge::setLabelFontPixelSize( int size )
{
    size = qBound( 1, size, 200 );
    if ( m_labelFontPixelSize == size )
    {
        return;
    }
    m_labelFontPixelSize = size;
    update();
    emit labelFontPixelSizeChanged( size );
}

void ExMultiRadialGauge::setNeedleStyle( NeedleStyle style )
{
    if ( style < NoNeedle || style > TriangleNeedle || m_needleStyle == style )
    {
        return;
    }
    m_needleStyle = style;
    update();
    emit needleStyleChanged( style );
}

void ExMultiRadialGauge::setNeedleOffset( QPointF offset )
{
    if ( !qIsFinite( offset.x() ) || !qIsFinite( offset.y() ) || m_needleOffset == offset )
    {
        return;
    }
    m_needleOffset = offset;
    update();
    emit needleOffsetChanged( offset );
}

void ExMultiRadialGauge::setTitleFontPixelSize( int size )
{
    size = qBound( 1, size, 200 );
    if ( m_titleFontPixelSize == size )
    {
        return;
    }
    m_titleFontPixelSize = size;
    update();
    emit titleFontPixelSizeChanged( size );
}

void ExMultiRadialGauge::setDetailFontPixelSize( int size )
{
    size = qBound( 1, size, 200 );
    if ( m_detailFontPixelSize == size )
    {
        return;
    }
    m_detailFontPixelSize = size;
    update();
    emit detailFontPixelSizeChanged( size );
}

void ExMultiRadialGauge::setValueDecimals( int decimals )
{
    decimals = qBound( 0, decimals, 6 );
    if ( m_valueDecimals == decimals )
    {
        return;
    }
    m_valueDecimals = decimals;
    update();
    emit valueDecimalsChanged( decimals );
}

void ExMultiRadialGauge::setValueAnimationDuration( int duration )
{
    duration = qBound( 0, duration, 5000 );
    if ( m_valueAnimationDuration == duration )
    {
        return;
    }
    m_valueAnimationDuration = duration;
    if ( duration == 0 )
    {
        m_valueAnimation->stop();
        synchronizeDisplayedValues();
    }
    emit valueAnimationDurationChanged( duration );
}

QList<ExMultiRadialGaugeItem*> ExMultiRadialGauge::items() const
{
    return m_items;
}

ExMultiRadialGaugeItem* ExMultiRadialGauge::addItem( const QString& label,
                                                      qreal value,
                                                      const QColor& color )
{
    auto* item = new ExMultiRadialGaugeItem( label, value, color, this );
    addItem( item );
    return item;
}

void ExMultiRadialGauge::addItem( ExMultiRadialGaugeItem* item )
{
    if ( !item || m_items.contains( item ) )
    {
        return;
    }
    item->setParent( this );
    m_items.append( item );
    m_displayedValues.insert( item, item->value() );
    connectItem( item );
    update();
    emit itemsChanged();
}

void ExMultiRadialGauge::removeItem( ExMultiRadialGaugeItem* item )
{
    if ( !item || !m_items.removeOne( item ) )
    {
        return;
    }
    disconnect( item, nullptr, this, nullptr );
    m_displayedValues.remove( item );
    m_animationStartValues.remove( item );
    item->deleteLater();
    update();
    emit itemsChanged();
}

void ExMultiRadialGauge::clearItems()
{
    if ( m_items.isEmpty() )
    {
        return;
    }
    m_valueAnimation->stop();
    const auto oldItems = m_items;
    m_items.clear();
    m_displayedValues.clear();
    m_animationStartValues.clear();
    for ( ExMultiRadialGaugeItem* item : oldItems )
    {
        disconnect( item, nullptr, this, nullptr );
        item->deleteLater();
    }
    update();
    emit itemsChanged();
}

QSize ExMultiRadialGauge::sizeHint() const
{
    return QSize( 320, 260 );
}

QSize ExMultiRadialGauge::minimumSizeHint() const
{
    return QSize( 120, 100 );
}

void ExMultiRadialGauge::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QStyleOption option;
    option.initFrom( this );
    const bool enabled = option.state.testFlag( QStyle::State_Enabled );
    const QPalette::ColorGroup colorGroup = enabled
                                                  ? ( option.state.testFlag( QStyle::State_Active )
                                                          ? QPalette::Active
                                                          : QPalette::Inactive )
                                                  : QPalette::Disabled;
    const QColor accentColor = option.palette.color( colorGroup, accentRole() );
    const QColor resolvedTrackColor = enabled && m_trackColor.isValid()
                                          ? m_trackColor
                                          : option.palette.color( colorGroup, QPalette::Mid );
    QColor resolvedTickColor = enabled && m_tickColor.isValid()
                                   ? m_tickColor
                                   : option.palette.color( colorGroup, QPalette::Text );
    QColor resolvedLabelColor = enabled && m_labelColor.isValid()
                                    ? m_labelColor
                                    : option.palette.color( colorGroup, QPalette::Text );
    const QColor resolvedTitleColor = enabled && m_titleColor.isValid()
                                          ? m_titleColor
                                          : option.palette.color( colorGroup, QPalette::Text );
    if ( !m_tickColor.isValid() )
    {
        resolvedTickColor.setAlpha( enabled ? 150 : 80 );
    }
    if ( !m_labelColor.isValid() )
    {
        resolvedLabelColor.setAlpha( enabled ? 180 : 90 );
    }

    const qreal side = qMin( width(), height() );
    const QPointF center = QRectF( rect() ).center();
    const qreal maximumLineWidth = qMax( m_trackWidth, m_progressWidth );
    const qreal radius = qMax( 1.0, side * 0.5 - m_scalePadding - maximumLineWidth * 0.5 - 1.0 );
    const QRectF scaleRect( center.x() - radius,
                            center.y() - radius,
                            radius * 2.0,
                            radius * 2.0 );
    const qreal sweep = sweepAngle();
    const qreal painterStartAngle = 90.0 - m_minimumAngle;

    QPainter painter( this );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

    if ( m_trackVisible )
    {
        painter.setPen( QPen( resolvedTrackColor,
                              m_trackWidth,
                              Qt::SolidLine,
                              m_trackCapStyle ) );
        painter.setBrush( Qt::NoBrush );
        painter.drawArc( scaleRect,
                         qRound( painterStartAngle * 16.0 ),
                         qRound( -sweep * 16.0 ) );
    }

    const auto itemColor = [&]( const ExMultiRadialGaugeItem* item )
    {
        if ( !item )
        {
            return accentColor;
        }
        if ( !enabled )
        {
            return disabledItemColor( item->color(), option.palette );
        }
        return item->color().isValid() ? item->color() : accentColor;
    };

    if ( m_progressVisible )
    {
        const auto drawProgress = [&]( const ExMultiRadialGaugeItem* item, qreal itemRadius )
        {
            const qreal fraction = valueFraction( displayedValue( item ) );
            if ( fraction <= 0.0 || itemRadius <= m_progressWidth * 0.5 )
            {
                return;
            }
            const QRectF itemRect( center.x() - itemRadius,
                                   center.y() - itemRadius,
                                   itemRadius * 2.0,
                                   itemRadius * 2.0 );
            painter.setPen( QPen( itemColor( item ),
                                  m_progressWidth,
                                  Qt::SolidLine,
                                  m_progressCapStyle ) );
            painter.drawArc( itemRect,
                             qRound( painterStartAngle * 16.0 ),
                             qRound( -sweep * fraction * 16.0 ) );
        };

        if ( m_progressOverlap )
        {
            QList<const ExMultiRadialGaugeItem*> sortedItems;
            sortedItems.reserve( m_items.size() );
            for ( const ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
            {
                if ( item && item->isVisible() )
                {
                    sortedItems.append( item );
                }
            }
            std::stable_sort( sortedItems.begin(),
                              sortedItems.end(),
                              [this]( const ExMultiRadialGaugeItem* left,
                                      const ExMultiRadialGaugeItem* right )
                              {
                                  return valueFraction( displayedValue( left ) )
                                         > valueFraction( displayedValue( right ) );
                              } );
            for ( const ExMultiRadialGaugeItem* item : std::as_const( sortedItems ) )
            {
                drawProgress( item, radius );
            }
        }
        else
        {
            for ( qsizetype index = 0; index < m_items.size(); ++index )
            {
                const ExMultiRadialGaugeItem* item = m_items.at( index );
                const qreal itemRadius = radius - index * ( m_progressWidth + m_progressSpacing );
                if ( !item || !item->isVisible() || itemRadius <= m_progressWidth * 0.5 )
                {
                    continue;
                }
                drawProgress( item, itemRadius );
            }
        }
    }

    const int majorTickCount = qBound( 2, m_majorTickCount, MaximumMajorTickCount );
    const int majorIntervalCount = majorTickCount - 1;
    const int maximumMinorTickCount = qMax( 0, MaximumTickCount / majorIntervalCount - 1 );
    const int minorTickCount = qMin( m_minorTickCount, maximumMinorTickCount );
    const qreal tickOuterRadius = qMax( 0.0, radius - maximumLineWidth * 0.5 - m_tickPadding );

    const auto drawTick = [&]( qreal fraction, qreal length, qreal width )
    {
        const qreal angle = m_minimumAngle + sweep * fraction;
        const qreal innerRadius = qMax( 0.0, tickOuterRadius - length );
        painter.setPen( QPen( resolvedTickColor, width, Qt::SolidLine, Qt::FlatCap ) );
        painter.drawLine( pointAtAngle( center, innerRadius, angle ),
                          pointAtAngle( center, tickOuterRadius, angle ) );
    };

    if ( m_tickLength > 0.0 && minorTickCount > 0 )
    {
        for ( int interval = 0; interval < majorIntervalCount; ++interval )
        {
            for ( int minorIndex = 1; minorIndex <= minorTickCount; ++minorIndex )
            {
                const qreal position = static_cast<qreal>( minorIndex ) / ( minorTickCount + 1 );
                drawTick( ( interval + position ) / majorIntervalCount,
                          m_tickLength,
                          m_tickWidth );
            }
        }
    }
    if ( m_majorTickLength > 0.0 )
    {
        for ( int index = 0; index < majorTickCount; ++index )
        {
            drawTick( static_cast<qreal>( index ) / majorIntervalCount,
                      m_majorTickLength,
                      m_majorTickWidth );
        }
    }

    if ( m_labelsVisible )
    {
        QFont labelFont = font();
        labelFont.setPixelSize( qMin( m_labelFontPixelSize,
                                      qMax( 1, qRound( side * 0.12 ) ) ) );
        painter.setFont( labelFont );
        painter.setPen( resolvedLabelColor );
        const QFontMetricsF metrics( labelFont );
        const qreal labelRadius = qMax( 0.0,
                                        tickOuterRadius - m_majorTickLength - m_labelPadding );
        for ( int index = 0; index < majorTickCount; ++index )
        {
            const qreal fraction = static_cast<qreal>( index ) / majorIntervalCount;
            const qreal value = m_minimum + ( m_maximum - m_minimum ) * fraction;
            const QPointF labelCenter = pointAtAngle( center,
                                                       labelRadius,
                                                       m_minimumAngle + sweep * fraction );
            const QString text = QString::number( value, 'g', 4 );
            const QRectF bounds = metrics.boundingRect( text );
            const QRectF textRect( labelCenter.x() - bounds.width() * 0.5 - 2.0,
                                   labelCenter.y() - metrics.height() * 0.5,
                                   bounds.width() + 4.0,
                                   metrics.height() );
            painter.drawText( textRect, Qt::AlignCenter, text );
        }
    }

    const QPointF needleCenter = center + QPointF( m_needleOffset.x() * radius,
                                                    m_needleOffset.y() * radius );
    if ( m_needleStyle != NoNeedle )
    {
        for ( const ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
        {
            if ( !item || !item->isVisible() )
            {
                continue;
            }
            const qreal angle = m_minimumAngle + sweep * valueFraction( displayedValue( item ) );
            const qreal radians = qDegreesToRadians( angle );
            const QPointF direction( std::sin( radians ), -std::cos( radians ) );
            const QPointF normal( -direction.y(), direction.x() );
            const QPointF tip = needleCenter + direction * ( radius * m_needleLength );
            const QColor color = itemColor( item );
            if ( m_needleStyle == LineNeedle )
            {
                const QPointF tail = needleCenter - direction * qMax( 2.0, m_needleWidth * 1.8 );
                painter.setPen( QPen( color, m_needleWidth, Qt::SolidLine, Qt::RoundCap ) );
                painter.setBrush( Qt::NoBrush );
                painter.drawLine( tail, tip );
            }
            else
            {
                const QPointF tail = needleCenter - direction * qMax( 2.0, m_needleWidth );
                const qreal halfWidth = m_needleWidth * 0.5;
                QPolygonF needle;
                needle << tip
                       << needleCenter + normal * halfWidth
                       << tail
                       << needleCenter - normal * halfWidth;
                painter.setPen( Qt::NoPen );
                painter.setBrush( color );
                painter.drawPolygon( needle );
            }
        }
    }

    if ( m_hubVisible && m_hubRadius > 0.0 )
    {
        const QColor color = enabled && m_hubColor.isValid() ? m_hubColor : accentColor;
        painter.setPen( Qt::NoPen );
        painter.setBrush( color );
        painter.drawEllipse( needleCenter, m_hubRadius, m_hubRadius );
    }

    QFont titleFont = font();
    titleFont.setPixelSize( qMin( m_titleFontPixelSize,
                                  qMax( 1, qRound( side * 0.12 ) ) ) );
    QFont detailFont = font();
    detailFont.setPixelSize( qMin( m_detailFontPixelSize,
                                   qMax( 1, qRound( side * 0.12 ) ) ) );
    detailFont.setWeight( QFont::DemiBold );
    const QFontMetricsF titleMetrics( titleFont );
    const QFontMetricsF detailMetrics( detailFont );

    for ( const ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
    {
        if ( !item || !item->isVisible() )
        {
            continue;
        }
        const QColor color = itemColor( item );
        if ( m_titleVisible && !item->label().isEmpty() )
        {
            const QPointF titleCenter = center + QPointF( item->titleOffset().x() * radius,
                                                           item->titleOffset().y() * radius );
            const qreal width = qMax( 40.0, titleMetrics.horizontalAdvance( item->label() ) + 8.0 );
            const QRectF titleRect( titleCenter.x() - width * 0.5,
                                    titleCenter.y() - titleMetrics.height() * 0.5,
                                    width,
                                    titleMetrics.height() );
            painter.setFont( titleFont );
            painter.setPen( resolvedTitleColor );
            painter.drawText( titleRect, Qt::AlignCenter, item->label() );
        }

        if ( m_detailVisible )
        {
            const QString valueText = QString::number( displayedValue( item ),
                                                       'f',
                                                       m_valueDecimals )
                                      + m_valueSuffix;
            const QPointF detailCenter = center + QPointF( item->detailOffset().x() * radius,
                                                            item->detailOffset().y() * radius );
            const qreal textWidth = detailMetrics.horizontalAdvance( valueText );
            const qreal detailWidth = textWidth + m_detailBadgePadding * 2.0;
            const qreal detailHeight = detailMetrics.height() + 2.0;
            const QRectF detailRect( detailCenter.x() - detailWidth * 0.5,
                                     detailCenter.y() - detailHeight * 0.5,
                                     detailWidth,
                                     detailHeight );
            painter.setFont( detailFont );
            if ( m_detailBadgeVisible )
            {
                painter.setPen( Qt::NoPen );
                painter.setBrush( color );
                painter.drawRoundedRect( detailRect, 3.0, 3.0 );
                painter.setPen( m_detailTextColor.isValid()
                                    ? m_detailTextColor
                                    : QColor( Qt::white ) );
            }
            else
            {
                painter.setPen( m_detailTextColor.isValid()
                                    ? m_detailTextColor
                                    : color );
            }
            painter.drawText( detailRect, Qt::AlignCenter, valueText );
        }
    }
}

qreal ExMultiRadialGauge::sweepAngle() const
{
    qreal sweep = std::fmod( m_maximumAngle - m_minimumAngle, FullCircle );
    if ( sweep <= 0.0 )
    {
        sweep += FullCircle;
    }
    return sweep;
}

qreal ExMultiRadialGauge::valueFraction( qreal value ) const
{
    const qreal range = m_maximum - m_minimum;
    if ( range <= 0.0 )
    {
        return 0.0;
    }
    return qBound( 0.0, ( value - m_minimum ) / range, 1.0 );
}

qreal ExMultiRadialGauge::displayedValue( const ExMultiRadialGaugeItem* item ) const
{
    return item ? m_displayedValues.value( item, item->value() ) : 0.0;
}

void ExMultiRadialGauge::connectItem( ExMultiRadialGaugeItem* item )
{
    connect( item, &ExMultiRadialGaugeItem::valueChanged, this, [this]( qreal )
             {
                 startValueAnimation();
             } );
    connect( item, &ExMultiRadialGaugeItem::itemChanged, this, [this]
             {
                 update();
                 emit itemsChanged();
             } );
    connect( item, &QObject::destroyed, this, [this, item]
             {
                 if ( m_items.removeOne( item ) )
                 {
                     m_displayedValues.remove( item );
                     m_animationStartValues.remove( item );
                     update();
                     emit itemsChanged();
                 }
             } );
}

void ExMultiRadialGauge::startValueAnimation()
{
    if ( !m_valueAnimation || m_valueAnimationDuration == 0 || !isVisible() )
    {
        synchronizeDisplayedValues();
        return;
    }
    m_valueAnimation->stop();
    m_animationStartValues = m_displayedValues;
    m_valueAnimation->setDuration( m_valueAnimationDuration );
    m_valueAnimation->start();
}

void ExMultiRadialGauge::synchronizeDisplayedValues()
{
    for ( const ExMultiRadialGaugeItem* item : std::as_const( m_items ) )
    {
        m_displayedValues.insert( item, item->value() );
    }
    m_animationStartValues.clear();
    update();
}
