#include "audiomaticimmersiveview.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QVariant>

#include "exspectrumwidget.h"

AudiomaticImmersiveView::AudiomaticImmersiveView( QWidget* parent )
    : QWidget( parent )
{
    setupUi();
    updateThemeStyle();
}

AudiomaticImmersiveView::~AudiomaticImmersiveView() = default;

void AudiomaticImmersiveView::setupUi()
{
    setFocusPolicy( Qt::StrongFocus );

    // 采用全屏重叠网格布局：波形铺满底层，歌曲信息与单行歌词悬浮顶部，CPU标签悬浮右下角
    auto* rootGrid = new QGridLayout( this );
    rootGrid->setContentsMargins( 0, 0, 0, 0 );
    rootGrid->setSpacing( 0 );

    // 1. 底层大波形
    m_visualizer = new ExSpectrumWidget( this );
    m_visualizer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    rootGrid->addWidget( m_visualizer, 0, 0 );

    // 2. 顶部居中悬浮歌曲信息与单行卡拉OK歌词横栏
    m_headerWidget = new QWidget( this );
    m_headerWidget->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    auto* headerLayout = new QVBoxLayout( m_headerWidget );
    // headerLayout->setContentsMargins( 24, 20, 24, 8 );
    headerLayout->setSpacing( 4 );
    headerLayout->setAlignment( Qt::AlignCenter );

    // 2.1 歌名 (10pt, 粗体, 居中)
    m_labelSongTitle = new QLabel( m_headerWidget );
    m_labelSongTitle->setAlignment( Qt::AlignCenter );
    m_labelSongTitle->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    QFont fontTitle = m_labelSongTitle->font();
    fontTitle.setPointSize( 10 );
    fontTitle.setBold( true );
    m_labelSongTitle->setFont( fontTitle );
    headerLayout->addWidget( m_labelSongTitle, 0, Qt::AlignCenter );

    // 2.2 歌手 (10pt, 居中)
    m_labelSongArtist = new QLabel( m_headerWidget );
    m_labelSongArtist->setAlignment( Qt::AlignCenter );
    m_labelSongArtist->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    QFont fontArtist = m_labelSongArtist->font();
    fontArtist.setPointSize( 10 );
    m_labelSongArtist->setFont( fontArtist );
    headerLayout->addWidget( m_labelSongArtist, 0, Qt::AlignCenter );

    // 2.3 单行卡拉OK歌词控件（居中自适应）
    m_lyricsWidget = new ExLyricsWidget( m_headerWidget );
    m_lyricsWidget->setViewMode( ExLyricsWidget::ViewMode::SingleLine );
    m_lyricsWidget->setKaraokeProgress( true );
    m_lyricsWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    m_lyricsWidget->setMinimumHeight( 34 );
    m_lyricsWidget->setMinimumWidth( 420 );
    m_lyricsWidget->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    headerLayout->addWidget( m_lyricsWidget, 0, Qt::AlignCenter );

    rootGrid->addWidget( m_headerWidget, 0, 0, Qt::AlignHCenter | Qt::AlignTop );

    // 3. 右下角纯净 CPU 标签（直接 setText，无边框无背景，鼠标事件完全穿透）
    m_labelCpuUsage = new QLabel( this );
    m_labelCpuUsage->setAlignment( Qt::AlignCenter );
    m_labelCpuUsage->setText( QStringLiteral( "CPU 0.0%" ) );
    m_labelCpuUsage->setToolTip( tr( "程序及系统 CPU 占用率。" ) );
    m_labelCpuUsage->setAttribute( Qt::WA_TransparentForMouseEvents, true );
    m_labelCpuUsage->setContentsMargins( 0, 0, 16, 12 );
    rootGrid->addWidget( m_labelCpuUsage, 0, 0, Qt::AlignRight | Qt::AlignBottom );
}

void AudiomaticImmersiveView::pushAudioData( const QByteArray& pcmData, int channels )
{
    if ( m_visualizer )
    {
        m_visualizer->pushAudioData( pcmData, channels );
    }
}

void AudiomaticImmersiveView::setAudioData( const QByteArray& pcmData )
{
    if ( m_visualizer )
    {
        m_visualizer->setAudioData( pcmData );
    }
}

void AudiomaticImmersiveView::setSampleRate( int sampleRate )
{
    if ( m_visualizer )
    {
        m_visualizer->setSampleRate( sampleRate );
    }
}

void AudiomaticImmersiveView::clearAudio()
{
    if ( m_visualizer )
    {
        m_visualizer->clear();
    }
}

void AudiomaticImmersiveView::setAccentColor( const QColor& color )
{
    m_accentColor = color;
    if ( m_visualizer )
    {
        m_visualizer->setAccentColor( color );
    }
    update();
}

void AudiomaticImmersiveView::setReflectionOpacity( float opacity )
{
    if ( m_visualizer )
    {
        m_visualizer->setReflectionOpacity( opacity );
    }
}

