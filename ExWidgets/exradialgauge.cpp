#include "exradialgauge.h"

#include <QAbstractAnimation>
#include <QBrush>
#include <QConicalGradient>
#include <QEasingCurve>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QSizePolicy>
#include <QStyleOptionSlider>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtMath>

#include <cmath>

namespace
{
constexpr qreal FullCircle = 360.0;
constexpr qreal EndpointExtensionAngle = 1.0;
constexpr int MaximumTickCount = 720;
constexpr int MaximumMajorTickCount = 180;

QPointF pointAtAngle( const QPointF& center, qreal radius, qreal angle )
{
    const qreal radians = qDegreesToRadians( angle );
    return QPointF( center.x() + radius * std::sin( radians ), center.y() - radius * std::cos( radians ) );
}

qreal circularDistance( qreal first, qreal second )
{
    return std::abs( std::remainder( first - second, FullCircle ) );
}

QPalette::ColorRole accentRole()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return QPalette::Accent;
#else
    return QPalette::Highlight;
#endif
}
}

ExRadialGaugeRange::ExRadialGaugeRange( QObject* parent )
    : QObject( parent )
{
}

ExRadialGaugeRange::ExRadialGaugeRange( int fromValue,
                                        int toValue,
                                        const QColor& color,
                                        QObject* parent )
    : QObject( parent )
    , m_fromValue( fromValue )
    , m_toValue( toValue )
    , m_color( color )
{
}

void ExRadialGaugeRange::setFromValue( int value )
{
    if ( m_fromValue == value )
    {
        return;
    }

    m_fromValue = value;
    emit fromValueChanged( value );
    emit rangeChanged();
}

void ExRadialGaugeRange::setToValue( int value )
{
    if ( m_toValue == value )
    {
        return;
    }

    m_toValue = value;
    emit toValueChanged( value );
    emit rangeChanged();
}

void ExRadialGaugeRange::setColor( QColor color )
{
    if ( m_color == color )
    {
        return;
    }

    m_color = color;
    emit colorChanged( color );
    emit rangeChanged();
}

ExRadialGauge::ExRadialGauge( QWidget* parent )
    : QDial( parent )
{
    setWrapping( false );
    setTracking( true );
    setFocusPolicy( Qt::StrongFocus );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

    m_valueAnimation = new QVariantAnimation( this );
    m_valueAnimation->setDuration( m_valueAnimationDuration );
    m_valueAnimation->setEasingCurve( QEasingCurve::OutCubic );
    connect( m_valueAnimation, &QVariantAnimation::valueChanged, this, [this]( const QVariant& animatedValue )
             {
                 QDial::setValue( qRound( animatedValue.toReal() ) );
             } );
    connect( m_valueAnimation, &QVariantAnimation::finished, this, [this]
             {
                 QDial::setValue( m_valueAnimation->endValue().toInt() );
             } );
    connect( this, &QDial::rangeChanged, this, [this]
             {
                 m_valueAnimation->stop();
             } );
}

#define EX_RADIAL_GAUGE_SIMPLE_SETTER( Type, Property, Setter ) \
    void ExRadialGauge::Setter( Type value ) \
    { \
        if ( m_##Property == value ) \
        { \
            return; \
        } \
        m_##Property = value; \
        update(); \
        emit Property##Changed( value ); \
    }

EX_RADIAL_GAUGE_SIMPLE_SETTER( bool, labelsVisible, setLabelsVisible )
EX_RADIAL_GAUGE_SIMPLE_SETTER( bool, hubVisible, setHubVisible )
EX_RADIAL_GAUGE_SIMPLE_SETTER( bool, valueVisible, setValueVisible )
EX_RADIAL_GAUGE_SIMPLE_SETTER( bool, progressGradientEnabled, setProgressGradientEnabled )
EX_RADIAL_GAUGE_SIMPLE_SETTER( bool, sweepAreaVisible, setSweepAreaVisible )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QString, title, setTitle )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QString, unit, setUnit )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, progressGradientStartColor, setProgressGradientStartColor )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, progressGradientEndColor, setProgressGradientEndColor )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, needleColor, setNeedleColor )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, tickColor, setTickColor )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, labelColor, setLabelColor )
EX_RADIAL_GAUGE_SIMPLE_SETTER( QColor, valueColor, setValueColor )

#undef EX_RADIAL_GAUGE_SIMPLE_SETTER

