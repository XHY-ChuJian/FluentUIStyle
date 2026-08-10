#include "exliquidgauge.h"

#include <QEvent>
#include <QHideEvent>
#include <QPainter>
#include <QPolygonF>
#include <QShowEvent>
#include <QTimer>
#include <QtMath>

#include <cmath>

namespace
{

constexpr qreal Pi = 3.14159265358979323846;
constexpr int AnimationFrameInterval = 16;

QColor accentColor( const QPalette& palette, QPalette::ColorGroup group )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return palette.color( group, QPalette::Accent );
#else
    return palette.color( group, QPalette::Highlight );
#endif
}

} // namespace

ExLiquidGauge::ExLiquidGauge( QWidget* parent )
    : QProgressBar( parent )
{
    setAlignment( Qt::AlignCenter );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    setAutoFillBackground( false );

    m_waveTimer = new QTimer( this );
    m_waveTimer->setInterval( AnimationFrameInterval );
    m_waveTimer->setTimerType( Qt::PreciseTimer );
    connect( m_waveTimer, &QTimer::timeout, this, [this]
             {
                 const qint64 elapsed = qMax<qint64>( 0, m_waveElapsed.restart() );
                 const qreal phaseStep = static_cast<qreal>( elapsed ) / m_waveAnimationDuration;
                 m_wavePhase = std::fmod( m_wavePhase + phaseStep, 1.0 );
                 update();
             } );
}

#define EX_LIQUID_GAUGE_COLOR_SETTER( Property, Setter ) \
    void ExLiquidGauge::Setter( QColor color ) \
    { \
        if ( m_##Property == color ) \
        { \
            return; \
        } \
        m_##Property = color; \
        update(); \
        emit Property##Changed( color ); \
    }

EX_LIQUID_GAUGE_COLOR_SETTER( waveColor, setWaveColor )
EX_LIQUID_GAUGE_COLOR_SETTER( backgroundColor, setBackgroundColor )
EX_LIQUID_GAUGE_COLOR_SETTER( outlineColor, setOutlineColor )
EX_LIQUID_GAUGE_COLOR_SETTER( textColor, setTextColor )
EX_LIQUID_GAUGE_COLOR_SETTER( submergedTextColor, setSubmergedTextColor )

#undef EX_LIQUID_GAUGE_COLOR_SETTER

void ExLiquidGauge::setShape( Shape shape )
{
    if ( shape < CircleShape || shape > TriangleShape || m_shape == shape )
    {
        return;
    }

    m_shape = shape;
    update();
    emit shapeChanged( shape );
}

void ExLiquidGauge::setWaveAmplitude( qreal amplitude )
{
    if ( !qIsFinite( amplitude ) )
    {
        return;
    }

    amplitude = qBound( 0.0, amplitude, 100.0 );
    if ( qFuzzyCompare( m_waveAmplitude, amplitude ) )
    {
        return;
    }

    m_waveAmplitude = amplitude;
    updateAnimationState();
    update();
    emit waveAmplitudeChanged( amplitude );
}

void ExLiquidGauge::setWaveCount( int count )
{
    count = qBound( 1, count, 20 );
    if ( m_waveCount == count )
    {
        return;
    }

    m_waveCount = count;
    update();
    emit waveCountChanged( count );
}

void ExLiquidGauge::setWaveAnimationDuration( int duration )
{
    duration = qBound( 100, duration, 60000 );
    if ( m_waveAnimationDuration == duration )
    {
        return;
    }

    m_waveAnimationDuration = duration;
    updateAnimationState();
    emit waveAnimationDurationChanged( duration );
}

void ExLiquidGauge::setAnimationEnabled( bool enabled )
{
    if ( m_animationEnabled == enabled )
    {
        return;
    }

    m_animationEnabled = enabled;
    updateAnimationState();
    update();
    emit animationEnabledChanged( enabled );
}

void ExLiquidGauge::setSecondaryWaveOpacity( qreal opacity )
{
    if ( !qIsFinite( opacity ) )
    {
        return;
    }

    opacity = qBound( 0.0, opacity, 1.0 );
    if ( qFuzzyCompare( m_secondaryWaveOpacity, opacity ) )
    {
        return;
    }

    m_secondaryWaveOpacity = opacity;
    update();
    emit secondaryWaveOpacityChanged( opacity );
}

void ExLiquidGauge::setOutlineWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }

    width = qBound( 0.0, width, 100.0 );
    if ( qFuzzyCompare( m_outlineWidth, width ) )
    {
        return;
    }

    m_outlineWidth = width;
    updateGeometry();
    update();
    emit outlineWidthChanged( width );
}

