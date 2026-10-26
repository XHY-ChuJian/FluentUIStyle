#include "exspectrumwidget.h"

#include <QDateTime>
#include <QEvent>
#include <QLinearGradient>
#include <QMutex>
#include <QMutexLocker>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>

#include <cstring>

extern "C" {
#include "kiss_fft.h"
#include "kiss_fftr.h"
}

namespace {

constexpr float kMinDisplayLevel = 0.02f;
constexpr float kMinDbDbfs       = -54.0f; // 最小底噪分贝
constexpr float kMaxDbDbfs       = -3.0f;  // 最大峰值分贝

QVector<float> buildHannWindow( int size )
{
    QVector<float> window( size );
    if ( size <= 1 )
    {
        if ( !window.isEmpty() )
        {
            window[ 0 ] = 1.0f;
        }
        return window;
    }

    for ( int i = 0; i < size; ++i )
    {
        window[ i ] = 0.5f * ( 1.0f - qCos( 2.0f * float( M_PI ) * float( i ) / float( size - 1 ) ) );
    }
    return window;
}

QColor resolveAccentColor( const QPalette& palette )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    const QColor accent = palette.color( QPalette::Accent );
    if ( accent.isValid() )
    {
        return accent;
    }
#endif
    return palette.color( QPalette::Highlight );
}

/// 连续频率 Bin 幅值插值（消除低频离散 Bin 整数截断跳闪）
float getInterpolatedMag( const kiss_fft_cpx* fft, int maxBin, float binIndex )
{
    const int idx    = qBound( 1, int( binIndex ), maxBin - 2 );
    const float frac = qBound( 0.0f, binIndex - float( idx ), 1.0f );

    const float r0 = fft[ idx ].r;
    const float i0 = fft[ idx ].i;
    const float m0 = qSqrt( r0 * r0 + i0 * i0 );

    const float r1 = fft[ idx + 1 ].r;
    const float i1 = fft[ idx + 1 ].i;
    const float m1 = qSqrt( r1 * r1 + i1 * i1 );

    return m0 * ( 1.0f - frac ) + m1 * frac;
}

}  // namespace

// ============================================================================
// 频带定义与物理动力学状态结构
// ============================================================================

struct ExSpectrumWidget::Private
{
    struct BandDefinition
    {
        float centerHz { 0.0f };
        float lowBin   { 0.0f };
        float highBin  { 0.0f };
        float tiltGain { 1.0f };
    };

    struct BarPhysicsState
    {
        float height          { kMinDisplayLevel };
        float velocity        { 0.0f };
        float peakCapHeight   { kMinDisplayLevel };
        float peakCapVelocity { 0.0f };
        int   peakHoldFrames  { 0 };
    };

    struct PcmPacket
    {
        QByteArray data;
        int channels { 0 };
    };

    kiss_fftr_cfg fftCfg { nullptr };

    QMutex pcmMutex;
    QVector<PcmPacket> pendingPackets;
    int emptyChunkCount { 0 };
    QColor explicitAccent;
    bool rainbowEnabled { ExSpectrumWidget::DefaultRainbowEnabled };

    QVector<float> pcmSamples;
    QVector<float> fftInput;
    QVector<kiss_fft_cpx> fftOutput;
    QVector<float> window;
    float windowSum { 1.0f };

    QVector<BandDefinition> bands;
    QVector<BarPhysicsState> physics;

    QVector<float> targets;
    QVector<float> bars;
    QVector<float> peakCaps;

    int barCount { ExSpectrumWidget::DefaultBarCount };
    int sampleRate { 44100 };
    int channelCount { ExSpectrumWidget::DefaultChannelCount };
    int refreshIntervalMs { ExSpectrumWidget::DefaultRefreshIntervalMs };
    bool reflectionEnabled { true };
    float reflectionOpacity { ExSpectrumWidget::DefaultReflectionOpacity };
    int barGap { ExSpectrumWidget::DefaultBarGap };
    int barRadius { ExSpectrumWidget::DefaultBarRadius };
    float minFrequency { ExSpectrumWidget::DefaultMinFrequency };
    float maxFrequency { ExSpectrumWidget::DefaultMaxFrequency };