void ExRadialGauge::setScaleMode( ScaleMode mode )
{
    if ( mode < TrackScale || mode > RangeScale || m_scaleMode == mode )
    {
        return;
    }

    m_scaleMode = mode;
    const Qt::PenCapStyle defaultCapStyle = mode == ProgressScale ? Qt::RoundCap : Qt::FlatCap;
    const bool trackCapStyleDidChange = m_trackCapStyle != defaultCapStyle;
    const bool ringCapStyleDidChange = m_ringCapStyle != defaultCapStyle;
    m_trackCapStyle = defaultCapStyle;
    m_ringCapStyle = defaultCapStyle;
    update();
    emit scaleModeChanged( mode );
    if ( trackCapStyleDidChange )
    {
        emit trackCapStyleChanged( m_trackCapStyle );
    }
    if ( ringCapStyleDidChange )
    {
        emit ringCapStyleChanged( m_ringCapStyle );
    }
}

void ExRadialGauge::setNeedleStyle( NeedleStyle style )
{
    if ( style < NoNeedle || style > TriangleNeedle || m_needleStyle == style )
    {
        return;
    }

    m_needleStyle = style;
    update();
    emit needleStyleChanged( style );
}

void ExRadialGauge::setValuePosition( ValuePosition position )
{
    if ( position < CenterValue || position > BottomValue || m_valuePosition == position )
    {
        return;
    }

    m_valuePosition = position;
    update();
    emit valuePositionChanged( position );
}

void ExRadialGauge::setInteractive( bool interactive )
{
    if ( m_interactive == interactive )
    {
        return;
    }

    if ( !interactive )
    {
        if ( isSliderDown() )
        {
            if ( !hasTracking() )
            {
                setValue( sliderPosition() );
            }
            setSliderDown( false );
        }
        m_interactiveFocusPolicy = focusPolicy();
        setFocusPolicy( Qt::NoFocus );
    }
    else
    {
        setFocusPolicy( m_interactiveFocusPolicy );
    }

    m_interactive = interactive;
    emit interactiveChanged( interactive );
}

void ExRadialGauge::setValueAnimationDuration( int duration )
{
    duration = qBound( 0, duration, 5000 );
    if ( m_valueAnimationDuration == duration )
    {
        return;
    }

    m_valueAnimationDuration = duration;
    if ( m_valueAnimation )
    {
        m_valueAnimation->stop();
        m_valueAnimation->setDuration( duration );
    }
    update();
    emit valueAnimationDurationChanged( duration );
}

void ExRadialGauge::setSweepAreaOpacity( qreal opacity )
{
    if ( !qIsFinite( opacity ) )
    {
        return;
    }

    opacity = qBound( 0.0, opacity, 1.0 );
    if ( qFuzzyCompare( m_sweepAreaOpacity + 1.0, opacity + 1.0 ) )
    {
        return;
    }

    m_sweepAreaOpacity = opacity;
    update();
    emit sweepAreaOpacityChanged( opacity );
}

void ExRadialGauge::setMinimumAngle( qreal angle )
{
    if ( !qIsFinite( angle ) )
    {
        return;
    }

    angle = qBound( -FullCircle, angle, FullCircle );
    if ( qFuzzyCompare( m_minimumAngle, angle ) )
    {
        return;
    }

    m_minimumAngle = angle;
    update();
    emit minimumAngleChanged( angle );
}

void ExRadialGauge::setMaximumAngle( qreal angle )
{
    if ( !qIsFinite( angle ) )
    {
        return;
    }

    angle = qBound( -FullCircle, angle, FullCircle );
    if ( qFuzzyCompare( m_maximumAngle, angle ) )
    {
        return;
    }

    m_maximumAngle = angle;
    update();
    emit maximumAngleChanged( angle );
}

void ExRadialGauge::setMajorTickCount( int count )
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

void ExRadialGauge::setMinorTickCount( int count )
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

void ExRadialGauge::setScaleWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qMax( 0.5, width );
    if ( qFuzzyCompare( m_scaleWidth, width ) )
    {
        return;
    }

    m_scaleWidth = width;
    update();
    emit scaleWidthChanged( width );
}

void ExRadialGauge::setScalePadding( qreal padding )
{
    if ( !qIsFinite( padding ) )
    {
        return;
    }

    padding = qMax( 0.0, padding );
    if ( qFuzzyCompare( m_scalePadding, padding ) )
    {
        return;
    }

    m_scalePadding = padding;
    update();
    emit scalePaddingChanged( padding );
}