void AudiomaticImmersiveView::syncSpectrumParameters( ExSpectrumWidget* source )
{
    if ( !source || !m_visualizer )
    {
        return;
    }
    m_visualizer->setAccentColor( source->accentColor() );
    m_visualizer->setReflectionEnabled( source->isReflectionEnabled() );
    m_visualizer->setReflectionOpacity( source->reflectionOpacity() );
    m_visualizer->setBarCount( source->barCount() );
    m_visualizer->setBarGap( source->barGap() );
    m_visualizer->setBarRadius( source->barRadius() );
    m_visualizer->setMaxFrequency( source->maxFrequency() );
    m_visualizer->setPeakCapEnabled( source->isPeakCapEnabled() );
    m_visualizer->setPeakCapHoldMs( source->peakCapHoldMs() );
    m_visualizer->setGravity( source->gravity() );
}

void AudiomaticImmersiveView::setCoverPixmap( const QPixmap& pixmap )
{
    m_coverPixmap = pixmap;
    // 沉浸模式按要求不展示专辑图，保持纯净大波形 + 顶部歌曲信息与歌词
}

void AudiomaticImmersiveView::setTrackInfo( const QString& title, const QString& artist )
{
    m_trackTitle  = title;
    m_trackArtist = artist;

    if ( m_labelSongTitle )
    {
        m_labelSongTitle->setText( title );
        m_labelSongTitle->setVisible( !title.isEmpty() );
    }
    if ( m_labelSongArtist )
    {
        m_labelSongArtist->setText( artist );
        m_labelSongArtist->setVisible( !artist.isEmpty() );
    }
}

void AudiomaticImmersiveView::setLyricsLines( const QList<ExLyricLine>& lines )
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->setLyricsLines( lines );
    }
}

void AudiomaticImmersiveView::setLyricPosition( qint64 positionMs )
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->setCurrentPositionMs( positionMs );
    }
}

void AudiomaticImmersiveView::setLyricHighlightColor( const QColor& color )
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->setHighlightColor( color );
    }
}

void AudiomaticImmersiveView::setLyricFontSizeLevel( int level )
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->setFontSizeLevel( level );
    }
}

void AudiomaticImmersiveView::setLyricStatusMessage( const QString& msg )
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->setStatusMessage( msg );
    }
}

void AudiomaticImmersiveView::clearLyrics()
{
    if ( m_lyricsWidget )
    {
        m_lyricsWidget->clear();
    }
}

void AudiomaticImmersiveView::setCpuUsageText( const QString& text, const QString& tip )
{
    if ( m_labelCpuUsage )
    {
        m_labelCpuUsage->setText( text );
        if ( !tip.isEmpty() )
        {
            m_labelCpuUsage->setToolTip( tip );
        }
    }
}

void AudiomaticImmersiveView::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );
    QPainter p( this );
    const QVariant scheme = qApp ? qApp->property( "_q_colorscheme" ) : QVariant();
    const bool dark = scheme.isValid() ? ( scheme.toInt() == 1 ) : ( qApp && qApp->palette().color( QPalette::Window ).lightness() < 128 );
    p.fillRect( rect(), dark ? QColor( 16, 16, 20 ) : QColor( 242, 242, 247 ) );
}

void AudiomaticImmersiveView::mouseDoubleClickEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        emit exitRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent( event );
}

void AudiomaticImmersiveView::contextMenuEvent( QContextMenuEvent* event )
{
    emit contextMenuRequested( event->globalPos() );
    event->accept();
}

void AudiomaticImmersiveView::keyPressEvent( QKeyEvent* event )
{
    if ( event->key() == Qt::Key_Escape )
    {
        emit exitRequested();
        event->accept();
        return;
    }
    QWidget::keyPressEvent( event );
}

void AudiomaticImmersiveView::changeEvent( QEvent* event )
{
    if ( event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange || event->type() == QEvent::ThemeChange
         || event->type() == QEvent::ApplicationPaletteChange )
    {
        updateThemeStyle();
        update();
    }
    QWidget::changeEvent( event );
}

void AudiomaticImmersiveView::updateThemeStyle()
{
    const QVariant scheme = qApp ? qApp->property( "_q_colorscheme" ) : QVariant();
    const bool dark = scheme.isValid() ? ( scheme.toInt() == 1 ) : ( qApp && qApp->palette().color( QPalette::Window ).lightness() < 128 );

    if ( m_labelSongTitle )
    {
        const QString titleColor = dark ? QStringLiteral( "#FFFFFF" ) : QStringLiteral( "#1C1C1C" );
        m_labelSongTitle->setStyleSheet( QStringLiteral( "color: %1; font-weight: 600;" ).arg( titleColor ) );
    }

    if ( m_labelSongArtist )
    {
        const QString artistColor = dark ? QStringLiteral( "rgba(255, 255, 255, 0.72)" ) : QStringLiteral( "rgba(0, 0, 0, 0.65)" );
        m_labelSongArtist->setStyleSheet( QStringLiteral( "color: %1;" ).arg( artistColor ) );
    }

    if ( m_labelCpuUsage )
    {
        const QString cpuColor = dark ? QStringLiteral( "color: rgba(255, 255, 255, 0.75);" )
                                      : QStringLiteral( "color: rgba(0, 0, 0, 0.65);" );
        m_labelCpuUsage->setStyleSheet( cpuColor );
    }
}
