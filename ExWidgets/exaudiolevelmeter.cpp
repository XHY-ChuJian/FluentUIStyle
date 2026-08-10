#include "exaudiolevelmeter.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QHideEvent>
#include <QMetaObject>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QShowEvent>
#include <QThread>
#include <QTimer>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace
{

constexpr int AnimationInterval = 16;

struct MeterLayout
{
    QVector<QRectF> channels;
    QRectF scale;
    QRectF content;
};

QColor withDisabledAlpha( QColor color, bool enabled )
{
    if ( !enabled )
    {
        color.setAlphaF( color.alphaF() * 0.45 );
    }
    return color;
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

bool usesDarkTheme( const QWidget* widget )
{
    const QVariant scheme = qApp ? qApp->property( "_q_colorscheme" ) : QVariant();
    return scheme.isValid() ? scheme.toInt() == 1
                            : widget->palette().color( QPalette::Window ).lightness() < 128;
}

QColor themedColor( const QWidget* widget,
                    const QColor& customColor,
                    const char* lightColor,
                    const char* darkColor )
{
    return customColor.isValid()
               ? customColor
               : QColor( QLatin1String( usesDarkTheme( widget ) ? darkColor : lightColor ) );
}

} // namespace

ExAudioLevelMeter::ExAudioLevelMeter( QWidget* parent )
    : QWidget( parent )
{
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );

    resizeLevelStorage();
    m_animationTimer = new QTimer( this );
    m_animationTimer->setInterval( AnimationInterval );
    m_animationTimer->setTimerType( Qt::PreciseTimer );
    connect( m_animationTimer, &QTimer::timeout, this, &ExAudioLevelMeter::updateAnimationFrame );
}

void ExAudioLevelMeter::setChannelCount( int count )
{
    count = qBound( 1, count, 8 );
    if ( m_channelCount == count )
    {
        return;
    }
    m_channelCount = count;
    resizeLevelStorage();
    updateGeometry();
    update();
    emit channelCountChanged( count );
}

void ExAudioLevelMeter::setMinimumDecibels( qreal decibels )
{
    if ( !qIsFinite( decibels ) )
    {
        return;
    }
    decibels = qBound( -160.0, decibels, m_maximumDecibels - 1.0 );
    if ( qFuzzyCompare( m_minimumDecibels, decibels ) )
    {
        return;
    }
    m_minimumDecibels = decibels;
    const qreal oldWarning = m_warningDecibels;
    const qreal oldClip = m_clipDecibels;
    m_warningDecibels = qBound( m_minimumDecibels, m_warningDecibels, m_maximumDecibels );
    m_clipDecibels = qBound( m_warningDecibels, m_clipDecibels, m_maximumDecibels );
    resizeLevelStorage();
    updateGeometry();
    update();
    emit minimumDecibelsChanged( decibels );
    if ( !qFuzzyCompare( oldWarning + 1.0, m_warningDecibels + 1.0 ) )
    {
        emit warningDecibelsChanged( m_warningDecibels );
    }
    if ( !qFuzzyCompare( oldClip + 1.0, m_clipDecibels + 1.0 ) )
    {
        emit clipDecibelsChanged( m_clipDecibels );
    }
}

void ExAudioLevelMeter::setMaximumDecibels( qreal decibels )
{
    if ( !qIsFinite( decibels ) )
    {
        return;
    }
    decibels = qBound( m_minimumDecibels + 1.0, decibels, 24.0 );
    if ( qFuzzyCompare( m_maximumDecibels + 1.0, decibels + 1.0 ) )
    {
        return;
    }
    m_maximumDecibels = decibels;
    const qreal oldWarning = m_warningDecibels;
    const qreal oldClip = m_clipDecibels;
    m_warningDecibels = qBound( m_minimumDecibels, m_warningDecibels, m_maximumDecibels );
    m_clipDecibels = qBound( m_warningDecibels, m_clipDecibels, m_maximumDecibels );
    resizeLevelStorage();
    updateGeometry();
    update();
    emit maximumDecibelsChanged( decibels );
    if ( !qFuzzyCompare( oldWarning + 1.0, m_warningDecibels + 1.0 ) )
    {
        emit warningDecibelsChanged( m_warningDecibels );
    }
    if ( !qFuzzyCompare( oldClip + 1.0, m_clipDecibels + 1.0 ) )
    {
        emit clipDecibelsChanged( m_clipDecibels );
    }
}