    bool peakCapEnabled { true };
    int peakCapHoldMs { ExSpectrumWidget::DefaultPeakCapHoldMs };
    int peakCapHeight { ExSpectrumWidget::DefaultPeakCapHeight };
    float gravity { ExSpectrumWidget::DefaultGravity };

    qint64 lastTimerTimestampMs { 0 };
    QTimer* timer { nullptr };
};

// ============================================================================
// 构造与析构
// ============================================================================

ExSpectrumWidget::ExSpectrumWidget( QWidget* parent )
    : QWidget( parent )
    , d( new Private )
{
    setAttribute( Qt::WA_OpaquePaintEvent, false );
    setAutoFillBackground( false );

    d->targets.fill( kMinDisplayLevel, d->barCount );
    d->bars.fill( kMinDisplayLevel, d->barCount );
    d->peakCaps.fill( kMinDisplayLevel, d->barCount );
    d->physics.resize( d->barCount );

    d->fftInput.fill( 0.0f, DefaultFftSize );
    d->fftOutput.resize( DefaultFftSize / 2 + 1 );
    d->window    = buildHannWindow( DefaultFftSize );
    d->windowSum = 0.0f;
    for ( float w : d->window )
    {
        d->windowSum += w;
    }
    if ( d->windowSum <= 0.0f )
    {
        d->windowSum = 1.0f;
    }

    d->fftCfg = kiss_fftr_alloc( DefaultFftSize, 0, nullptr, nullptr );

    d->timer = new QTimer( this );
    d->timer->setTimerType( Qt::PreciseTimer );
    connect( d->timer, &QTimer::timeout, this, &ExSpectrumWidget::onRefreshTimer );
    d->timer->start( d->refreshIntervalMs );

    rebuildLogBinMap();
}

ExSpectrumWidget::~ExSpectrumWidget()
{
    if ( d->fftCfg )
    {
        kiss_fftr_free( d->fftCfg );
        d->fftCfg = nullptr;
    }
    delete d;
}

// ============================================================================
// 音频数据推入接口
// ============================================================================

void ExSpectrumWidget::pushAudioData( const QByteArray& pcmData )
{
    if ( pcmData.isEmpty() )
    {
        return;
    }

    QMutexLocker locker( &d->pcmMutex );
    d->pendingPackets.append( Private::PcmPacket { pcmData, 0 } );
}

void ExSpectrumWidget::pushAudioData( const QByteArray& pcmData, int channels )
{
    if ( pcmData.isEmpty() && channels <= 0 )
    {
        return;
    }

    QMutexLocker locker( &d->pcmMutex );
    d->pendingPackets.append( Private::PcmPacket { pcmData, channels } );
}

void ExSpectrumWidget::setAudioData( const QByteArray& pcmData )
{
    pushAudioData( pcmData );
}

// ============================================================================
// 属性 Getter / Setter
// ============================================================================

void ExSpectrumWidget::setSampleRate( int sampleRate )
{
    const int clamped = qMax( 8000, sampleRate );
    if ( d->sampleRate == clamped )
    {
        return;
    }

    d->sampleRate = clamped;
    rebuildLogBinMap();
    emit sampleRateChanged( d->sampleRate );
}

int ExSpectrumWidget::sampleRate() const
{
    return d->sampleRate;
}

void ExSpectrumWidget::setChannelCount( int channels )
{
    const int clamped = qBound( 1, channels, 8 );
    if ( d->channelCount == clamped )
    {
        return;
    }

    d->channelCount = clamped;
    emit channelCountChanged( d->channelCount );
}

int ExSpectrumWidget::channelCount() const
{
    return d->channelCount;
}