void ExLiquidGauge::setOutlineDistance( qreal distance )
{
    if ( !qIsFinite( distance ) )
    {
        return;
    }

    distance = qBound( 0.0, distance, 100.0 );
    if ( qFuzzyCompare( m_outlineDistance, distance ) )
    {
        return;
    }

    m_outlineDistance = distance;
    updateGeometry();
    update();
    emit outlineDistanceChanged( distance );
}

void ExLiquidGauge::setContentFontPixelSize( int size )
{
    size = qBound( 0, size, 200 );
    if ( m_contentFontPixelSize == size )
    {
        return;
    }

    m_contentFontPixelSize = size;
    update();
    emit contentFontPixelSizeChanged( size );
}

QSize ExLiquidGauge::sizeHint() const
{
    return QSize( 180, 180 );
}

QSize ExLiquidGauge::minimumSizeHint() const
{
    return QSize( 56, 56 );
}

void ExLiquidGauge::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.setRenderHint( QPainter::TextAntialiasing, true );

    const qreal side = qMin( width(), height() );
    if ( side <= 2.0 )
    {
        return;
    }

    const QPointF center = QRectF( rect() ).center();
    const qreal outerInset = qMax( 1.0, m_outlineWidth * 0.5 + 1.0 );
    QRectF outerBounds( center.x() - side * 0.5 + outerInset,
                        center.y() - side * 0.5 + outerInset,
                        side - outerInset * 2.0,
                        side - outerInset * 2.0 );
    if ( outerBounds.width() <= 0.0 || outerBounds.height() <= 0.0 )
    {
        return;
    }

    const qreal innerInset = m_outlineWidth * 0.5 + m_outlineDistance;
    const QRectF innerBounds = outerBounds.adjusted( innerInset, innerInset, -innerInset, -innerInset );
    if ( innerBounds.width() <= 1.0 || innerBounds.height() <= 1.0 )
    {
        return;
    }

    const QPainterPath outerShape = shapePath( outerBounds );
    const QPainterPath innerShape = shapePath( innerBounds );
    const QColor wave = resolvedWaveColor();
    const QColor background = resolvedColor( m_backgroundColor, QPalette::Base );
    const QColor outline = m_outlineColor.isValid()
                               ? resolvedColor( m_outlineColor, QPalette::Mid )
                               : wave;

    painter.fillPath( innerShape, background );

    const qreal fraction = valueFraction();
    const qreal baseline = innerBounds.bottom() - fraction * innerBounds.height();
    const qreal edgeAttenuation = std::sin( fraction * Pi );
    const qreal amplitude = qMin( m_waveAmplitude, innerBounds.height() * 0.14 ) * edgeAttenuation;
    const qreal phase = m_wavePhase * 2.0 * Pi;
    const QPainterPath rearWave = wavePath( innerBounds,
                                            baseline + amplitude * 0.18,
                                            amplitude * 0.82,
                                            -phase * 0.78 + Pi * 0.55 );
    const QPainterPath frontWave = wavePath( innerBounds, baseline, amplitude, phase );

    painter.save();
    painter.setClipPath( innerShape );
    QColor rearColor = wave;
    rearColor.setAlphaF( rearColor.alphaF() * m_secondaryWaveOpacity );
    painter.fillPath( rearWave, rearColor );
    painter.fillPath( frontWave, wave );
    painter.restore();

    if ( m_outlineWidth > 0.0 )
    {
        painter.setBrush( Qt::NoBrush );
        painter.setPen( QPen( outline, m_outlineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter.drawPath( outerShape );
    }

    if ( isTextVisible() && !text().isEmpty() )
    {
        QFont contentFont = font();
        const int pixelSize = m_contentFontPixelSize > 0
                                  ? m_contentFontPixelSize
                                  : qMax( 10, qRound( side * ( m_shape == PinShape ? 0.14 : 0.18 ) ) );
        contentFont.setPixelSize( pixelSize );
        contentFont.setWeight( QFont::DemiBold );
        painter.setFont( contentFont );

        const QColor dryText = resolvedColor( m_textColor, QPalette::Text );
        const QColor wetText = resolvedColor( m_submergedTextColor, QPalette::HighlightedText );
        const QRectF textBounds = innerBounds.adjusted( 4.0, 4.0, -4.0, -4.0 );
        const int flags = static_cast<int>( alignment() ) | Qt::TextSingleLine;

        painter.save();
        painter.setClipPath( innerShape );
        painter.setPen( dryText );
        painter.drawText( textBounds, flags, text() );
        painter.restore();

        const auto drawSubmergedText = [&]( const QPainterPath& waveClip )
        {
            painter.save();
            painter.setClipPath( innerShape );
            painter.setClipPath( waveClip, Qt::IntersectClip );
            painter.setPen( wetText );
            painter.drawText( textBounds, flags, text() );
            painter.restore();
        };
        if ( m_secondaryWaveOpacity > 0.0 )
        {
            drawSubmergedText( rearWave );
        }
        drawSubmergedText( frontWave );
    }
}

void ExLiquidGauge::showEvent( QShowEvent* event )
{
    QProgressBar::showEvent( event );
    updateAnimationState();
}

void ExLiquidGauge::hideEvent( QHideEvent* event )
{
    QProgressBar::hideEvent( event );
    updateAnimationState();
}

void ExLiquidGauge::changeEvent( QEvent* event )
{
    QProgressBar::changeEvent( event );
    if ( event->type() == QEvent::EnabledChange )
    {
        updateAnimationState();
    }
    if ( event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange
         || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange )
    {
        update();
    }
}

qreal ExLiquidGauge::valueFraction() const
{
    const qreal range = static_cast<qreal>( maximum() ) - static_cast<qreal>( minimum() );
    if ( range <= 0.0 || value() < minimum() )
    {
        return 0.0;
    }

    return qBound( 0.0, ( static_cast<qreal>( value() ) - minimum() ) / range, 1.0 );
}

QPainterPath ExLiquidGauge::shapePath( const QRectF& bounds ) const
{
    QPainterPath path;
    switch ( m_shape )
    {
    case RectShape:
        path.addRect( bounds );
        break;
    case PinShape:
    {
        const qreal centerX = bounds.center().x();
        const qreal top = bounds.top();
        const qreal bottom = bounds.bottom();
        const qreal left = bounds.left() + bounds.width() * 0.08;
        const qreal right = bounds.right() - bounds.width() * 0.08;
        const qreal height = bounds.height();
        const qreal width = bounds.width();
        path.moveTo( centerX, bottom );
        path.cubicTo( centerX - width * 0.08,
                      bottom - height * 0.16,
                      left,
                      top + height * 0.58,
                      left,
                      top + height * 0.37 );
        path.cubicTo( left,
                      top + height * 0.16,
                      centerX - width * 0.2,
                      top,
                      centerX,
                      top );
        path.cubicTo( centerX + width * 0.2,
                      top,
                      right,
                      top + height * 0.16,
                      right,
                      top + height * 0.37 );
        path.cubicTo( right,
                      top + height * 0.58,
                      centerX + width * 0.08,
                      bottom - height * 0.16,
                      centerX,
                      bottom );
        path.closeSubpath();
        break;
    }
    case TriangleShape:
        path.addPolygon( QPolygonF{QVector<QPointF>{QPointF( bounds.center().x(), bounds.top() ),
                                   QPointF( bounds.right(), bounds.bottom() ),
                                   QPointF( bounds.left(), bounds.bottom() )} });
        path.closeSubpath();
        break;
    case CircleShape:
    default:
        path.addEllipse( bounds );
        break;
    }
    return path;
}

QPainterPath ExLiquidGauge::wavePath( const QRectF& bounds,
                                      qreal baseline,
                                      qreal amplitude,
                                      qreal phase ) const
{
    const qreal wavelength = bounds.width() / qMax( 1, m_waveCount );
    const qreal extendedLeft = bounds.left() - wavelength;
    const qreal extendedRight = bounds.right() + wavelength;
    const int sampleCount = qMax( 64, qCeil( ( extendedRight - extendedLeft ) / 2.0 ) );

    QPainterPath path;
    path.moveTo( extendedLeft, bounds.bottom() + amplitude + 2.0 );
    for ( int index = 0; index <= sampleCount; ++index )
    {
        const qreal progress = static_cast<qreal>( index ) / sampleCount;
        const qreal x = extendedLeft + progress * ( extendedRight - extendedLeft );
        const qreal y = baseline + amplitude * std::sin( ( x - bounds.left() ) / wavelength * 2.0 * Pi + phase );
        path.lineTo( x, y );
    }
    path.lineTo( extendedRight, bounds.bottom() + amplitude + 2.0 );
    path.closeSubpath();
    return path;
}

QColor ExLiquidGauge::resolvedWaveColor() const
{
    if ( m_waveColor.isValid() )
    {
        return resolvedColor( m_waveColor, QPalette::Highlight );
    }

    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return accentColor( palette(), group );
}

QColor ExLiquidGauge::resolvedColor( const QColor& configuredColor,
                                     QPalette::ColorRole fallbackRole ) const
{
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    QColor color = configuredColor.isValid() ? configuredColor : palette().color( group, fallbackRole );
    if ( !isEnabled() && configuredColor.isValid() )
    {
        color.setAlphaF( color.alphaF() * 0.46 );
    }
    return color;
}

void ExLiquidGauge::updateAnimationState()
{
    const bool shouldRun = m_animationEnabled && m_waveAmplitude > 0.0 && isVisible() && isEnabled();
    if ( shouldRun )
    {
        if ( !m_waveTimer->isActive() )
        {
            m_waveElapsed.start();
            m_waveTimer->start();
        }
    }
    else
    {
        m_waveTimer->stop();
    }
}