void ExAudioLevelMeter::setWarningDecibels( qreal decibels )
{
    if ( !qIsFinite( decibels ) )
    {
        return;
    }
    decibels = qBound( m_minimumDecibels, decibels, m_clipDecibels );
    if ( qFuzzyCompare( m_warningDecibels + 1.0, decibels + 1.0 ) )
    {
        return;
    }
    m_warningDecibels = decibels;
    update();
    emit warningDecibelsChanged( decibels );
}

void ExAudioLevelMeter::setClipDecibels( qreal decibels )
{
    if ( !qIsFinite( decibels ) )
    {
        return;
    }
    decibels = qBound( m_warningDecibels, decibels, m_maximumDecibels );
    if ( qFuzzyCompare( m_clipDecibels + 1.0, decibels + 1.0 ) )
    {
        return;
    }
    m_clipDecibels = decibels;
    update();
    emit clipDecibelsChanged( decibels );
}

void ExAudioLevelMeter::setSegmentCount( int count )
{
    count = qBound( 2, count, 120 );
    if ( m_segmentCount == count )
    {
        return;
    }
    m_segmentCount = count;
    update();
    emit segmentCountChanged( count );
}

void ExAudioLevelMeter::setSegmentSpacing( qreal spacing )
{
    if ( !qIsFinite( spacing ) )
    {
        return;
    }
    spacing = qBound( 0.0, spacing, 20.0 );
    if ( qFuzzyCompare( m_segmentSpacing + 1.0, spacing + 1.0 ) )
    {
        return;
    }
    m_segmentSpacing = spacing;
    update();
    emit segmentSpacingChanged( spacing );
}

void ExAudioLevelMeter::setSegmentRadius( qreal radius )
{
    if ( !qIsFinite( radius ) )
    {
        return;
    }
    radius = qBound( 0.0, radius, 20.0 );
    if ( qFuzzyCompare( m_segmentRadius + 1.0, radius + 1.0 ) )
    {
        return;
    }
    m_segmentRadius = radius;
    update();
    emit segmentRadiusChanged( radius );
}

void ExAudioLevelMeter::setChannelSpacing( qreal spacing )
{
    if ( !qIsFinite( spacing ) )
    {
        return;
    }
    spacing = qBound( 0.0, spacing, 40.0 );
    if ( qFuzzyCompare( m_channelSpacing + 1.0, spacing + 1.0 ) )
    {
        return;
    }
    m_channelSpacing = spacing;
    update();
    emit channelSpacingChanged( spacing );
}

void ExAudioLevelMeter::setScalePosition( ScalePosition position )
{
    if ( position < NoScale || position > CenterScale || m_scalePosition == position )
    {
        return;
    }
    m_scalePosition = position;
    updateGeometry();
    update();
    emit scalePositionChanged( position );
}

void ExAudioLevelMeter::setScaleMode( ScaleMode mode )
{
    if ( mode < IntervalScale || mode > CustomScale || m_scaleMode == mode )
    {
        return;
    }
    m_scaleMode = mode;
    updateGeometry();
    update();
    emit scaleModeChanged( mode );
}

void ExAudioLevelMeter::setScaleInterval( qreal interval )
{
    if ( !qIsFinite( interval ) )
    {
        return;
    }
    interval = qBound( 1.0, interval, 60.0 );
    if ( qFuzzyCompare( m_scaleInterval, interval ) )
    {
        return;
    }
    m_scaleInterval = interval;
    updateGeometry();
    update();
    emit scaleIntervalChanged( interval );
}

void ExAudioLevelMeter::setScaleTickCount( int count )
{
    count = qBound( 2, count, 64 );
    if ( m_scaleTickCount == count )
    {
        return;
    }
    m_scaleTickCount = count;
    updateGeometry();
    update();
    emit scaleTickCountChanged( count );
}

void ExAudioLevelMeter::setScaleUnit( QString unit )
{
    if ( m_scaleUnit == unit )
    {
        return;
    }
    m_scaleUnit = std::move( unit );
    updateGeometry();
    update();
    emit scaleUnitChanged( m_scaleUnit );
}

void ExAudioLevelMeter::setScaleUnitVisible( bool visible )
{
    if ( m_scaleUnitVisible == visible )
    {
        return;
    }
    m_scaleUnitVisible = visible;
    updateGeometry();
    update();
    emit scaleUnitVisibleChanged( visible );
}

void ExAudioLevelMeter::setScalePrecision( int precision )
{
    precision = qBound( 0, precision, 3 );
    if ( m_scalePrecision == precision )
    {
        return;
    }
    m_scalePrecision = precision;
    updateGeometry();
    update();
    emit scalePrecisionChanged( precision );
}