void ExRadialGauge::setTrackCapStyle( Qt::PenCapStyle style )
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

void ExRadialGauge::setRingCapStyle( Qt::PenCapStyle style )
{
    if ( style != Qt::FlatCap && style != Qt::SquareCap && style != Qt::RoundCap )
    {
        return;
    }
    if ( m_ringCapStyle == style )
    {
        return;
    }

    m_ringCapStyle = style;
    update();
    emit ringCapStyleChanged( style );
}

void ExRadialGauge::setNeedleWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qMax( 0.5, width );
    if ( qFuzzyCompare( m_needleWidth, width ) )
    {
        return;
    }

    m_needleWidth = width;
    update();
    emit needleWidthChanged( width );
}

void ExRadialGauge::setNeedleLength( qreal length )
{
    if ( !qIsFinite( length ) )
    {
        return;
    }

    length = qBound( 0.05, length, 1.0 );
    if ( qFuzzyCompare( m_needleLength, length ) )
    {
        return;
    }

    m_needleLength = length;
    update();
    emit needleLengthChanged( length );
}

void ExRadialGauge::setTickLength( qreal length )
{
    if ( !qIsFinite( length ) )
    {
        return;
    }

    length = qMax( 0.0, length );
    if ( qFuzzyCompare( m_tickLength, length ) )
    {
        return;
    }

    m_tickLength = length;
    update();
    emit tickLengthChanged( length );
}

void ExRadialGauge::setTickWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qMax( 0.5, width );
    if ( qFuzzyCompare( m_tickWidth, width ) )
    {
        return;
    }

    m_tickWidth = width;
    update();
    emit tickWidthChanged( width );
}

void ExRadialGauge::setMajorTickLength( qreal length )
{
    if ( !qIsFinite( length ) )
    {
        return;
    }

    length = qMax( 0.0, length );
    if ( qFuzzyCompare( m_majorTickLength, length ) )
    {
        return;
    }

    m_majorTickLength = length;
    update();
    emit majorTickLengthChanged( length );
}

void ExRadialGauge::setMajorTickWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qMax( 0.5, width );
    if ( qFuzzyCompare( m_majorTickWidth, width ) )
    {
        return;
    }

    m_majorTickWidth = width;
    update();
    emit majorTickWidthChanged( width );
}

void ExRadialGauge::setTickPadding( qreal padding )
{
    if ( !qIsFinite( padding ) )
    {
        return;
    }

    padding = qMax( 0.0, padding );
    if ( qFuzzyCompare( m_tickPadding, padding ) )
    {
        return;
    }

    m_tickPadding = padding;
    update();
    emit tickPaddingChanged( padding );
}

void ExRadialGauge::setLabelPadding( qreal padding )
{
    if ( !qIsFinite( padding ) )
    {
        return;
    }

    padding = qMax( 0.0, padding );
    if ( qFuzzyCompare( m_labelPadding, padding ) )
    {
        return;
    }

    m_labelPadding = padding;
    update();
    emit labelPaddingChanged( padding );
}

void ExRadialGauge::setLabelFontPixelSize( int size )
{
    size = qMax( 1, size );
    if ( m_labelFontPixelSize == size )
    {
        return;
    }

    m_labelFontPixelSize = size;
    update();
    emit labelFontPixelSizeChanged( size );
}

void ExRadialGauge::setHubRadius( qreal radius )
{
    if ( !qIsFinite( radius ) )
    {
        return;
    }

    radius = qMax( 0.0, radius );
    if ( qFuzzyCompare( m_hubRadius, radius ) )
    {
        return;
    }

    m_hubRadius = radius;
    update();
    emit hubRadiusChanged( radius );
}

void ExRadialGauge::setValueFontPixelSize( int size )
{
    size = qMax( 0, size );
    if ( m_valueFontPixelSize == size )
    {
        return;
    }

    m_valueFontPixelSize = size;
    update();
    emit valueFontPixelSizeChanged( size );
}

QSize ExRadialGauge::sizeHint() const
{
    return QSize( 240, 240 );
}

QSize ExRadialGauge::minimumSizeHint() const
{
    return QSize( 100, 100 );
}

QList<ExRadialGaugeRange*> ExRadialGauge::ranges() const
{
    return m_ranges;
}