void ExSpectrumWidget::setReflectionEnabled( bool enabled )
{
    if ( d->reflectionEnabled == enabled )
    {
        return;
    }

    d->reflectionEnabled = enabled;
    emit reflectionEnabledChanged( d->reflectionEnabled );
    update();
}

bool ExSpectrumWidget::isReflectionEnabled() const
{
    return d->reflectionEnabled;
}

void ExSpectrumWidget::setReflectionOpacity( float opacity )
{
    const float clamped = qBound( 0.0f, opacity, 1.0f );
    if ( qFuzzyCompare( d->reflectionOpacity, clamped ) )
    {
        return;
    }

    d->reflectionOpacity = clamped;
    emit reflectionOpacityChanged( d->reflectionOpacity );
    update();
}

float ExSpectrumWidget::reflectionOpacity() const
{
    return d->reflectionOpacity;
}

void ExSpectrumWidget::setBarGap( int gap )
{
    const int clamped = qBound( 0, gap, 20 );
    if ( d->barGap == clamped )
    {
        return;
    }

    d->barGap = clamped;
    emit barGapChanged( d->barGap );
    update();
}

int ExSpectrumWidget::barGap() const
{
    return d->barGap;
}

void ExSpectrumWidget::setBarRadius( int radius )
{
    const int clamped = qBound( 0, radius, 16 );
    if ( d->barRadius == clamped )
    {
        return;
    }

    d->barRadius = clamped;
    emit barRadiusChanged( d->barRadius );
    update();
}

int ExSpectrumWidget::barRadius() const
{
    return d->barRadius;
}

void ExSpectrumWidget::setMinFrequency( float minFreq )
{
    const float upper = qMax( 10.0f, d->maxFrequency - 100.0f );
    const float clamped = qBound( 10.0f, minFreq, upper );
    if ( qFuzzyCompare( d->minFrequency, clamped ) )
    {
        return;
    }

    d->minFrequency = clamped;
    rebuildLogBinMap();
    emit minFrequencyChanged( d->minFrequency );
    update();
}

float ExSpectrumWidget::minFrequency() const
{
    return d->minFrequency;
}

void ExSpectrumWidget::setMaxFrequency( float maxFreq )
{
    const float clamped = qMax( d->minFrequency + 100.0f, maxFreq );
    if ( qFuzzyCompare( d->maxFrequency, clamped ) )
    {
        return;
    }

    d->maxFrequency = clamped;
    rebuildLogBinMap();
    emit maxFrequencyChanged( d->maxFrequency );
    update();
}

float ExSpectrumWidget::maxFrequency() const
{
    return d->maxFrequency;
}

void ExSpectrumWidget::setBarCount( int count )
{
    const int clamped = qBound( 8, count, 256 );
    if ( d->barCount == clamped )
    {
        return;
    }

    d->barCount = clamped;
    d->targets.fill( kMinDisplayLevel, d->barCount );
    d->bars.fill( kMinDisplayLevel, d->barCount );
    d->peakCaps.fill( kMinDisplayLevel, d->barCount );
    d->physics.resize( d->barCount );
    for ( auto& p : d->physics )
    {
        p = Private::BarPhysicsState {};
    }

    rebuildLogBinMap();
    emit barCountChanged( d->barCount );
    emit barsChanged( d->bars );
    emit peakCapsChanged( d->peakCaps );
    update();
}

int ExSpectrumWidget::barCount() const
{
    return d->barCount;
}

void ExSpectrumWidget::setRefreshIntervalMs( int intervalMs )
{
    const int clamped = qBound( 8, intervalMs, 1000 );
    if ( d->refreshIntervalMs == clamped )
    {
        return;
    }

    d->refreshIntervalMs = clamped;
    if ( d->timer )
    {
        d->timer->setInterval( d->refreshIntervalMs );
    }
    emit refreshIntervalMsChanged( d->refreshIntervalMs );
}

int ExSpectrumWidget::refreshIntervalMs() const
{
    return d->refreshIntervalMs;
}

