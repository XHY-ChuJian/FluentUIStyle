#include "exmultiprogressring.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QFontMetricsF>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>
#include <QVariantAnimation>
#include <QtMath>

#include <cmath>
#include <utility>

namespace
{
constexpr qreal FullCircle = 360.0;

QPalette::ColorRole accentRole()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return QPalette::Accent;
#else
    return QPalette::Highlight;
#endif
}
}

ExMultiProgressRingItem::ExMultiProgressRingItem( QObject* parent )
    : QObject( parent )
{
}

ExMultiProgressRingItem::ExMultiProgressRingItem( const QString& label,
                                                  qreal value,
                                                  const QColor& color,
                                                  QObject* parent )
    : QObject( parent )
    , m_label( label )
    , m_value( qIsFinite( value ) ? value : 0.0 )
    , m_color( color )
{
}

void ExMultiProgressRingItem::setLabel( QString label )
{
    if ( m_label == label )
    {
        return;
    }

    m_label = std::move( label );
    emit labelChanged( m_label );
    emit itemChanged();
}

void ExMultiProgressRingItem::setValue( qreal value )
{
    if ( !qIsFinite( value ) || qFuzzyCompare( m_value + 1.0, value + 1.0 ) )
    {
        return;
    }

    m_value = value;
    emit valueChanged( value );
    emit itemChanged();
}

void ExMultiProgressRingItem::setColor( QColor color )
{
    if ( m_color == color )
    {
        return;
    }

    m_color = color;
    emit colorChanged( color );
    emit itemChanged();
}

ExMultiProgressRing::ExMultiProgressRing( QWidget* parent )
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
                 for ( const ExMultiProgressRingItem* item : std::as_const( m_items ) )
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

#define EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( Type, Property, Setter ) \
    void ExMultiProgressRing::Setter( Type value ) \
    { \
        if ( m_##Property == value ) \
        { \
            return; \
        } \
        m_##Property = value; \
        update(); \
        emit Property##Changed( value ); \
    }

EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( bool, trackVisible, setTrackVisible )
EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( bool, detailsVisible, setDetailsVisible )
EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( bool, valueBadgeVisible, setValueBadgeVisible )
EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( QColor, trackColor, setTrackColor )
EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( QColor, labelColor, setLabelColor )
EX_MULTI_PROGRESS_RING_SIMPLE_SETTER( QString, valueSuffix, setValueSuffix )

#undef EX_MULTI_PROGRESS_RING_SIMPLE_SETTER

void ExMultiProgressRing::setMinimum( qreal minimum )
{
    if ( !qIsFinite( minimum ) || qFuzzyCompare( m_minimum + 1.0, minimum + 1.0 ) )
    {
        return;
    }

    m_minimum = minimum;
    update();
    emit minimumChanged( minimum );
}

void ExMultiProgressRing::setMaximum( qreal maximum )
{
    if ( !qIsFinite( maximum ) || qFuzzyCompare( m_maximum + 1.0, maximum + 1.0 ) )
    {
        return;
    }

    m_maximum = maximum;
    update();
    emit maximumChanged( maximum );
}

void ExMultiProgressRing::setRange( qreal minimum, qreal maximum )
{
    if ( !qIsFinite( minimum ) || !qIsFinite( maximum ) )
    {
        return;
    }

    setMinimum( minimum );
    setMaximum( maximum );
}

void ExMultiProgressRing::setStartAngle( qreal angle )
{
    if ( !qIsFinite( angle ) )
    {
        return;
    }

    angle = std::fmod( angle, FullCircle );
    if ( qFuzzyCompare( m_startAngle + 1.0, angle + 1.0 ) )
    {
        return;
    }

    m_startAngle = angle;
    update();
    emit startAngleChanged( angle );
}

void ExMultiProgressRing::setSweepAngle( qreal angle )
{
    if ( !qIsFinite( angle ) )
    {
        return;
    }

    angle = qBound( 0.0, angle, FullCircle );
    if ( qFuzzyCompare( m_sweepAngle + 1.0, angle + 1.0 ) )
    {
        return;
    }

    m_sweepAngle = angle;
    update();
    emit sweepAngleChanged( angle );
}

void ExMultiProgressRing::setRingWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qBound( 0.5, width, 100.0 );
    if ( qFuzzyCompare( m_ringWidth, width ) )
    {
        return;
    }

    m_ringWidth = width;
    updateGeometry();
    update();
    emit ringWidthChanged( width );
}

void ExMultiProgressRing::setRingSpacing( qreal spacing )
{
    if ( !qIsFinite( spacing ) )
    {
        return;
    }

    spacing = qBound( 0.0, spacing, 100.0 );
    if ( qFuzzyCompare( m_ringSpacing + 1.0, spacing + 1.0 ) )
    {
        return;
    }

    m_ringSpacing = spacing;
    updateGeometry();
    update();
    emit ringSpacingChanged( spacing );
}