ExRadialGaugeRange* ExRadialGauge::addRange( int fromValue, int toValue, const QColor& color )
{
    auto* range = new ExRadialGaugeRange( fromValue, toValue, color, this );
    m_ranges.append( range );

    connect( range, &ExRadialGaugeRange::rangeChanged, this, [this]
             {
                 update();
                 emit rangesChanged();
             } );
    connect( range, &QObject::destroyed, this, [this, range]
             {
                 if ( m_ranges.removeOne( range ) )
                 {
                     update();
                     emit rangesChanged();
                 }
             } );

    update();
    emit rangesChanged();
    return range;
}

void ExRadialGauge::removeRange( ExRadialGaugeRange* range )
{
    if ( !range || !m_ranges.removeOne( range ) )
    {
        return;
    }

    disconnect( range, nullptr, this, nullptr );
    range->deleteLater();
    update();
    emit rangesChanged();
}

void ExRadialGauge::clearRanges()
{
    if ( m_ranges.isEmpty() )
    {
        return;
    }

    const auto oldRanges = m_ranges;
    m_ranges.clear();
    for ( ExRadialGaugeRange* range : oldRanges )
    {
        disconnect( range, nullptr, this, nullptr );
        range->deleteLater();
    }

    update();
    emit rangesChanged();
}