void ExAudioLevelMeter::setScaleTickMarksVisible( bool visible )
{
    if ( m_scaleTickMarksVisible == visible )
    {
        return;
    }
    m_scaleTickMarksVisible = visible;
    updateGeometry();
    update();
    emit scaleTickMarksVisibleChanged( visible );
}

void ExAudioLevelMeter::setScaleTickLength( qreal length )
{
    if ( !qIsFinite( length ) )
    {
        return;
    }
    length = qBound( 1.0, length, 20.0 );
    if ( qFuzzyCompare( m_scaleTickLength, length ) )
    {
        return;
    }
    m_scaleTickLength = length;
    updateGeometry();
    update();
    emit scaleTickLengthChanged( length );
}

void ExAudioLevelMeter::setChannelLabelsVisible( bool visible )
{
    if ( m_channelLabelsVisible == visible )
    {
        return;
    }
    m_channelLabelsVisible = visible;
    updateGeometry();
    update();
    emit channelLabelsVisibleChanged( visible );
}

void ExAudioLevelMeter::setPeakHoldEnabled( bool enabled )
{
    if ( m_peakHoldEnabled == enabled )
    {
        return;
    }
    m_peakHoldEnabled = enabled;
    if ( !enabled )
    {
        m_peakLevels = m_displayedLevels;
    }
    updateAnimationState();
    update();
    emit peakHoldEnabledChanged( enabled );
}

void ExAudioLevelMeter::setPeakHoldDuration( int duration )
{
    duration = qBound( 0, duration, 10000 );
    if ( m_peakHoldDuration == duration )
    {
        return;
    }
    m_peakHoldDuration = duration;
    emit peakHoldDurationChanged( duration );
}

void ExAudioLevelMeter::setDecayRate( qreal decibelsPerSecond )
{
    if ( !qIsFinite( decibelsPerSecond ) )
    {
        return;
    }
    decibelsPerSecond = qBound( 0.0, decibelsPerSecond, 1000.0 );
    if ( qFuzzyCompare( m_decayRate + 1.0, decibelsPerSecond + 1.0 ) )
    {
        return;
    }
    m_decayRate = decibelsPerSecond;
    emit decayRateChanged( decibelsPerSecond );
}

void ExAudioLevelMeter::setPeakDecayRate( qreal decibelsPerSecond )
{
    if ( !qIsFinite( decibelsPerSecond ) )
    {
        return;
    }
    decibelsPerSecond = qBound( 0.0, decibelsPerSecond, 1000.0 );
    if ( qFuzzyCompare( m_peakDecayRate + 1.0, decibelsPerSecond + 1.0 ) )
    {
        return;
    }
    m_peakDecayRate = decibelsPerSecond;
    emit peakDecayRateChanged( decibelsPerSecond );
}

void ExAudioLevelMeter::setInputTimeout( int timeout )
{
    timeout = qBound( 0, timeout, 10000 );
    if ( m_inputTimeout == timeout )
    {
        return;
    }
    m_inputTimeout = timeout;
    updateAnimationState();
    emit inputTimeoutChanged( timeout );
}

void ExAudioLevelMeter::setAnimationEnabled( bool enabled )
{
    if ( m_animationEnabled == enabled )
    {
        return;
    }
    m_animationEnabled = enabled;
    if ( !enabled )
    {
        m_displayedLevels = m_levels;
        m_peakLevels = m_levels;
    }
    updateAnimationState();
    update();
    emit animationEnabledChanged( enabled );
}

void ExAudioLevelMeter::setColorMode( ColorMode mode )
{
    if ( mode < SingleColor || mode > GradientColors || m_colorMode == mode )
    {
        return;
    }
    m_colorMode = mode;
    update();
    emit colorModeChanged( mode );
}

void ExAudioLevelMeter::setBackgroundColor( QColor color )
{
    if ( m_backgroundColor == color )
    {
        return;
    }
    m_backgroundColor = color;
    update();
    emit backgroundColorChanged( color );
}

void ExAudioLevelMeter::setActiveColor( QColor color )
{
    if ( m_activeColor == color )
    {
        return;
    }
    m_activeColor = color;
    update();
    emit activeColorChanged( color );
}

void ExAudioLevelMeter::setInactiveColor( QColor color )
{
    if ( m_inactiveColor == color )
    {
        return;
    }
    m_inactiveColor = color;
    update();
    emit inactiveColorChanged( color );
}

void ExAudioLevelMeter::setWarningColor( QColor color )
{
    if ( m_warningColor == color )
    {
        return;
    }
    m_warningColor = color;
    update();
    emit warningColorChanged( color );
}

void ExAudioLevelMeter::setClipColor( QColor color )
{
    if ( m_clipColor == color )
    {
        return;
    }
    m_clipColor = color;
    update();
    emit clipColorChanged( color );
}