void ExSpectrumWidget::setPeakCapEnabled( bool enabled )
{
    if ( d->peakCapEnabled == enabled )
    {
        return;
    }

    d->peakCapEnabled = enabled;
    emit peakCapEnabledChanged( d->peakCapEnabled );
    update();
}

bool ExSpectrumWidget::isPeakCapEnabled() const
{
    return d->peakCapEnabled;
}

void ExSpectrumWidget::setPeakCapHoldMs( int ms )
{
    const int clamped = qBound( 0, ms, 2000 );
    if ( d->peakCapHoldMs == clamped )
    {
        return;
    }

    d->peakCapHoldMs = clamped;
    emit peakCapHoldMsChanged( d->peakCapHoldMs );
}

int ExSpectrumWidget::peakCapHoldMs() const
{
    return d->peakCapHoldMs;
}

void ExSpectrumWidget::setPeakCapHeight( int height )
{
    const int clamped = qBound( 1, height, 16 );
    if ( d->peakCapHeight == clamped )
    {
        return;
    }

    d->peakCapHeight = clamped;
    emit peakCapHeightChanged( d->peakCapHeight );
    update();
}

int ExSpectrumWidget::peakCapHeight() const
{
    return d->peakCapHeight;
}

void ExSpectrumWidget::setGravity( float g )
{
    const float clamped = qBound( 0.5f, g, 50.0f );
    if ( qFuzzyCompare( d->gravity, clamped ) )
    {
        return;
    }

    d->gravity = clamped;
    emit gravityChanged( d->gravity );
}

float ExSpectrumWidget::gravity() const
{
    return d->gravity;
}

QVector<float> ExSpectrumWidget::bars() const
{
    return d->bars;
}

QVector<float> ExSpectrumWidget::peakCaps() const
{
    return d->peakCaps;
}

void ExSpectrumWidget::clear()
{
    {
        QMutexLocker locker( &d->pcmMutex );
        d->pendingPackets.clear();
    }
    d->emptyChunkCount = 0;
    d->pcmSamples.clear();
    d->targets.fill( kMinDisplayLevel, d->barCount );
    d->bars.fill( kMinDisplayLevel, d->barCount );
    d->peakCaps.fill( kMinDisplayLevel, d->barCount );
    for ( auto& p : d->physics )
    {
        p = Private::BarPhysicsState {};
    }
    emit barsChanged( d->bars );
    emit peakCapsChanged( d->peakCaps );
    update();
}

void ExSpectrumWidget::setAccentColor( QColor color )
{
    if ( !color.isValid() )
    {
        color = QColor();
    }
    if ( d->explicitAccent == color )
    {
        return;
    }

    const QColor oldColor = accentColor();
    d->explicitAccent     = color;
    const QColor newColor = accentColor();
    if ( oldColor != newColor )
    {
        emit accentColorChanged( newColor );
    }
    update();
}

QColor ExSpectrumWidget::accentColor() const
{
    if ( d->explicitAccent.isValid() )
    {
        return d->explicitAccent;
    }
    return resolveAccentColor( palette() );
}

void ExSpectrumWidget::setRainbowEnabled( bool enabled )
{
    if ( d->rainbowEnabled == enabled )
    {
        return;
    }

    d->rainbowEnabled = enabled;
    emit rainbowEnabledChanged( d->rainbowEnabled );
    update();
}

bool ExSpectrumWidget::isRainbowEnabled() const
{
    return d->rainbowEnabled;
}

namespace {

QColor rainbowBarColor( int index, int count )
{
    const float span = ( count <= 1 ) ? 0.0f : float( index ) / float( count - 1 );
    // 左红，经橙黄绿青，到右紫。色相停在 285，避免右端绕回红色。
    const int hue = qBound( 0, qRound( span * 285.0f ), 285 );
    return QColor::fromHsv( hue, 220, 250 );
}

} // namespace

QSize ExSpectrumWidget::sizeHint() const
{
    return { 640, 160 };
}

QSize ExSpectrumWidget::minimumSizeHint() const
{
    return { 200, 80 };
}