void ExRadialGauge::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QStyleOptionSlider option;
    initStyleOption( &option );

    const bool enabled = option.state.testFlag( QStyle::State_Enabled );
    const QPalette::ColorGroup colorGroup = enabled
                                                  ? ( option.state.testFlag( QStyle::State_Active ) ? QPalette::Active : QPalette::Inactive )
                                                  : QPalette::Disabled;
    const QColor paletteAccentColor = option.palette.color( colorGroup, accentRole() );
    const QColor trackColor = option.palette.color( colorGroup, QPalette::Mid );
    const auto resolveColor = [enabled, &option, colorGroup]( const QColor& color, QPalette::ColorRole fallbackRole )
    {
        return enabled && color.isValid() ? color : option.palette.color( colorGroup, fallbackRole );
    };

    const QColor needlePaintColor = resolveColor( m_needleColor, accentRole() );
    QColor tickPaintColor = resolveColor( m_tickColor, QPalette::Text );
    QColor labelPaintColor = resolveColor( m_labelColor, QPalette::Text );
    const QColor valuePaintColor = resolveColor( m_valueColor, QPalette::Text );
    if ( !m_tickColor.isValid() )
    {
        tickPaintColor.setAlpha( enabled ? 150 : 80 );
    }
    if ( !m_labelColor.isValid() )
    {
        labelPaintColor.setAlpha( enabled ? 180 : 90 );
    }

    const qreal side = qMin( width(), height() );
    const QPointF center = QRectF( rect() ).center();
    const qreal radius = qMax( 1.0, side * 0.5 - m_scalePadding - m_scaleWidth * 0.5 - 1.0 );
    const QRectF scaleRect( center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0 );
    const qreal sweep = sweepAngle();
    const qreal endpointExtension = sweep < FullCircle - 0.001 ? EndpointExtensionAngle : 0.0;
    const qreal fraction = positionFraction();
    const qreal trackStartAngle = 90.0 - m_minimumAngle + endpointExtension;
    const qreal trackSpan = -( sweep + endpointExtension * 2.0 );

    QPainter painter( this );
    painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );

    const QColor progressStartColor = enabled
                                          ? ( m_progressGradientStartColor.isValid()
                                                  ? m_progressGradientStartColor
                                                  : paletteAccentColor.lighter( 135 ) )
                                          : paletteAccentColor;
    const QColor progressEndColor = enabled && m_progressGradientEndColor.isValid()
                                        ? m_progressGradientEndColor
                                        : paletteAccentColor;
    const auto makeAngularGradient = [&]( qreal sectionStartAngle,
                                          qreal sectionSpan,
                                          const QColor& startColor,
                                          const QColor& endColor )
    {
        qreal gradientAngle = std::fmod( sectionStartAngle + sectionSpan, FullCircle );
        if ( gradientAngle < 0.0 )
        {
            gradientAngle += FullCircle;
        }

        const qreal activeStop = qBound( 0.0, -sectionSpan / FullCircle, 1.0 );
        QConicalGradient gradient( center, gradientAngle );
        gradient.setColorAt( 0.0, endColor );
        gradient.setColorAt( activeStop, startColor );
        if ( activeStop < 1.0 )
        {
            gradient.setColorAt( 1.0, endColor );
        }
        return QBrush( gradient );
    };

    if ( m_scaleMode == ProgressScale && m_sweepAreaVisible && fraction > 0.0 && m_sweepAreaOpacity > 0.0 )
    {
        QColor areaStartColor = progressStartColor;
        QColor areaEndColor = progressEndColor;
        areaStartColor.setAlphaF( areaStartColor.alphaF() * m_sweepAreaOpacity );
        areaEndColor.setAlphaF( areaEndColor.alphaF() * m_sweepAreaOpacity );

        const qreal areaStartAngle = 90.0 - m_minimumAngle;
        const qreal areaSpan = -sweep * fraction;
        QPainterPath sweepArea;
        sweepArea.moveTo( center );
        sweepArea.lineTo( pointAtAngle( center, radius, m_minimumAngle ) );
        sweepArea.arcTo( scaleRect, areaStartAngle, areaSpan );
        sweepArea.closeSubpath();
        painter.fillPath( sweepArea,
                          makeAngularGradient( areaStartAngle,
                                               areaSpan,
                                               areaStartColor,
                                               areaEndColor ) );
    }

    QPen trackPen( trackColor, m_scaleWidth, Qt::SolidLine, m_trackCapStyle );
    painter.setPen( trackPen );
    painter.drawArc( scaleRect, qRound( trackStartAngle * 16.0 ), qRound( trackSpan * 16.0 ) );

    const auto drawScaleSection = [&]( qreal firstFraction,
                                       qreal secondFraction,
                                       const QBrush& brush,
                                       Qt::PenCapStyle capStyle )
    {
        firstFraction = qBound( 0.0, firstFraction, 1.0 );
        secondFraction = qBound( 0.0, secondFraction, 1.0 );
        if ( firstFraction > secondFraction )
        {
            qSwap( firstFraction, secondFraction );
        }
        if ( qFuzzyCompare( firstFraction, secondFraction ) )
        {
            return;
        }

        qreal sectionStartAngle = 90.0 - ( m_minimumAngle + sweep * firstFraction );
        qreal sectionSpan = -sweep * ( secondFraction - firstFraction );
        if ( endpointExtension > 0.0 && qFuzzyIsNull( firstFraction ) )
        {
            sectionStartAngle += endpointExtension;
            sectionSpan -= endpointExtension;
        }
        if ( endpointExtension > 0.0 && qFuzzyCompare( secondFraction, 1.0 ) )
        {
            sectionSpan -= endpointExtension;
        }
        painter.setPen( QPen( brush, m_scaleWidth, Qt::SolidLine, capStyle ) );
        painter.drawArc( scaleRect, qRound( sectionStartAngle * 16.0 ), qRound( sectionSpan * 16.0 ) );
    };

    if ( m_scaleMode == ProgressScale && fraction > 0.0 )
    {
        QBrush progressBrush( paletteAccentColor );
        if ( enabled && m_progressGradientEnabled )
        {
            const qreal sectionStartAngle = 90.0 - m_minimumAngle + endpointExtension;
            qreal sectionSpan = -( sweep * fraction + endpointExtension );
            if ( qFuzzyCompare( fraction, 1.0 ) )
            {
                sectionSpan -= endpointExtension;
            }

            progressBrush = makeAngularGradient( sectionStartAngle,
                                                  sectionSpan,
                                                  progressStartColor,
                                                  progressEndColor );
        }
        drawScaleSection( 0.0, fraction, progressBrush, m_ringCapStyle );
    }
    else if ( m_scaleMode == RangeScale )
    {
        for ( const ExRadialGaugeRange* rangeItem : m_ranges )
        {
            if ( !rangeItem )
            {
                continue;
            }

            QColor rangeColor = rangeItem->color();
            if ( !enabled || !rangeColor.isValid() )
            {
                rangeColor = paletteAccentColor;
            }
            drawScaleSection( valueFraction( rangeItem->fromValue() ),
                              valueFraction( rangeItem->toValue() ),
                              QBrush( rangeColor ),
                              m_ringCapStyle );
        }
    }

    const qint64 range = static_cast<qint64>( maximum() ) - static_cast<qint64>( minimum() );
    const int majorTickCount = range > 0
                                   ? qMin( m_majorTickCount,
                                           static_cast<int>( qMin<qint64>( range + 1, MaximumMajorTickCount ) ) )
                                   : 0;
    const int majorIntervalCount = qMax( 1, majorTickCount - 1 );
    const int maximumMinorTickCount = qMax( 0, MaximumTickCount / majorIntervalCount - 1 );
    const int minorTickCount = qMin( m_minorTickCount, maximumMinorTickCount );
    const auto visualFraction = [this]( qreal logicalFraction )
    {
        return invertedAppearance() ? 1.0 - logicalFraction : logicalFraction;
    };
    const auto forEachMajorTick = [&]( const auto& callback )
    {
        for ( int index = 0; index < majorTickCount; ++index )
        {
            const qreal logicalFraction = static_cast<qreal>( index ) / majorIntervalCount;
            callback( index, logicalFraction, visualFraction( logicalFraction ) );
        }
    };

    if ( majorTickCount >= 2 && ( m_tickLength > 0.0 || m_majorTickLength > 0.0 ) )
    {
        const qreal tickOuterRadius = qMax( 0.0, radius - m_tickPadding );

        const auto drawTick = [&]( qreal tickFraction, qreal tickLength, qreal tickWidth )
        {
            const qreal angle = m_minimumAngle + sweep * tickFraction;
            const qreal tickInnerRadius = qMax( 0.0, tickOuterRadius - tickLength );
            painter.setPen( QPen( tickPaintColor, tickWidth, Qt::SolidLine, Qt::FlatCap ) );
            painter.drawLine( pointAtAngle( center, tickInnerRadius, angle ),
                              pointAtAngle( center, tickOuterRadius, angle ) );
        };

        if ( m_tickLength > 0.0 && minorTickCount > 0 )
        {
            for ( int interval = 0; interval < majorIntervalCount; ++interval )
            {
                for ( int minorIndex = 1; minorIndex <= minorTickCount; ++minorIndex )
                {
                    const qreal positionInInterval = static_cast<qreal>( minorIndex )
                                                     / ( minorTickCount + 1 );
                    const qreal logicalFraction = ( interval + positionInInterval ) / majorIntervalCount;
                    drawTick( visualFraction( logicalFraction ), m_tickLength, m_tickWidth );
                }
            }
        }
        if ( m_majorTickLength > 0.0 )
        {
            forEachMajorTick( [&]( int, qreal, qreal tickFraction )
                              {
                                  drawTick( tickFraction, m_majorTickLength, m_majorTickWidth );
                              } );
        }
    }

    if ( m_labelsVisible && majorTickCount >= 2 )
    {
        QFont labelFont = font();
        labelFont.setPixelSize( qMin( m_labelFontPixelSize, qMax( 1, qRound( side * 0.12 ) ) ) );
        painter.setFont( labelFont );
        painter.setPen( labelPaintColor );
        const QFontMetricsF metrics( labelFont );
        const qreal labelRadius = qMax( 0.0, radius - m_scaleWidth * 0.5 - m_labelPadding );

        const auto drawLabel = [&]( qint64 labelValue, qreal labelFraction )
        {
            const qreal angle = m_minimumAngle + sweep * labelFraction;
            const QPointF labelCenter = pointAtAngle( center, labelRadius, angle );
            const QString text = QString::number( labelValue );
            const QRectF textBounds = metrics.boundingRect( text );
            const QRectF textRect( labelCenter.x() - textBounds.width() * 0.5 - 2.0,
                                   labelCenter.y() - metrics.height() * 0.5,
                                   textBounds.width() + 4.0,
                                   metrics.height() );
            painter.drawText( textRect, Qt::AlignCenter, text );
        };

        forEachMajorTick( [&]( int index, qreal logicalFraction, qreal labelFraction )
                          {
                              qint64 labelValue = static_cast<qint64>( minimum() )
                                                  + qRound64( range * logicalFraction );
                              if ( index == majorTickCount - 1 )
                              {
                                  labelValue = maximum();
                              }
                              drawLabel( labelValue, labelFraction );
                          } );
    }

    const qreal needleAngle = m_minimumAngle + sweep * fraction;
    if ( m_needleStyle == LineNeedle )
    {
        painter.setPen( QPen( needlePaintColor, m_needleWidth, Qt::SolidLine, Qt::RoundCap ) );
        painter.setBrush( Qt::NoBrush );
        painter.drawLine( center, pointAtAngle( center, radius * m_needleLength, needleAngle ) );
    }
    else if ( m_needleStyle == TriangleNeedle )
    {
        const qreal radians = qDegreesToRadians( needleAngle );
        const QPointF direction( std::sin( radians ), -std::cos( radians ) );
        const QPointF normal( -direction.y(), direction.x() );
        const QPointF tip = center + direction * ( radius * m_needleLength );
        const QPointF tail = center - direction * qMax( 2.0, m_needleWidth );
        const qreal halfWidth = m_needleWidth * 0.5;

        QPolygonF needle;
        needle << tip << center + normal * halfWidth << tail << center - normal * halfWidth;
        painter.setPen( Qt::NoPen );
        painter.setBrush( needlePaintColor );
        painter.drawPolygon( needle );
    }

    if ( m_needleStyle != NoNeedle && m_hubVisible && m_hubRadius > 0.0 )
    {
        painter.setPen( Qt::NoPen );
        painter.setBrush( needlePaintColor );
        painter.drawEllipse( center, m_hubRadius, m_hubRadius );
        painter.setBrush( option.palette.color( colorGroup, QPalette::Base ) );
        painter.drawEllipse( center, m_hubRadius * 0.38, m_hubRadius * 0.38 );
    }

    if ( m_valueVisible || !m_title.isEmpty() )
    {
        QFont valueFont = font();
        valueFont.setWeight( QFont::DemiBold );
        const int automaticValueSize = qBound( 14, qRound( side * 0.12 ), 38 );
        valueFont.setPixelSize( m_valueFontPixelSize > 0 ? m_valueFontPixelSize : automaticValueSize );
        painter.setFont( valueFont );
        painter.setPen( valuePaintColor );

        const QFontMetricsF metrics( valueFont );
        const qreal titleOffset = m_title.isEmpty()
                                      ? 0.0
                                      : qBound( 2.0, metrics.height() * 0.1, 5.0 );
        const qreal valueCenterY = ( m_valuePosition == CenterValue
                                         ? center.y() + radius * 0.38
                                         : center.y() + radius * 0.72 )
                                   + titleOffset;
        const QRectF valueRect( center.x() - radius * 0.62,
                                valueCenterY - metrics.height() * 0.5,
                                radius * 1.24,
                                metrics.height() );
        if ( m_valueVisible )
        {
            QString valueText = QString::number( sliderPosition() );
            if ( !m_unit.isEmpty() )
            {
                valueText += QStringLiteral( " " ) + m_unit;
            }
            painter.drawText( valueRect, Qt::AlignCenter, valueText );
        }

        if ( !m_title.isEmpty() )
        {
            QFont titleFont = font();
            titleFont.setPixelSize( qMax( 8, qRound( valueFont.pixelSize() * 0.48 ) ) );
            titleFont.setWeight( QFont::DemiBold );
            painter.setFont( titleFont );
            const QFontMetricsF titleMetrics( titleFont );
            const QRectF titleRect( center.x() - radius * 0.62,
                                    valueRect.top() - titleMetrics.height() * 0.92,
                                    radius * 1.24,
                                    titleMetrics.height() );
            painter.drawText( titleRect, Qt::AlignCenter, m_title );
        }
    }
}

