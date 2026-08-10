#include "exborderbeambutton.h"

#include <QApplication>
#include <QEvent>
#include <QHideEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOptionButton>
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

ExBorderBeamButton::ExBorderBeamButton( QWidget* parent )
    : QPushButton( parent )
{
    initialize();
}

ExBorderBeamButton::ExBorderBeamButton( const QString& text, QWidget* parent )
    : QPushButton( text, parent )
{
    initialize();
}

void ExBorderBeamButton::initialize()
{
    m_animationTimer = new QTimer( this );
    m_animationTimer->setInterval( AnimationFrameInterval );
    m_animationTimer->setTimerType( Qt::PreciseTimer );
    connect( m_animationTimer, &QTimer::timeout, this, [this]
             {
                 const qint64 elapsed = qMax<qint64>( qint64( 0 ), m_elapsed.restart() );
                 const qreal sign = m_direction == Clockwise ? 1.0 : -1.0;
                 m_progress = std::fmod(
                     m_progress + sign * static_cast<qreal>( elapsed ) / m_animationDuration,
                     1.0 );
                 if ( m_progress < 0.0 )
                 {
                     m_progress += 1.0;
                 }
                 update();
             } );
}

void ExBorderBeamButton::setBeamLength( qreal length )
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

void ExBorderBeamButton::setBeamWidth( qreal width )
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

void ExBorderBeamButton::setCornerRadius( qreal radius )
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

void ExBorderBeamButton::setBackgroundColor( QColor color )
{
    if ( m_backgroundColor == color )
    {
        return;
    }
    m_backgroundColor = color;
    update();
    emit backgroundColorChanged( color );
}

void ExBorderBeamButton::setBorderColor( QColor color )
{
    if ( m_borderColor == color )
    {
        return;
    }
    m_borderColor = color;
    update();
    emit borderColorChanged( color );
}

void ExBorderBeamButton::setStartColor( QColor color )
{
    if ( m_startColor == color )
    {
        return;
    }
    m_startColor = color;
    update();
    emit startColorChanged( color );
}

void ExBorderBeamButton::setEndColor( QColor color )
{
    if ( m_endColor == color )
    {
        return;
    }
    m_endColor = color;
    update();
    emit endColorChanged( color );
}

void ExBorderBeamButton::setAnimationDuration( int duration )
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

void ExBorderBeamButton::setInitialProgress( qreal progress )
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

void ExBorderBeamButton::setDirection( Direction direction )
{
    if ( direction < Clockwise || direction > CounterClockwise || m_direction == direction )
    {
        return;
    }
    m_direction = direction;
    update();
    emit directionChanged( direction );
}

void ExBorderBeamButton::setBeamCount( int count )
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

void ExBorderBeamButton::setAnimationEnabled( bool enabled )
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

void ExBorderBeamButton::setThemeMode( ThemeMode mode )
{
    if ( mode < AutoTheme || mode > DarkTheme || m_themeMode == mode )
    {
        return;
    }
    m_themeMode = mode;
    update();
    emit themeModeChanged( mode );
}

bool ExBorderBeamButton::isRunning() const
{
    return m_animationTimer->isActive();
}

QSize ExBorderBeamButton::sizeHint() const
{
    return QPushButton::sizeHint().expandedTo( QSize( 120, 40 ) );
}

void ExBorderBeamButton::restartAnimation()
{
    m_progress = m_initialProgress;
    if ( m_animationTimer->isActive() )
    {
        m_elapsed.restart();
    }
    update();
}

ExBorderBeamButton::ThemeConfig ExBorderBeamButton::defaultLightTheme()
{
    return ExBorderBeam::defaultLightTheme();
}

ExBorderBeamButton::ThemeConfig ExBorderBeamButton::defaultDarkTheme()
{
    return ExBorderBeam::defaultDarkTheme();
}

ExBorderBeamButton::ThemeConfig ExBorderBeamButton::lightTheme() const
{
    return m_lightTheme;
}

ExBorderBeamButton::ThemeConfig ExBorderBeamButton::darkTheme() const
{
    return m_darkTheme;
}

ExBorderBeamButton::ThemeConfig ExBorderBeamButton::activeTheme() const
{
    return usesDarkTheme() ? m_darkTheme : m_lightTheme;
}