// ============================================================================
// 绘制管线 (Fluent 柱体 + 浮动峰值帽 + 镜像倒影)
// ============================================================================

void ExSpectrumWidget::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event )

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );

    const QRect area = rect();
    painter.fillRect( area, palette().color( QPalette::Window ) );

    if ( d->barCount <= 0 || area.width() <= 0 || area.height() <= 0 )
    {
        return;
    }

    const bool hasReflection = d->reflectionEnabled;
    const int waterY         = hasReflection ? area.height() / 2 : area.height() - 2;
    const int totalGap       = d->barGap * ( d->barCount - 1 );
    const int barWidth       = qMax( 1, ( area.width() - totalGap ) / d->barCount );
    const int usedWidth      = barWidth * d->barCount + totalGap;
    const int xOffset        = ( area.width() - usedWidth ) / 2;
    const int maxBarHeight   = hasReflection ? qMax( 8, waterY - 10 ) : qMax( 8, area.height() - 4 );

    const bool dark          = palette().color( QPalette::Window ).lightness() < 128;
    const QColor divider     = QColor( palette().color( QPalette::Text ).red(),
                                   palette().color( QPalette::Text ).green(),
                                   palette().color( QPalette::Text ).blue(),
                                   dark ? 20 : 30 );

    const QColor solidColor = accentColor();
    const int capH           = qBound( 1, d->peakCapHeight, 4 );

    painter.setPen( Qt::NoPen );

    for ( int i = 0; i < d->barCount; ++i )
    {
        const int x         = xOffset + i * ( barWidth + d->barGap );
        const int barHeight = qMax( 2, int( d->bars[ i ] * maxBarHeight ) );
        const QColor barColor = d->rainbowEnabled ? rainbowBarColor( i, d->barCount ) : solidColor;
        const QColor peakColor = d->rainbowEnabled
            ? QColor::fromHsv( barColor.hue(), 130, 255 )
            : ( dark ? barColor.lighter( 135 ) : barColor.darker( 115 ) );

        // 1. 柱体绘制
        const QRectF upperRect( x, waterY - barHeight, barWidth, barHeight );
        painter.setBrush( barColor );
        painter.drawRoundedRect( upperRect, d->barRadius, d->barRadius );

        // 2. 浮动峰值帽绘制
        if ( d->peakCapEnabled && d->peakCaps.size() > i )
        {
            const int peakY = waterY - int( d->peakCaps[ i ] * maxBarHeight );
            if ( peakY <= waterY - barHeight - capH )
            {
                const QRectF capRect( x, peakY, barWidth, capH );
                painter.setBrush( peakColor );
                painter.drawRoundedRect( capRect, 1, 1 );
            }
        }

        // 3. 水波倒影绘制
        if ( hasReflection && d->reflectionOpacity > 0.001f )
        {
            const int reflectHeight = qMax( 2, int( barHeight * 0.92f ) );
            const QRectF lowerRect( x, waterY + 1, barWidth, reflectHeight );

            QLinearGradient gradient( x, waterY + 1, x, waterY + 1 + reflectHeight );
            QColor top = barColor;
            top.setAlpha( qBound( 0, int( d->reflectionOpacity * 255.0f ), 255 ) );
            QColor bottom = barColor;
            bottom.setAlpha( 0 );
            gradient.setColorAt( 0.0, top );
            gradient.setColorAt( 1.0, bottom );

            painter.setBrush( gradient );
            painter.drawRoundedRect( lowerRect, d->barRadius, d->barRadius );

            // 浮动峰值帽倒影
            if ( d->peakCapEnabled && d->peakCaps.size() > i )
            {
                const int peakReflectY = waterY + 1 + int( d->peakCaps[ i ] * maxBarHeight * 0.92f );
                if ( peakReflectY >= waterY + 1 + reflectHeight + capH )
                {
                    const QRectF capReflectRect( x, peakReflectY, barWidth, capH );
                    QColor capReflectColor = peakColor;
                    capReflectColor.setAlpha( qBound( 0, int( d->reflectionOpacity * 180.0f ), 255 ) );
                    painter.setBrush( capReflectColor );
                    painter.drawRoundedRect( capReflectRect, 1, 1 );
                }
            }
        }
    }

    if ( hasReflection )
    {
        painter.setPen( divider );
        painter.drawLine( 0, waterY, area.width(), waterY );
    }
}