void ExAudioLevelMeter::setPeakColor( QColor color )
{
    if ( m_peakColor == color )
    {
        return;
    }
    m_peakColor = color;
    update();
    emit peakColorChanged( color );
}

void ExAudioLevelMeter::setScaleColor( QColor color )
{
    if ( m_scaleColor == color )
    {
        return;
    }
    m_scaleColor = color;
    update();
    emit scaleColorChanged( color );
}

QStringList ExAudioLevelMeter::channelLabels() const
{
    return m_channelLabels;
}

void ExAudioLevelMeter::setChannelLabels( const QStringList& labels )
{
    if ( m_channelLabels == labels )
    {
        return;
    }
    m_channelLabels = labels;
    update();
    emit channelLabelsChanged( labels );
}

QVector<qreal> ExAudioLevelMeter::customScaleValues() const
{
    return m_customScaleValues;
}

void ExAudioLevelMeter::setCustomScaleValues( const QVector<qreal>& values )
{
    QVector<qreal> normalized;
    normalized.reserve( values.size() );
    for ( qreal value : values )
    {
        if ( qIsFinite( value ) )
        {
            normalized.append( value );
        }
    }
    std::sort( normalized.begin(), normalized.end(), std::greater<qreal>() );
    normalized.erase( std::unique( normalized.begin(),
                                   normalized.end(),
                                   []( qreal first, qreal second )
                                   { return qAbs( first - second ) < 0.0001; } ),
                      normalized.end() );

    if ( m_customScaleValues == normalized )
    {
        return;
    }
    m_customScaleValues = normalized;
    updateGeometry();
    update();
    emit customScaleValuesChanged( m_customScaleValues );
}

QVector<qreal> ExAudioLevelMeter::levels() const
{
    return m_levels;
}

QVector<qreal> ExAudioLevelMeter::displayedLevels() const
{
    return m_displayedLevels;
}

QVector<qreal> ExAudioLevelMeter::peakLevels() const
{
    return m_peakLevels;
}

qreal ExAudioLevelMeter::level( int channel ) const
{
    return channel >= 0 && channel < m_levels.size() ? m_levels.at( channel ) : m_minimumDecibels;
}

qreal ExAudioLevelMeter::displayedLevel( int channel ) const
{
    return channel >= 0 && channel < m_displayedLevels.size() ? m_displayedLevels.at( channel )
                                                              : m_minimumDecibels;
}

qreal ExAudioLevelMeter::peakLevel( int channel ) const
{
    return channel >= 0 && channel < m_peakLevels.size() ? m_peakLevels.at( channel ) : m_minimumDecibels;
}

bool ExAudioLevelMeter::isRunning() const
{
    return m_animationTimer->isActive();
}

QSize ExAudioLevelMeter::sizeHint() const
{
    const int width = m_channelCount == 1 ? 100 : 70 + m_channelCount * 42;
    return QSize( width, 320 );
}

QSize ExAudioLevelMeter::minimumSizeHint() const
{
    return QSize( m_channelCount == 1 ? 64 : 48 + m_channelCount * 24, 140 );
}

void ExAudioLevelMeter::setLevel( qreal decibels )
{
    setLevels( QVector<qreal>{decibels} );
}

void ExAudioLevelMeter::setStereoLevels( qreal leftDecibels, qreal rightDecibels )
{
    setLevels( QVector<qreal>{leftDecibels, rightDecibels} );
}

void ExAudioLevelMeter::setLevels( const QVector<qreal>& decibels )
{
    if ( QThread::currentThread() != thread() )
    {
        const QVector<qreal> copy = decibels;
        QMetaObject::invokeMethod( this, [this, copy] { applyLevels( copy ); }, Qt::QueuedConnection );
        return;
    }
    applyLevels( decibels );
}

void ExAudioLevelMeter::setLinearLevel( qreal amplitude )
{
    setLinearLevels( QVector<qreal>{amplitude} );
}

void ExAudioLevelMeter::setLinearLevels( const QVector<qreal>& amplitudes )
{
    QVector<qreal> decibels;
    decibels.reserve( amplitudes.size() );
    for ( qreal amplitude : amplitudes )
    {
        const qreal magnitude = qAbs( amplitude );
        decibels.append( magnitude > 0.0 ? 20.0 * std::log10( magnitude ) : -160.0 );
    }
    setLevels( decibels );
}

void ExAudioLevelMeter::resetPeaks()
{
    m_peakLevels = m_displayedLevels;
    m_peakHoldRemaining.fill( m_peakHoldDuration );
    update();
    emit peakLevelsChanged( m_peakLevels );
}