void ExRadialGauge::mousePressEvent( QMouseEvent* event )
{
    if ( !m_interactive || !isEnabled() || event->button() != Qt::LeftButton )
    {
        event->ignore();
        return;
    }

    m_valueAnimation->stop();
    setFocus( Qt::MouseFocusReason );
    setSliderDown( true );
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    updatePositionFromPoint( event->position() );
#else
    updatePositionFromPoint( event->localPos() );
#endif
    event->accept();
}

void ExRadialGauge::mouseMoveEvent( QMouseEvent* event )
{
    if ( !m_interactive || !isEnabled() || !isSliderDown() )
    {
        event->ignore();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    updatePositionFromPoint( event->position() );
#else
    updatePositionFromPoint( event->localPos() );
#endif
    event->accept();
}

void ExRadialGauge::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !m_interactive || !isEnabled() || event->button() != Qt::LeftButton || !isSliderDown() )
    {
        event->ignore();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    updatePositionFromPoint( event->position() );
#else
    updatePositionFromPoint( event->localPos() );
#endif
    if ( !hasTracking() )
    {
        setValue( sliderPosition() );
    }
    setSliderDown( false );
    event->accept();
}

void ExRadialGauge::wheelEvent( QWheelEvent* event )
{
    if ( !m_interactive )
    {
        event->ignore();
        return;
    }

    m_valueAnimation->stop();
    QDial::wheelEvent( event );
}

