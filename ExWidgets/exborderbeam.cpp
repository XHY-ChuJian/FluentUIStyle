#include "exborderbeam.h"

#include <QEvent>
#include <QApplication>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QShowEvent>
#include <QTimer>
#include <QtMath>

#include <cmath>

namespace
{

constexpr int AnimationFrameInterval = 16;

QColor accentColor( const QPalette& palette, QPalette::ColorGroup group )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return palette.color( group, QPalette::Accent );
#else
    return palette.color( group, QPalette::Highlight );
#endif
}

QColor mixedColor( const QColor& from, const QColor& to, qreal progress )
{
    progress = qBound( 0.0, progress, 1.0 );
    QColor result;
    result.setRgbF( from.redF() + ( to.redF() - from.redF() ) * progress,
                    from.greenF() + ( to.greenF() - from.greenF() ) * progress,
                    from.blueF() + ( to.blueF() - from.blueF() ) * progress,
                    from.alphaF() + ( to.alphaF() - from.alphaF() ) * progress );
    return result;
}

qreal wrappedDistance( qreal distance, qreal pathLength )
{
    distance = std::fmod( distance, pathLength );
    return distance < 0.0 ? distance + pathLength : distance;
}

} // namespace

ExBorderBeam::ExBorderBeam( QWidget* parent )
    : QFrame( parent )
{
    setFrameShape( QFrame::NoFrame );
    setAutoFillBackground( false );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

    m_animationTimer = new QTimer( this );
    m_animationTimer->setInterval( AnimationFrameInterval );
    m_animationTimer->setTimerType( Qt::PreciseTimer );
    connect( m_animationTimer, &QTimer::timeout, this, [this]
             {
                 const qint64 elapsed = qMax<qint64>( qint64( 0 ), m_elapsed.restart() );
                 const qreal direction = m_direction == Clockwise ? 1.0 : -1.0;
                 const qreal step = direction * static_cast<qreal>( elapsed ) / m_animationDuration;
                 m_progress = std::fmod( m_progress + step, 1.0 );
                 if ( m_progress < 0.0 )
                 {
                     m_progress += 1.0;
                 }
                 update();
             } );
}

void ExBorderBeam::setBeamLength( qreal length )
{
    if ( !qIsFinite( length ) )
    {
        return;
    }
    length = qBound( 0.0, length, 10000.0 );
    if ( qFuzzyCompare( m_beamLength + 1.0, length + 1.0 ) )
    {
        return;
    }
    m_beamLength = length;
    updateAnimationState();
    update();
    emit beamLengthChanged( length );
}

void ExBorderBeam::setBeamWidth( qreal width )
{
    if ( !qIsFinite( width ) )
    {
        return;
    }
    width = qBound( 0.5, width, 32.0 );
    if ( qFuzzyCompare( m_beamWidth, width ) )
    {
        return;
    }
    m_beamWidth = width;
    m_pathDirty = true;
    update();
    emit beamWidthChanged( width );
}

void ExBorderBeam::setCornerRadius( qreal radius )
{
    if ( !qIsFinite( radius ) )
    {
        return;
    }
    radius = qBound( 0.0, radius, 1000.0 );
    if ( qFuzzyCompare( m_cornerRadius + 1.0, radius + 1.0 ) )
    {
        return;
    }
    m_cornerRadius = radius;
    m_pathDirty = true;
    update();
    emit cornerRadiusChanged( radius );
}

void ExBorderBeam::setBackgroundColor( QColor color )
{
    if ( m_backgroundColor == color )
    {
        return;
    }
    m_backgroundColor = color;
    update();
    emit backgroundColorChanged( color );
}

void ExBorderBeam::setBorderColor( QColor color )
{
    if ( m_borderColor == color )
    {
        return;
    }
    m_borderColor = color;
    update();
    emit borderColorChanged( color );
}

void ExBorderBeam::setStartColor( QColor color )
{
    if ( m_startColor == color )
    {
        return;
    }
    m_startColor = color;
    update();
    emit startColorChanged( color );
}

void ExBorderBeam::setEndColor( QColor color )
{
    if ( m_endColor == color )
    {
        return;
    }
    m_endColor = color;
    update();
    emit endColorChanged( color );
}

void ExBorderBeam::setAnimationDuration( int duration )
{
    duration = qBound( 100, duration, 600000 );
    if ( m_animationDuration == duration )
    {
        return;
    }
    m_animationDuration = duration;
    if ( m_animationTimer->isActive() )
    {
        m_elapsed.restart();
    }
    emit animationDurationChanged( duration );
}