void ExSpectrumWidget::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    update();
}

void ExSpectrumWidget::changeEvent( QEvent* event )
{
    QWidget::changeEvent( event );
    if ( event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange
         || event->type() == QEvent::StyleChange || event->type() == QEvent::ThemeChange )
    {
        update();
    }
}

// ============================================================================
// 定时刷新与物理驱动
// ============================================================================

void ExSpectrumWidget::onRefreshTimer()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    float dt           = 0.016f;
    if ( d->lastTimerTimestampMs > 0 )
    {
        const qint64 delta = nowMs - d->lastTimerTimestampMs;
        dt                 = qBound( 0.004f, float( delta ) / 1000.0f, 0.050f );
    }
    d->lastTimerTimestampMs = nowMs;

    updateSpectrum();
    stepPhysics( dt );
    update();
}

// ============================================================================
// 频带对数映射表构建 (连续重采样与等响度频倾补偿)
// ============================================================================

void ExSpectrumWidget::rebuildLogBinMap()
{
    d->bands.resize( d->barCount );

    const float nyquist    = float( d->sampleRate ) * 0.5f;
    float maxFreq          = qMin( d->maxFrequency, nyquist * 0.95f );
    const float minFreq    = qBound( 10.0f, d->minFrequency, qMax( 10.0f, maxFreq - 100.0f ) );
    if ( maxFreq <= minFreq )
    {
        maxFreq = minFreq + 100.0f;
    }
    const float logMin  = qLn( minFreq );
    const float logMax  = qLn( qMax( logMin + 0.1f, maxFreq ) );

    for ( int bar = 0; bar < d->barCount; ++bar )
    {
        const float t0 = float( bar ) / float( d->barCount );
        const float t1 = float( bar + 1 ) / float( d->barCount );

        const float f0 = qExp( logMin + ( logMax - logMin ) * t0 );
        const float f1 = qExp( logMin + ( logMax - logMin ) * t1 );
        const float fc = qSqrt( f0 * f1 );

        d->bands[ bar ].centerHz = fc;
        d->bands[ bar ].lowBin   = f0 * float( DefaultFftSize ) / float( d->sampleRate );
        d->bands[ bar ].highBin  = f1 * float( DefaultFftSize ) / float( d->sampleRate );

        // 听觉等响度与粉红噪声频倾补偿（每倍频程 +3.0dB 增益，高频最高上限 +9dB / 2.82x）
        const float octavesFrom1k = float( qLn( qMax( fc, 100.0f ) / 1000.0f ) / qLn( 2.0f ) );
        const float tiltDb        = qBound( -6.0f, octavesFrom1k * 3.0f, 9.0f );
        d->bands[ bar ].tiltGain  = float( qPow( 10.0f, tiltDb / 20.0f ) );
    }
}

// ============================================================================
// FFT 与连续频带能量积分 (转 dBFS 感知标尺)
// ============================================================================