void ExRadialGauge::keyPressEvent( QKeyEvent* event )
{
    if ( !m_interactive )
    {
        event->ignore();
        return;
    }

    m_valueAnimation->stop();
    QDial::keyPressEvent( event );
}

void ExRadialGauge::setValue( int targetValue )
{
    targetValue = qBound( minimum(), targetValue, maximum() );
    if ( !m_valueAnimation || m_valueAnimationDuration == 0 || !isVisible() || isSliderDown() )
    {
        if ( m_valueAnimation )
        {
            m_valueAnimation->stop();
        }
        QDial::setValue( targetValue );
        return;
    }

    if ( value() == targetValue )
    {
        m_valueAnimation->stop();
        return;
    }

    m_valueAnimation->stop();
    m_valueAnimation->setDuration( m_valueAnimationDuration );
    m_valueAnimation->setStartValue( static_cast<qreal>( value() ) );
    m_valueAnimation->setEndValue( static_cast<qreal>( targetValue ) );
    m_valueAnimation->start();
}

bool ExRadialGauge::isValueAnimating() const
{
    return m_valueAnimation && m_valueAnimation->state() == QAbstractAnimation::Running;
}

qreal ExRadialGauge::sweepAngle() const
{
    qreal sweep = std::fmod( m_maximumAngle - m_minimumAngle, FullCircle );
    if ( sweep <= 0.0 )
    {
        sweep += FullCircle;
    }
    return sweep;
}