void ExBorderBeamButton::setLightTheme( const ThemeConfig& config )
{
    if ( m_lightTheme == config )
    {
        return;
    }
    m_lightTheme = config;
    update();
    emit lightThemeChanged();
}

void ExBorderBeamButton::setDarkTheme( const ThemeConfig& config )
{
    if ( m_darkTheme == config )
    {
        return;
    }
    m_darkTheme = config;
    update();
    emit darkThemeChanged();
}

void ExBorderBeamButton::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    if ( m_pathDirty || m_pathSize != size() )
    {
        rebuildBorderPath();
    }

    QStyleOptionButton option;
    initStyleOption( &option );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );

    QColor background = resolvedBackgroundColor();
    if ( option.state.testFlag( QStyle::State_Sunken ) || option.state.testFlag( QStyle::State_On ) )
    {
        background = background.darker( 108 );
    }
    else if ( option.state.testFlag( QStyle::State_MouseOver ) )
    {
        background = background.lighter( 106 );
    }

    const QRectF borderRect = QRectF( rect() ).adjusted( 0.5, 0.5, -0.5, -0.5 );
    const qreal borderRadius = qMin( m_cornerRadius, qMin( borderRect.width(), borderRect.height() ) * 0.5 );
    painter.setPen( QPen( resolvedBorderColor(), 1.0 ) );
    painter.setBrush( background );
    painter.drawRoundedRect( borderRect, borderRadius, borderRadius );

    style()->drawControl( QStyle::CE_PushButtonLabel, &option, &painter, this );

    if ( m_beamLength <= 0.0 || m_beamWidth <= 0.0 || m_borderLength <= 0.0
         || m_borderPath.isEmpty() )
    {
        return;
    }

    QColor from = resolvedStartColor();
    QColor to = resolvedEndColor( from );
    if ( !isEnabled() )
    {
        from.setAlphaF( from.alphaF() * 0.46 );
        to.setAlphaF( to.alphaF() * 0.46 );
    }

    const qreal visibleLength = qMin( m_beamLength, m_borderLength * 0.95 );
    const int segmentCount = qBound( 12, qCeil( visibleLength / 2.5 ), 160 );
    for ( int beamIndex = 0; beamIndex < m_beamCount; ++beamIndex )
    {
        qreal beamProgress = m_progress + static_cast<qreal>( beamIndex ) / m_beamCount;
        beamProgress -= std::floor( beamProgress );
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
            color.setAlphaF( color.alphaF() * qMin( 1.0, secondProgress * 4.0 ) );
            painter.setPen( QPen( color, m_beamWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
            painter.drawLine( firstPoint, secondPoint );
        }
    }
}

void ExBorderBeamButton::showEvent( QShowEvent* event )
{
    QPushButton::showEvent( event );
    updateAnimationState();
}

void ExBorderBeamButton::hideEvent( QHideEvent* event )
{
    QPushButton::hideEvent( event );
    updateAnimationState();
}

void ExBorderBeamButton::changeEvent( QEvent* event )
{
    QPushButton::changeEvent( event );
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

void ExBorderBeamButton::rebuildBorderPath()
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

void ExBorderBeamButton::updateAnimationState()
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

QColor ExBorderBeamButton::resolvedStartColor() const
{
    if ( m_startColor.isValid() )
    {
        return m_startColor;
    }
    const QColor color = activeTheme().startColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : accentColor( palette(), group );
}

QColor ExBorderBeamButton::resolvedEndColor( const QColor& start ) const
{
    const QColor color = activeTheme().endColor;
    return m_endColor.isValid() ? m_endColor : ( color.isValid() ? color : start.lighter( 145 ) );
}

QColor ExBorderBeamButton::resolvedBackgroundColor() const
{
    if ( m_backgroundColor.isValid() )
    {
        return m_backgroundColor;
    }
    const QColor color = activeTheme().backgroundColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : palette().color( group, QPalette::Button );
}

QColor ExBorderBeamButton::resolvedBorderColor() const
{
    if ( m_borderColor.isValid() )
    {
        return m_borderColor;
    }
    const QColor color = activeTheme().borderColor;
    const QPalette::ColorGroup group = isEnabled() ? QPalette::Active : QPalette::Disabled;
    return color.isValid() ? color : palette().color( group, QPalette::Mid );
}

bool ExBorderBeamButton::usesDarkTheme() const
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