void ExBorderBeam::setInitialProgress( qreal progress )
{
    if ( !qIsFinite( progress ) )
    {
        return;
    }
    progress = qBound( 0.0, progress, 1.0 );
    if ( qFuzzyCompare( m_initialProgress + 1.0, progress + 1.0 ) )
    {
        return;
    }
    m_initialProgress = progress;
    m_progress = progress;
    if ( m_animationTimer->isActive() )
    {
        m_elapsed.restart();
    }
    update();
    emit initialProgressChanged( progress );
}

void ExBorderBeam::setDirection( Direction direction )
{
    if ( direction < Clockwise || direction > CounterClockwise || m_direction == direction )
    {
        return;
    }
    m_direction = direction;
    update();
    emit directionChanged( direction );
}

void ExBorderBeam::setBeamCount( int count )
{
    count = qBound( 1, count, 8 );
    if ( m_beamCount == count )
    {
        return;
    }
    m_beamCount = count;
    update();
    emit beamCountChanged( count );
}

void ExBorderBeam::setAnimationEnabled( bool enabled )
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

void ExBorderBeam::setThemeMode( ThemeMode mode )
{
    if ( mode < AutoTheme || mode > DarkTheme || m_themeMode == mode )
    {
        return;
    }
    m_themeMode = mode;
    update();
    emit themeModeChanged( mode );
}

bool ExBorderBeam::isRunning() const
{
    return m_animationTimer->isActive();
}

QSize ExBorderBeam::sizeHint() const
{
    return QSize( 320, 180 );
}

QSize ExBorderBeam::minimumSizeHint() const
{
    return QSize( 40, 40 );
}

void ExBorderBeam::restartAnimation()
{
    m_progress = m_initialProgress;
    if ( m_animationTimer->isActive() )
    {
        m_elapsed.restart();
    }
    update();
}

ExBorderBeam::ThemeConfig ExBorderBeam::defaultLightTheme()
{
    return {QColor( QStringLiteral( "#F9F9F9" ) ),
            QColor( QStringLiteral( "#E5E5E5" ) ),
            QColor( QStringLiteral( "#005FB8" ) ),
            QColor( QStringLiteral( "#60CDFF" ) )};
}

ExBorderBeam::ThemeConfig ExBorderBeam::defaultDarkTheme()
{
    return {QColor( QStringLiteral( "#272727" ) ),
            QColor( QStringLiteral( "#454545" ) ),
            QColor( QStringLiteral( "#60CDFF" ) ),
            QColor( QStringLiteral( "#A5E5FF" ) )};
}

ExBorderBeam::ThemeConfig ExBorderBeam::lightTheme() const
{
    return m_lightTheme;
}

ExBorderBeam::ThemeConfig ExBorderBeam::darkTheme() const
{
    return m_darkTheme;
}

ExBorderBeam::ThemeConfig ExBorderBeam::activeTheme() const
{
    return usesDarkTheme() ? m_darkTheme : m_lightTheme;
}

void ExBorderBeam::setLightTheme( const ThemeConfig& config )
{
    if ( m_lightTheme == config )
    {
        return;
    }
    m_lightTheme = config;
    update();
    emit lightThemeChanged();
}

void ExBorderBeam::setDarkTheme( const ThemeConfig& config )
{
    if ( m_darkTheme == config )
    {
        return;
    }
    m_darkTheme = config;
    update();
    emit darkThemeChanged();
}