qreal ExRadialGauge::valueFraction( qreal value ) const
{
    const qreal range = static_cast<qreal>( maximum() ) - static_cast<qreal>( minimum() );
    if ( range <= 0.0 )
    {
        return 0.0;
    }

    qreal fraction = ( value - static_cast<qreal>( minimum() ) ) / range;
    fraction = qBound( 0.0, fraction, 1.0 );
    return invertedAppearance() ? 1.0 - fraction : fraction;
}

qreal ExRadialGauge::positionFraction() const
{
    return valueFraction( sliderPosition() );
}

int ExRadialGauge::positionFromPoint( const QPointF& point ) const
{
    const QPointF center = QRectF( rect() ).center();
    const qreal deltaX = point.x() - center.x();
    const qreal deltaY = point.y() - center.y();
    if ( qFuzzyIsNull( deltaX ) && qFuzzyIsNull( deltaY ) )
    {
        return sliderPosition();
    }

    qreal angle = qRadiansToDegrees( std::atan2( deltaX, -deltaY ) );
    while ( angle < m_minimumAngle )
    {
        angle += FullCircle;
    }
    while ( angle >= m_minimumAngle + FullCircle )
    {
        angle -= FullCircle;
    }

    const qreal sweep = sweepAngle();
    const qreal endAngle = m_minimumAngle + sweep;
    if ( qAbs( sweep - FullCircle ) < 0.0001 && qAbs( angle - m_minimumAngle ) < 0.0001
         && positionFraction() > 0.5 )
    {
        angle = endAngle;
    }

    if ( angle > endAngle )
    {
        const qreal minimumDistance = circularDistance( angle, m_minimumAngle );
        const qreal maximumDistance = circularDistance( angle, endAngle );
        if ( qAbs( minimumDistance - maximumDistance ) < 0.0001 )
        {
            angle = positionFraction() < 0.5 ? m_minimumAngle : endAngle;
        }
        else
        {
            angle = minimumDistance < maximumDistance ? m_minimumAngle : endAngle;
        }
    }

    qreal fraction = ( angle - m_minimumAngle ) / sweep;
    if ( invertedAppearance() )
    {
        fraction = 1.0 - fraction;
    }

    const qint64 range = static_cast<qint64>( maximum() ) - static_cast<qint64>( minimum() );
    const qint64 position = static_cast<qint64>( minimum() ) + qRound64( fraction * static_cast<qreal>( range ) );
    return static_cast<int>( qBound( static_cast<qint64>( minimum() ), position, static_cast<qint64>( maximum() ) ) );
}

void ExRadialGauge::updatePositionFromPoint( const QPointF& point )
{
    setSliderPosition( positionFromPoint( point ) );
}