void ExAudioLevelMeter::clear()
{
    m_levels.fill( m_minimumDecibels );
    m_displayedLevels.fill( m_minimumDecibels );
    m_peakLevels.fill( m_minimumDecibels );
    m_peakHoldRemaining.fill( 0 );
    m_inputElapsed.invalidate();
    updateAnimationState();
    update();
    emit levelsChanged( m_levels );
    emit peakLevelsChanged( m_peakLevels );
}

void ExAudioLevelMeter::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.fillRect( rect(), themedColor( this, m_backgroundColor, "#F4F4F4", "#111111" ) );

    const QFontMetrics metrics( font() );
    const QVector<qreal> ticks = scaleValues();
    const qreal labelHeight = m_channelLabelsVisible ? metrics.height() + 6.0 : 0.0;
    qreal scaleLabelWidth = 0.0;
    for ( qreal value : ticks )
    {
        scaleLabelWidth = qMax( scaleLabelWidth,
                                static_cast<qreal>( metrics.horizontalAdvance( scaleLabel( value ) ) ) );
    }
    const qreal tickSpace = m_scaleTickMarksVisible ? m_scaleTickLength + 4.0 : 0.0;
    const qreal scaleWidth = m_scalePosition == NoScale ? 0.0 : scaleLabelWidth + tickSpace + 4.0;
    QRectF content = QRectF( rect() ).adjusted( 10.0, 10.0, -10.0, -10.0 - labelHeight );
    if ( content.width() <= 0.0 || content.height() < 4.0 )
    {
        return;
    }

    MeterLayout layout;
    layout.content = content;
    ScalePosition scalePosition = m_scalePosition;
    if ( scalePosition == CenterScale && m_channelCount != 2 )
    {
        scalePosition = RightScale;
    }

    QRectF meterArea = content;
    if ( scalePosition == LeftScale )
    {
        layout.scale = QRectF( content.left(), content.top(), scaleWidth, content.height() );
        meterArea.setLeft( layout.scale.right() + m_channelSpacing );
    }
    else if ( scalePosition == RightScale )
    {
        layout.scale = QRectF( content.right() - scaleWidth, content.top(), scaleWidth, content.height() );
        meterArea.setRight( layout.scale.left() - m_channelSpacing );
    }

    if ( scalePosition == CenterScale )
    {
        const qreal meterWidth = ( content.width() - scaleWidth - 2.0 * m_channelSpacing ) * 0.5;
        if ( meterWidth <= 0.0 )
        {
            return;
        }
        layout.channels.append( QRectF( content.left(), content.top(), meterWidth, content.height() ) );
        layout.scale = QRectF( content.left() + meterWidth + m_channelSpacing,
                               content.top(),
                               scaleWidth,
                               content.height() );
        layout.channels.append( QRectF( layout.scale.right() + m_channelSpacing,
                                        content.top(),
                                        meterWidth,
                                        content.height() ) );
    }
    else
    {
        const qreal totalSpacing = m_channelSpacing * ( m_channelCount - 1 );
        const qreal channelWidth = ( meterArea.width() - totalSpacing ) / m_channelCount;
        if ( channelWidth <= 0.0 )
        {
            return;
        }
        for ( int channel = 0; channel < m_channelCount; ++channel )
        {
            layout.channels.append( QRectF( meterArea.left() + channel * ( channelWidth + m_channelSpacing ),
                                             meterArea.top(),
                                             channelWidth,
                                             meterArea.height() ) );
        }
    }

    QColor inactive = resolvedInactiveColor();
    QColor peak = withDisabledAlpha(
        themedColor( this, m_peakColor, "#202020", "#FFFFFF" ), isEnabled() );
    painter.setPen( Qt::NoPen );

    for ( int channel = 0; channel < layout.channels.size(); ++channel )
    {
        const QRectF channelRect = layout.channels.at( channel );
        const int visibleSegments = qMin( m_segmentCount,
                                          qMax( 2, qFloor( channelRect.height() / 2.0 ) ) );
        const qreal spacing = qMin( m_segmentSpacing,
                                    channelRect.height() * 0.45 / ( visibleSegments - 1 ) );
        const qreal segmentHeight = ( channelRect.height() - spacing * ( visibleSegments - 1 ) )
                                    / visibleSegments;
        const qreal displayedRatio = levelRatio( displayedLevel( channel ) );

        for ( int segment = 0; segment < visibleSegments; ++segment )
        {
            const qreal segmentRatio = static_cast<qreal>( segment + 1 ) / visibleSegments;
            const qreal segmentDecibels = m_minimumDecibels
                                          + segmentRatio * ( m_maximumDecibels - m_minimumDecibels );
            const qreal y = channelRect.bottom() - ( segment + 1 ) * segmentHeight - segment * spacing;
            const QRectF segmentRect( channelRect.left(), y, channelRect.width(), segmentHeight );

            QColor color = segmentRatio <= displayedRatio ? colorForLevel( segmentDecibels ) : inactive;
            painter.setBrush( withDisabledAlpha( color, isEnabled() ) );
            painter.drawRoundedRect( segmentRect, m_segmentRadius, m_segmentRadius );
        }

        if ( m_peakHoldEnabled && peakLevel( channel ) > m_minimumDecibels )
        {
            const qreal peakRatio = levelRatio( peakLevel( channel ) );
            const qreal y = channelRect.bottom() - peakRatio * channelRect.height();
            const qreal markerHeight = qMax( 1.0, qMin( 2.0, segmentHeight ) );
            painter.setBrush( peak );
            painter.drawRoundedRect( QRectF( channelRect.left(),
                                              y - markerHeight * 0.5,
                                              channelRect.width(),
                                              markerHeight ),
                                     markerHeight * 0.5,
                                     markerHeight * 0.5 );
        }

        if ( m_channelLabelsVisible )
        {
            painter.setPen( resolvedScaleColor() );
            const QRectF labelRect( channelRect.left(), content.bottom() + 4.0, channelRect.width(), labelHeight );
            painter.drawText( labelRect, Qt::AlignHCenter | Qt::AlignTop, resolvedChannelLabel( channel ) );
            painter.setPen( Qt::NoPen );
        }
    }

    if ( !layout.scale.isEmpty() )
    {
        painter.setPen( withDisabledAlpha( resolvedScaleColor(), isEnabled() ) );
        const bool labelsOnRight = scalePosition != LeftScale;
        for ( qreal value : ticks )
        {
            const qreal y = content.bottom() - levelRatio( value ) * content.height();
            const QString text = scaleLabel( value );
            QRectF textRect = layout.scale;
            textRect.setTop( y - metrics.height() * 0.5 );
            textRect.setHeight( metrics.height() );
            if ( m_scaleTickMarksVisible )
            {
                if ( labelsOnRight )
                {
                    painter.drawLine( QPointF( layout.scale.left(), y ),
                                      QPointF( layout.scale.left() + m_scaleTickLength, y ) );
                    textRect.setLeft( textRect.left() + tickSpace );
                }
                else
                {
                    painter.drawLine( QPointF( layout.scale.right() - m_scaleTickLength, y ),
                                      QPointF( layout.scale.right(), y ) );
                    textRect.setRight( textRect.right() - tickSpace );
                }
            }
            painter.drawText( textRect,
                              ( labelsOnRight ? Qt::AlignLeft : Qt::AlignRight ) | Qt::AlignVCenter,
                              text );
        }
    }
}