void ExSpectrumWidget::updateSpectrum()
{
    if ( !d->fftCfg )
    {
        return;
    }

    QVector<Private::PcmPacket> packets;
    {
        QMutexLocker locker( &d->pcmMutex );
        if ( !d->pendingPackets.isEmpty() )
        {
            packets.swap( d->pendingPackets );
        }
    }

    if ( packets.isEmpty() )
    {
        ++d->emptyChunkCount;
        // 跨线程推送偶发空一拍时保留当前窗口，连续约 64ms 没有新数据才视为停止。
        if ( d->emptyChunkCount >= 4 )
        {
            if ( d->emptyChunkCount == 4 )
            {
                d->pcmSamples.clear();
            }
            for ( int i = 0; i < d->barCount; ++i )
            {
                d->targets[ i ] = kMinDisplayLevel;
            }
            return;
        }
    }
    else
    {
        d->emptyChunkCount = 0;
        for ( const Private::PcmPacket& packet : packets )
        {
            if ( packet.channels > 0 && packet.channels != d->channelCount )
            {
                setChannelCount( packet.channels );
            }
            if ( packet.data.isEmpty() )
            {
                continue;
            }

            const int channels     = qMax( 1, d->channelCount );
            const int totalSamples = packet.data.size() / int( sizeof( qint16 ) );
            const int frameCount   = totalSamples / channels;
            const auto* samples    = reinterpret_cast<const qint16*>( packet.data.constData() );
            d->pcmSamples.reserve( d->pcmSamples.size() + frameCount );

            if ( channels == 1 )
            {
                for ( int i = 0; i < frameCount; ++i )
                {
                    d->pcmSamples.append( float( samples[ i ] ) / 32768.0f );
                }
            }
            else
            {
                for ( int i = 0; i < frameCount; ++i )
                {
                    qint32 sum     = 0;
                    const int base = i * channels;
                    for ( int ch = 0; ch < channels; ++ch )
                    {
                        sum += samples[ base + ch ];
                    }
                    d->pcmSamples.append( ( float( sum ) / float( channels ) ) / 32768.0f );
                }
            }
        }

        constexpr int kMaxBufferSamples = DefaultFftSize * 8;
        if ( d->pcmSamples.size() > kMaxBufferSamples )
        {
            d->pcmSamples.remove( 0, d->pcmSamples.size() - kMaxBufferSamples );
        }
    }

    // 若样本不足以进行一次完整 FFT 分析，目标能量回退到底噪
    if ( d->pcmSamples.size() < DefaultFftSize )
    {
        for ( int i = 0; i < d->barCount; ++i )
        {
            d->targets[ i ] = kMinDisplayLevel;
        }
        return;
    }

    // 取最新 2048 个时域样本加汉宁窗
    const float* recent = d->pcmSamples.constData() + ( d->pcmSamples.size() - DefaultFftSize );
    for ( int i = 0; i < DefaultFftSize; ++i )
    {
        d->fftInput[ i ] = recent[ i ] * d->window[ i ];
    }

    kiss_fftr( d->fftCfg, d->fftInput.constData(), d->fftOutput.data() );

    const int maxBin = DefaultFftSize / 2;

    for ( int bar = 0; bar < d->barCount; ++bar )
    {
        const auto& band = d->bands[ bar ];
        float blended    = 0.0f;

        // 若频带较窄（< 1.5 个 Bin，通常在低频），采用连续多点插值采样，彻底杜绝单 Bin 频闪跳跃
        if ( ( band.highBin - band.lowBin ) < 1.5f )
        {
            const float mLow    = getInterpolatedMag( d->fftOutput.constData(), maxBin, band.lowBin );
            const float mCenter = getInterpolatedMag( d->fftOutput.constData(), maxBin, ( band.lowBin + band.highBin ) * 0.5f );
            const float mHigh   = getInterpolatedMag( d->fftOutput.constData(), maxBin, band.highBin );
            blended             = ( mLow * 0.25f + mCenter * 0.50f + mHigh * 0.25f );
        }
        else
        {
            // 中高频：在对应 Bin 范围内进行 RMS 能量积分 + 峰值融合
            const int startBin = qBound( 1, int( qFloor( band.lowBin ) ), maxBin - 1 );
            const int endBin   = qBound( startBin + 1, int( qCeil( band.highBin ) ), maxBin );

            float sumSq   = 0.0f;
            float peakMag = 0.0f;
            int count     = 0;
            for ( int bin = startBin; bin < endBin; ++bin )
            {
                const float r = d->fftOutput[ bin ].r;
                const float i = d->fftOutput[ bin ].i;
                const float m = qSqrt( r * r + i * i );
                sumSq += m * m;
                peakMag = qMax( peakMag, m );
                ++count;
            }

            const float rms        = count > 0 ? qSqrt( sumSq / float( count ) ) : 0.0f;
            const float peakWeight = qBound( 0.30f, band.centerHz / 4000.0f, 0.70f );
            blended                = rms * ( 1.0f - peakWeight ) + peakMag * peakWeight;
        }

        const float amplitude = ( blended * 2.0f ) / float( DefaultFftSize );
        const float rawGain   = amplitude * band.tiltGain;

        // 转换为真实声学分贝标尺 (dBFS -54dB ~ -3dB)，符合人耳对音量的对数感知特性
        const float db         = ( rawGain > 1e-5f ) ? ( 20.0f * float( qLn( rawGain ) / qLn( 10.0f ) ) ) : kMinDbDbfs;
        const float normalized = qBound( 0.0f, ( db - kMinDbDbfs ) / ( kMaxDbDbfs - kMinDbDbfs ), 1.0f );

        d->targets[ bar ] = qMax( kMinDisplayLevel, normalized );
    }
}