void ExBorderBeam::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );

    const QRectF borderRect = QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
    const qreal borderRadius = qMin( m_cornerRadius, qMin( borderRect.width(), borderRect.height() ) * 0.5 );
    painter.setPen( QPen( resolvedBorderColor(), 1.0 ) );
    painter.setBrush( resolvedBackgroundColor() );
    painter.drawRoundedRect( borderRect, borderRadius, borderRadius );

    if ( m_beamLength <= 0.0 || m_beamWidth <= 0.0 )
    {
        return;
    }
    if ( m_pathDirty || m_pathSize != size() )
    {
        rebuildBorderPath();
    }
    if ( m_borderLength <= 0.0 || m_borderPath.isEmpty() )
    {
        return;
    }

    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    QColor from = resolvedStartColor();
    QColor to = resolvedEndColor( from );
    if ( group == QPalette::Disabled )
    {
        from.setAlphaF( from.alphaF() * 0.46 );
        to.setAlphaF( to.alphaF() * 0.46 );
    }

    const qreal visibleLength = qMin( m_beamLength, m_borderLength * 0.95 );
    const int segmentCount = qBound( 12, qCeil( visibleLength / 2.5 ), 160 );

    for ( int beamIndex = 0; beamIndex < m_beamCount; ++beamIndex )
    {
        qreal beamProgress = m_progress + static_cast<qreal>( beamIndex ) / m_beamCount;
        beamProgress = beamProgress - std::floor( beamProgress );
        const qreal headDistance = beamProgress * m_borderLength;

        for ( int segment = 0; segment < segmentCount; ++segment )
        {
            const qreal firstProgress = static_cast<qreal>( segment ) / segmentCount;
            const qreal secondProgress = static_cast<qreal>( segment + 1 ) / segmentCount;
            const qreal firstDistance = m_direction == Clockwise
                                            ? headDistance - visibleLength + visibleLength * firstProgress
                                            : headDistance + visibleLength - visibleLength * firstProgress;
            const qreal secondDistance = m_direction == Clockwise
                                             ? headDistance - visibleLength + visibleLength * secondProgress
                                             : headDistance + visibleLength - visibleLength * secondProgress;
            const QPointF firstPoint = m_borderPath.pointAtPercent(
                m_borderPath.percentAtLength( wrappedDistance( firstDistance, m_borderLength ) ) );
            const QPointF secondPoint = m_borderPath.pointAtPercent(
                m_borderPath.percentAtLength( wrappedDistance( secondDistance, m_borderLength ) ) );

            QColor color = mixedColor( from, to, secondProgress );
            const qreal fade = qMin( 1.0, secondProgress * 4.0 );
            color.setAlphaF( color.alphaF() * fade );

            painter.setPen( QPen( color, m_beamWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter.drawLine( firstPoint, secondPoint );
        }
    }
}

void ExBorderBeam::showEvent( QShowEvent* event )
{
    QFrame::showEvent( event );
    updateAnimationState();
}

void ExBorderBeam::hideEvent( QHideEvent* event )
{
    QFrame::hideEvent( event );
    updateAnimationState();
}

void ExBorderBeam::changeEvent( QEvent* event )
{
    QFrame::changeEvent( event );
    if ( event->type() == QEvent::EnabledChange )
    {
        updateAnimationState();
    }
    if ( event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange
         || event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::StyleChange )
    {
        update();
    }
}

void ExBorderBeam::rebuildBorderPath()
{
    m_borderPath = QPainterPath();
    m_borderLength = 0.0;
    m_pathSize = size();
    m_pathDirty = false;

    const qreal inset = m_beamWidth * 0.5 + 0.5;
    const QRectF bounds = QRectF( rect() ).adjusted( inset, inset, -inset, -inset );
    if ( bounds.width() <= 0.0 || bounds.height() <= 0.0 )
    {
        return;
    }

    const qreal maximumRadius = qMin( bounds.width(), bounds.height() ) * 0.5;
    const qreal radius = qBound( 0.0, m_cornerRadius - inset, maximumRadius );
    m_borderPath.addRoundedRect( bounds, radius, radius );
    m_borderLength = m_borderPath.length();
}

void ExBorderBeam::updateAnimationState()
{
    const bool wasRunning = m_animationTimer->isActive();
    const bool shouldRun = m_animationEnabled && m_beamLength > 0.0 && isVisible() && isEnabled();
    if ( shouldRun && !wasRunning )
    {
        m_elapsed.start();
        m_animationTimer->start();
    }
    else if ( !shouldRun && wasRunning )
    {
        m_animationTimer->stop();
    }

    if ( wasRunning != m_animationTimer->isActive() )
    {
        emit runningChanged( m_animationTimer->isActive() );
    }
}

QColor ExBorderBeam::resolvedStartColor() const
{
    if ( m_startColor.isValid() )
    {
        return m_startColor;
    }
    const QColor color = activeTheme().startColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : accentColor( palette(), group );
}

QColor ExBorderBeam::resolvedEndColor( const QColor& start ) const
{
    const QColor color = activeTheme().endColor;
    return m_endColor.isValid() ? m_endColor : ( color.isValid() ? color : start.lighter( 145 ) );
}

QColor ExBorderBeam::resolvedBackgroundColor() const
{
    if ( m_backgroundColor.isValid() )
    {
        return m_backgroundColor;
    }
    const QColor color = activeTheme().backgroundColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : palette().color( group, QPalette::Window );
}

QColor ExBorderBeam::resolvedBorderColor() const
{
    if ( m_borderColor.isValid() )
    {
        return m_borderColor;
    }
    const QColor color = activeTheme().borderColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : palette().color( group, QPalette::Mid );
}

bool ExBorderBeam::usesDarkTheme() const
{
    if ( m_themeMode != AutoTheme )
    {
        return m_themeMode == DarkTheme;
    }

    const QVariant colorScheme = qApp ? qApp->property( "_q_colorscheme" ) : QVariant();
    if ( colorScheme.isValid() )
    {
        return colorScheme.toInt() == 1;
    }
    return palette().color( QPalette::Window ).lightness() < 128;
}