void ExAudioLevelMeter::showEvent( QShowEvent* event )
{
    QWidget::showEvent( event );
    updateAnimationState();
}

void ExAudioLevelMeter::hideEvent( QHideEvent* event )
{
    QWidget::hideEvent( event );
    updateAnimationState();
}

void ExAudioLevelMeter::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::EnabledChange )
    {
        updateAnimationState();
    }
    if ( event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange
         || event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::StyleChange
         || event->type() == QEvent::FontChange )
    {
        update();
    }
}

void ExAudioLevelMeter::applyLevels( const QVector<qreal>& decibels )
{
    if ( decibels.isEmpty() )
    {
        clear();
        return;
    }
    if ( decibels.size() != m_channelCount )
    {
        setChannelCount( decibels.size() );
    }

    bool peakChanged = false;
    for ( int channel = 0; channel < m_channelCount; ++channel )
    {
        const qreal value = boundedLevel( decibels.value( channel, m_minimumDecibels ) );
        m_levels[channel] = value;
        if ( !m_animationEnabled || value >= m_displayedLevels.at( channel ) )
        {
            m_displayedLevels[channel] = value;
        }
        if ( value >= m_peakLevels.at( channel ) )
        {
            m_peakLevels[channel] = value;
            m_peakHoldRemaining[channel] = m_peakHoldDuration;
            peakChanged = true;
        }
    }

    if ( !m_inputElapsed.isValid() )
    {
        m_inputElapsed.start();
    }
    else
    {
        m_inputElapsed.restart();
    }
    updateAnimationState();
    update();
    emit levelsChanged( m_levels );
    if ( peakChanged )
    {
        emit peakLevelsChanged( m_peakLevels );
    }
}