// ============================================================================
// 动力学物理引擎 (重力落体 + 瞬态冲刺 + 悬浮峰值帽)
// ============================================================================

void ExSpectrumWidget::stepPhysics( float dt )
{
    bool changed               = false;
    const int holdFrames       = qMax( 1, int( float( d->peakCapHoldMs ) / ( dt * 1000.0f ) ) );
    const float barAttackSpeed = 28.0f;              // 冲刺响应速率
    const float peakDropGravity = d->gravity * 0.35f; // 峰值帽悬浮飘落重力

    for ( int i = 0; i < d->barCount; ++i )
    {
        auto& p            = d->physics[ i ];
        const float target = d->targets[ i ];

        // --- 1. 柱体动力学 ---
        if ( target > p.height )
        {
            // 向上爆发冲击：快速弹起，重力下落速度清零
            const float factor = qBound( 0.0f, dt * barAttackSpeed, 1.0f );
            p.height += ( target - p.height ) * factor;
            p.velocity = 0.0f;
        }
        else
        {
            // 向下自由落体：受重力加速度自然掉落
            p.velocity += d->gravity * dt;
            p.height -= p.velocity * dt;
            if ( p.height < target )
            {
                p.height   = target;
                p.velocity = 0.0f;
            }
        }
        p.height = qBound( kMinDisplayLevel, p.height, 1.0f );

        // --- 2. 浮动峰值帽动力学 ---
        if ( d->peakCapEnabled )
        {
            if ( p.height >= p.peakCapHeight )
            {
                // 柱体顶起峰值帽：重置悬停时间与初速度
                p.peakCapHeight   = p.height;
                p.peakHoldFrames  = holdFrames;
                p.peakCapVelocity = 0.0f;
            }
            else
            {
                if ( p.peakHoldFrames > 0 )
                {
                    --p.peakHoldFrames; // 悬停失重期
                }
                else
                {
                    // 悬停结束，轻柔重力飘落
                    p.peakCapVelocity += peakDropGravity * dt;
                    p.peakCapHeight -= p.peakCapVelocity * dt;
                    if ( p.peakCapHeight < p.height )
                    {
                        p.peakCapHeight   = p.height;
                        p.peakCapVelocity = 0.0f;
                    }
                }
            }
            p.peakCapHeight  = qBound( kMinDisplayLevel, p.peakCapHeight, 1.0f );
            d->peakCaps[ i ] = p.peakCapHeight;
        }

        if ( qAbs( p.height - d->bars[ i ] ) > 0.0005f )
        {
            d->bars[ i ] = p.height;
            changed      = true;
        }
    }

    if ( changed )
    {
        emit barsChanged( d->bars );
        if ( d->peakCapEnabled )
        {
            emit peakCapsChanged( d->peakCaps );
        }
    }
}