void ExMultiProgressRing::setRingPadding( qreal padding )
{
    if ( !qIsFinite( padding ) )
    {
        return;
    }

    padding = qBound( 0.0, padding, 200.0 );
    if ( qFuzzyCompare( m_ringPadding + 1.0, padding + 1.0 ) )
    {
        return;
    }

    m_ringPadding = padding;
    updateGeometry();
    update();
    emit ringPaddingChanged( padding );
}

void ExMultiProgressRing::setCapStyle( Qt::PenCapStyle style )
{
    if ( style != Qt::FlatCap && style != Qt::SquareCap && style != Qt::RoundCap )
    {
        return;
    }
    if ( m_capStyle == style )
    {
        return;
    }

    m_capStyle = style;
    update();
    emit capStyleChanged( style );
}

void ExMultiProgressRing::setValueDecimals( int decimals )
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

void ExMultiProgressRing::setLabelFontPixelSize( int size )
{
    size = qBound( 0, size, 200 );
    if ( m_labelFontPixelSize == size )
    {
        return;
    }

    m_labelFontPixelSize = size;
    update();
    emit labelFontPixelSizeChanged( size );
}

void ExMultiProgressRing::setValueFontPixelSize( int size )
{
    size = qBound( 0, size, 200 );
    if ( m_valueFontPixelSize == size )
    {
        return;
    }

    m_valueFontPixelSize = size;
    update();
    emit valueFontPixelSizeChanged( size );
}

void ExMultiProgressRing::setValueAnimationDuration( int duration )
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

QList<ExMultiProgressRingItem*> ExMultiProgressRing::items() const
{
    return m_items;
}

ExMultiProgressRingItem* ExMultiProgressRing::addItem( const QString& label,
                                                       qreal value,
                                                       const QColor& color )
{
    auto* item = new ExMultiProgressRingItem( label, value, color, this );
    addItem( item );
    return item;
}

void ExMultiProgressRing::addItem( ExMultiProgressRingItem* item )
{
    if ( !item || m_items.contains( item ) )
    {
        return;
    }

    item->setParent( this );
    m_items.append( item );
    m_displayedValues.insert( item, item->value() );
    connectItem( item );
    updateGeometry();
    update();
    emit itemsChanged();
}

void ExMultiProgressRing::removeItem( ExMultiProgressRingItem* item )
{
    if ( !item || !m_items.removeOne( item ) )
    {
        return;
    }

    disconnect( item, nullptr, this, nullptr );
    m_displayedValues.remove( item );
    m_animationStartValues.remove( item );
    item->deleteLater();
    updateGeometry();
    update();
    emit itemsChanged();
}

void ExMultiProgressRing::clearItems()
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
    for ( ExMultiProgressRingItem* item : oldItems )
    {
        disconnect( item, nullptr, this, nullptr );
        item->deleteLater();
    }
    updateGeometry();
    update();
    emit itemsChanged();
}

QSize ExMultiProgressRing::sizeHint() const
{
    return QSize( 220, 220 );
}

QSize ExMultiProgressRing::minimumSizeHint() const
{
    return QSize( 80, 80 );
}