void ExAudioLevelMeter::updateAnimationState()
{
    const bool wasRunning = m_animationTimer->isActive();
    const bool waitingForTimeout = m_inputTimeout > 0 && m_inputElapsed.isValid()
                                   && m_inputElapsed.elapsed() < m_inputTimeout;
    const bool timedOut = m_inputTimeout > 0 && m_inputElapsed.isValid() && !waitingForTimeout;
    bool hasMotion = false;
    for ( int channel = 0; channel < m_channelCount; ++channel )
    {
        const qreal target = timedOut ? m_minimumDecibels : m_levels.at( channel );
        hasMotion = hasMotion || qAbs( m_displayedLevels.at( channel ) - target ) > 0.001
                    || ( m_peakHoldEnabled
                         && m_peakLevels.at( channel ) > m_displayedLevels.at( channel ) + 0.001 );
    }
    const bool shouldRun = m_animationEnabled && isVisible() && isEnabled()
                           && ( hasMotion || waitingForTimeout || timedOut );

    if ( shouldRun && !wasRunning )
    {
        m_frameElapsed.start();
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

void ExAudioLevelMeter::updateAnimationFrame()
{
    const qint64 elapsedMs = qBound<qint64>( qint64( 0 ), m_frameElapsed.restart(), qint64( 100 ) );
    const qreal elapsedSeconds = elapsedMs / 1000.0;
    const bool timedOut = m_inputTimeout > 0 && m_inputElapsed.isValid()
                          && m_inputElapsed.elapsed() >= m_inputTimeout;
    bool changed = false;
    bool peakChanged = false;

    for ( int channel = 0; channel < m_channelCount; ++channel )
    {
        const qreal target = timedOut ? m_minimumDecibels : m_levels.at( channel );
        qreal& displayed = m_displayedLevels[channel];
        if ( target >= displayed )
        {
            displayed = target;
        }
        else
        {
            const qreal next = qMax( target, displayed - m_decayRate * elapsedSeconds );
            changed = changed || !qFuzzyCompare( displayed + 1.0, next + 1.0 );
            displayed = next;
        }

        if ( !m_peakHoldEnabled )
        {
            m_peakLevels[channel] = displayed;
            continue;
        }
        if ( displayed > m_peakLevels.at( channel ) + 0.001 )
        {
            m_peakLevels[channel] = displayed;
            m_peakHoldRemaining[channel] = m_peakHoldDuration;
            peakChanged = true;
        }
        else if ( m_peakHoldRemaining.at( channel ) > 0 )
        {
            m_peakHoldRemaining[channel] = qMax( 0, m_peakHoldRemaining.at( channel ) - static_cast<int>( elapsedMs ) );
        }
        else
        {
            const qreal nextPeak = qMax( displayed,
                                         m_peakLevels.at( channel ) - m_peakDecayRate * elapsedSeconds );
            peakChanged = peakChanged
                          || !qFuzzyCompare( m_peakLevels.at( channel ) + 1.0, nextPeak + 1.0 );
            m_peakLevels[channel] = nextPeak;
        }
    }

    if ( timedOut )
    {
        m_levels.fill( m_minimumDecibels );
        m_inputElapsed.invalidate();
    }
    if ( changed || peakChanged )
    {
        update();
    }
    if ( peakChanged )
    {
        emit peakLevelsChanged( m_peakLevels );
    }
    updateAnimationState();
}

void ExAudioLevelMeter::resizeLevelStorage()
{
    const int oldSize = m_levels.size();
    m_levels.resize( m_channelCount );
    m_displayedLevels.resize( m_channelCount );
    m_peakLevels.resize( m_channelCount );
    m_peakHoldRemaining.resize( m_channelCount );
    for ( int channel = 0; channel < m_channelCount; ++channel )
    {
        if ( channel >= oldSize )
        {
            m_levels[channel] = m_minimumDecibels;
            m_displayedLevels[channel] = m_minimumDecibels;
            m_peakLevels[channel] = m_minimumDecibels;
            m_peakHoldRemaining[channel] = 0;
            continue;
        }
        m_levels[channel] = boundedLevel( m_levels.at( channel ) );
        m_displayedLevels[channel] = boundedLevel( m_displayedLevels.at( channel ) );
        m_peakLevels[channel] = boundedLevel( m_peakLevels.at( channel ) );
        m_peakHoldRemaining[channel] = qMax( 0, m_peakHoldRemaining.at( channel ) );
    }
}

qreal ExAudioLevelMeter::boundedLevel( qreal decibels ) const
{
    if ( !qIsFinite( decibels ) )
    {
        return m_minimumDecibels;
    }
    return qBound( m_minimumDecibels, decibels, m_maximumDecibels );
}

qreal ExAudioLevelMeter::levelRatio( qreal decibels ) const
{
    return qBound( 0.0,
                   ( boundedLevel( decibels ) - m_minimumDecibels )
                       / ( m_maximumDecibels - m_minimumDecibels ),
                   1.0 );
}

QColor ExAudioLevelMeter::resolvedInactiveColor() const
{
    if ( m_inactiveColor.isValid() )
    {
        return m_inactiveColor;
    }
    QColor color = palette().color( isEnabled() ? QPalette::Active : QPalette::Disabled,
                                    QPalette::Text );
    color.setAlphaF( 0.14 );
    return color;
}

QColor ExAudioLevelMeter::resolvedScaleColor() const
{
    if ( m_scaleColor.isValid() )
    {
        return m_scaleColor;
    }
    QColor color = palette().color( isEnabled() ? QPalette::Active : QPalette::Disabled,
                                    QPalette::Text );
    color.setAlphaF( 0.58 );
    return color;
}

QColor ExAudioLevelMeter::colorForLevel( qreal decibels ) const
{
    const QColor active = themedColor( this, m_activeColor, "#C85D00", "#FF9F2D" );
    const QColor warning = themedColor( this, m_warningColor, "#A15C00", "#FFD166" );
    const QColor clip = themedColor( this, m_clipColor, "#C42B1C", "#FF5A5F" );
    if ( m_colorMode == SingleColor )
    {
        return active;
    }
    if ( m_colorMode == GradientColors )
    {
        if ( decibels <= m_warningDecibels )
        {
            const qreal range = m_warningDecibels - m_minimumDecibels;
            const qreal progress = range > 0.0 ? ( decibels - m_minimumDecibels ) / range : 1.0;
            return mixedColor( active, warning, progress );
        }
        if ( decibels <= m_clipDecibels )
        {
            const qreal range = m_clipDecibels - m_warningDecibels;
            const qreal progress = range > 0.0 ? ( decibels - m_warningDecibels ) / range : 1.0;
            return mixedColor( warning, clip, progress );
        }
        return clip;
    }
    if ( decibels >= m_clipDecibels )
    {
        return clip;
    }
    if ( decibels >= m_warningDecibels )
    {
        return warning;
    }
    return active;
}

QString ExAudioLevelMeter::resolvedChannelLabel( int channel ) const
{
    if ( channel >= 0 && channel < m_channelLabels.size() && !m_channelLabels.at( channel ).isEmpty() )
    {
        return m_channelLabels.at( channel );
    }
    if ( m_channelCount == 2 )
    {
        return channel == 0 ? QStringLiteral( "L" ) : QStringLiteral( "R" );
    }
    return QString::number( channel + 1 );
}

QVector<qreal> ExAudioLevelMeter::scaleValues() const
{
    QVector<qreal> values;
    if ( m_scaleMode == CustomScale )
    {
        for ( qreal value : m_customScaleValues )
        {
            if ( value >= m_minimumDecibels && value <= m_maximumDecibels )
            {
                values.append( value );
            }
        }
        if ( !values.isEmpty() )
        {
            return values;
        }
    }

    if ( m_scaleMode == FixedTickCount )
    {
        const qreal step = ( m_maximumDecibels - m_minimumDecibels ) / ( m_scaleTickCount - 1 );
        values.reserve( m_scaleTickCount );
        for ( int index = 0; index < m_scaleTickCount; ++index )
        {
            values.append( index == m_scaleTickCount - 1
                               ? m_minimumDecibels
                               : m_maximumDecibels - index * step );
        }
        return values;
    }

    for ( qreal value = m_maximumDecibels;
          value >= m_minimumDecibels - 0.001;
          value -= m_scaleInterval )
    {
        values.append( qMax( value, m_minimumDecibels ) );
    }
    if ( values.isEmpty() || values.constLast() > m_minimumDecibels + 0.001 )
    {
        values.append( m_minimumDecibels );
    }
    return values;
}

QString ExAudioLevelMeter::scaleLabel( qreal decibels ) const
{
    const qreal zeroThreshold = 0.5 * std::pow( 10.0, -m_scalePrecision );
    if ( qAbs( decibels ) < zeroThreshold )
    {
        decibels = 0.0;
    }
    QString label = QString::number( decibels, 'f', m_scalePrecision );
    if ( m_scaleUnitVisible && !m_scaleUnit.isEmpty() )
    {
        label += QLatin1Char( ' ' );
        label += m_scaleUnit;
    }
    return label;
}