void ExMultiProgressRing::paintEvent( QPaintEvent* event )
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
    const QColor fallbackRingColor = option.palette.color( colorGroup, accentRole() );
    const QColor resolvedTrackColor = enabled && m_trackColor.isValid()
                                          ? m_trackColor
                                          : option.palette.color( colorGroup, QPalette::Mid );
    const QColor resolvedLabelColor = enabled && m_labelColor.isValid()
                                          ? m_labelColor
                                          : option.palette.color( colorGroup, QPalette::Text );

    const qreal side = qMin( width(), height() );
    const QPointF center = QRectF( rect() ).center();
    const qreal outerRadius = side * 0.5 - m_ringPadding - m_ringWidth * 0.5 - 1.0;
    const qreal painterStartAngle = 90.0 - m_startAngle;

    QPainter painter( this );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

    qreal innermostRadius = outerRadius;
    for ( qsizetype index = 0; index < m_items.size(); ++index )
    {
        const ExMultiProgressRingItem* item = m_items.at( index );
        const qreal radius = outerRadius - index * ( m_ringWidth + m_ringSpacing );
        if ( !item || radius <= m_ringWidth * 0.5 )
        {
            break;
        }
        innermostRadius = radius;

        const QRectF ringRect( center.x() - radius,
                               center.y() - radius,
                               radius * 2.0,
                               radius * 2.0 );
        if ( m_trackVisible && m_sweepAngle > 0.0 )
        {
            painter.setPen( QPen( resolvedTrackColor,
                                  m_ringWidth,
                                  Qt::SolidLine,
                                  m_capStyle ) );
            painter.drawArc( ringRect,
                             qRound( painterStartAngle * 16.0 ),
                             qRound( -m_sweepAngle * 16.0 ) );
        }

        const qreal fraction = itemFraction( displayedValue( item ) );
        if ( fraction <= 0.0 || m_sweepAngle <= 0.0 )
        {
            continue;
        }

        const QColor ringColor = enabled && item->color().isValid()
                                     ? item->color()
                                     : fallbackRingColor;
        painter.setPen( QPen( ringColor,
                              m_ringWidth,
                              Qt::SolidLine,
                              m_capStyle ) );
        painter.drawArc( ringRect,
                         qRound( painterStartAngle * 16.0 ),
                         qRound( -m_sweepAngle * fraction * 16.0 ) );
    }

    if ( !m_detailsVisible || m_items.isEmpty() )
    {
        return;
    }

    QFont labelFont = font();
    labelFont.setPixelSize( m_labelFontPixelSize > 0
                                ? m_labelFontPixelSize
                                : qBound( 9, qRound( side * 0.045 ), 13 ) );
    QFont valueFont = font();
    valueFont.setPixelSize( m_valueFontPixelSize > 0
                                ? m_valueFontPixelSize
                                : qBound( 9, qRound( side * 0.045 ), 13 ) );
    valueFont.setWeight( QFont::DemiBold );

    const QFontMetricsF labelMetrics( labelFont );
    const QFontMetricsF valueMetrics( valueFont );
    const qreal badgeHeight = valueMetrics.height() + 2.0;
    const qreal rowSpacing = qMax( 3.0, side * 0.012 );
    const qreal rowHeight = labelMetrics.height() + 2.0 + badgeHeight;
    const qreal totalHeight = m_items.size() * rowHeight
                              + qMax<qsizetype>( 0, m_items.size() - 1 ) * rowSpacing;
    qreal rowTop = center.y() - totalHeight * 0.5;
    const qreal contentHalfWidth = qMax( 16.0,
                                         innermostRadius - m_ringWidth * 0.5 - 8.0 );

    for ( const ExMultiProgressRingItem* item : std::as_const( m_items ) )
    {
        if ( !item )
        {
            continue;
        }

        const QColor itemColor = enabled && item->color().isValid()
                                     ? item->color()
                                     : fallbackRingColor;
        painter.setFont( labelFont );
        painter.setPen( resolvedLabelColor );
        painter.drawText( QRectF( center.x() - contentHalfWidth,
                                  rowTop,
                                  contentHalfWidth * 2.0,
                                  labelMetrics.height() ),
                          Qt::AlignCenter,
                          item->label() );

        const QString valueText = QString::number( displayedValue( item ),
                                                   'f',
                                                   m_valueDecimals )
                                  + m_valueSuffix;
        const qreal valueTop = rowTop + labelMetrics.height() + 2.0;
        painter.setFont( valueFont );
        if ( m_valueBadgeVisible )
        {
            const qreal badgeWidth = qMin( contentHalfWidth * 2.0,
                                           valueMetrics.horizontalAdvance( valueText ) + 16.0 );
            const QRectF badgeRect( center.x() - badgeWidth * 0.5,
                                    valueTop,
                                    badgeWidth,
                                    badgeHeight );
            painter.setPen( QPen( itemColor, 1.5 ) );
            painter.setBrush( Qt::NoBrush );
            painter.drawRoundedRect( badgeRect,
                                     badgeRect.height() * 0.5,
                                     badgeRect.height() * 0.5 );
            painter.setPen( itemColor );
            painter.drawText( badgeRect, Qt::AlignCenter, valueText );
        }
        else
        {
            painter.setPen( itemColor );
            painter.drawText( QRectF( center.x() - contentHalfWidth,
                                      valueTop,
                                      contentHalfWidth * 2.0,
                                      badgeHeight ),
                              Qt::AlignCenter,
                              valueText );
        }

        rowTop += rowHeight + rowSpacing;
    }
}

qreal ExMultiProgressRing::itemFraction( qreal value ) const
{
    const qreal range = m_maximum - m_minimum;
    if ( range <= 0.0 )
    {
        return 0.0;
    }
    return qBound( 0.0, ( value - m_minimum ) / range, 1.0 );
}

qreal ExMultiProgressRing::displayedValue( const ExMultiProgressRingItem* item ) const
{
    return item ? m_displayedValues.value( item, item->value() ) : 0.0;
}

void ExMultiProgressRing::connectItem( ExMultiProgressRingItem* item )
{
    connect( item, &ExMultiProgressRingItem::valueChanged, this, [this]( qreal )
             {
                 startValueAnimation();
                 emit itemsChanged();
             } );
    connect( item, &ExMultiProgressRingItem::labelChanged, this, [this]( const QString& )
             {
                 update();
                 emit itemsChanged();
             } );
    connect( item, &ExMultiProgressRingItem::colorChanged, this, [this]( const QColor& )
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
                     updateGeometry();
                     update();
                     emit itemsChanged();
                 }
             } );
}

void ExMultiProgressRing::startValueAnimation()
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

void ExMultiProgressRing::synchronizeDisplayedValues()
{
    for ( const ExMultiProgressRingItem* item : std::as_const( m_items ) )
    {
        m_displayedValues.insert( item, item->value() );
    }
    m_animationStartValues.clear();
    update();
}
